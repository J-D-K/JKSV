#include <vector>

#include "ui.h"
#include "ttl.h"
#include "file.h"
#include "util.h"
#include "cfg.h"

static int ttlHelpX = 0;
static std::vector<ui::titleview *> ttlViews;
static ui::menu *ttlOpts;
ui::slideOutPanel *ui::ttlOptsPanel;
static ui::slideOutPanel *infoPanel;
static Mutex ttlViewLock = 0;

void ui::ttlRefresh()
{
    mutexLock(&ttlViewLock);
    for(int i = 0; i < (int)data::users.size(); i++)
        ttlViews[i]->refresh();
    mutexUnlock(&ttlViewLock);
}

static void ttlViewCallback(void *a)
{
    unsigned curUserIndex = data::getCurrentUserIndex();
    unsigned setUserTitleIndex = ttlViews[curUserIndex]->getSelected();
    data::setTitleIndex(setUserTitleIndex);
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    uint64_t tid = d->tid;

    switch(ui::padKeysDown())
    {
        case HidNpadButton_A:
            if(fs::mountSave(d->saveInfo))
            {
                ttlViews[curUserIndex]->setActive(false, true);   
                ui::fldPopulateMenu();
            }
            break;

        case HidNpadButton_B:
            ttlViews[curUserIndex]->setActive(false, false);
            ui::usrMenu->setActive(true);
            ui::changeState(USR_SEL);
            break;

        case HidNpadButton_X:
            ttlViews[curUserIndex]->setActive(false, true);
            ttlOpts->setActive(true);
            ui::ttlOptsPanel->openPanel();
            break;

        case HidNpadButton_Y:
            {
                cfg::addTitleToFavorites(tid);
                int newSel = data::getTitleIndexInUser(data::users[curUserIndex], tid);
                ttlViews[curUserIndex]->setSelected(newSel);
            }
            break;
    }
}

static void ttlOptsCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            {
                int curUserIndex = data::getCurrentUserIndex();
                ttlOpts->setActive(false);
                ui::ttlOptsPanel->closePanel();
                ttlViews[curUserIndex]->setActive(true, true);
                ui::updateInput();
            }
            break;
    }
}

static void ttlOptsPanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    ttlOpts->draw(panel, &ui::txtCont, true);
}

static void ttlOptsShowInfoPanel(void *)
{
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    data::titleInfo     *tinfo  = data::getTitleInfoByTID(utinfo->tid);

    char tmp[256];
    sprintf(tmp, ui::getUICString("infoStatus", 4), tinfo->author.c_str());

    size_t titleWidth = gfx::getTextWidth(tinfo->title.c_str(), 18);
    size_t pubWidth = gfx::getTextWidth(tmp, 18);
    if(titleWidth > 410 || pubWidth > 410)
    {
        size_t newWidth = titleWidth > pubWidth ? titleWidth : pubWidth;
        infoPanel->resizePanel(newWidth + 40, 720, 0);
    }
    else
        infoPanel->resizePanel(410, 720, 0);

    ttlOpts->setActive(false);
    ui::ttlOptsPanel->closePanel();
    infoPanel->openPanel();
}

static void ttlOptsBlacklistTitle(void *a)
{
    std::string title = data::getTitleNameByTID(data::getCurrentUserTitleInfo()->tid);
    ui::confirmArgs *conf = ui::confirmArgsCreate(false, cfg::addTitleToBlacklist, NULL, NULL, ui::getUICString("confirmBlacklist", 0), title.c_str());
    ui::confirm(conf);
}

static void ttlOptsDefinePath(void *a)
{
    uint64_t tid = data::getCurrentUserTitleInfo()->tid;
    std::string safeTitle = data::getTitleInfoByTID(tid)->safeTitle;
    std::string newSafeTitle = util::getStringInput(SwkbdType_QWERTY, safeTitle, ui::getUICString("swkbdNewSafeTitle", 0), 0x200, 0, NULL);
    if(!newSafeTitle.empty())
        cfg::pathDefAdd(tid, newSafeTitle);
}

static void ttlOptsToFileMode(void *a)
{
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    if(fs::mountSave(d->saveInfo))
    {
        data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
        std::string sdmcPath = util::generatePathByTID(utinfo->tid);
        ui::fmPrep((FsSaveDataType)d->saveInfo.save_data_type, "sv:/", sdmcPath, true);
        ui::usrSelPanel->closePanel();
        ui::ttlOptsPanel->closePanel();
        ui::changeState(FIL_MDE);
    }
}

static void ttlOptsDeleteAllBackups_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(ui::getUICString("threadStatusDeletingFile", 0));
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    std::string targetPath = util::generatePathByTID(d->tid);
    fs::dirList *backupList = new fs::dirList(targetPath);
    for(unsigned i = 0; i < backupList->getCount(); i++)
    {
        std::string delPath = targetPath + backupList->getItem(i);
        if(backupList->isDir(i))
        {
            delPath += "/";
            fs::delDir(delPath);
        }
        else
            fs::delfile(delPath);
    }
    delete backupList;
    t->finished = true;
}

static void ttlOptsDeleteAllBackups(void *a)
{
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    std::string currentTitle = data::getTitleNameByTID(d->tid);

    ui::confirmArgs *send = ui::confirmArgsCreate(cfg::config["holdDel"], ttlOptsDeleteAllBackups_t, NULL, NULL, ui::getUICString("confirmDeleteBackupsTitle", 0), currentTitle.c_str());
    ui::confirm(send);
}

static void ttlOptsResetSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    std::string title = data::getTitleNameByTID(d->tid);
    t->status->setStatus(ui::getUICString("threadStatusResettingSaveData", 0));

    fs::mountSave(d->saveInfo);
    fs::delDir("sv:/");
    fsdevCommitDevice("sv:/");
    fs::unmountSave();
    ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataResetSuccess", 0), title.c_str());
    t->finished = true;
}

static void ttlOptsResetSaveData(void *a)
{
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    if(d->saveInfo.save_data_type != FsSaveDataType_System)
    {
        std::string title = data::getTitleNameByTID(d->tid);
        ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdDel"], ttlOptsResetSaveData_t, NULL, NULL, ui::getUICString("confirmResetSaveData", 0), title.c_str());
        ui::confirm(conf);
    }
}

static void ttlOptsDeleteSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    data::user *u = data::getCurrentUser();
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    unsigned userIndex = data::getCurrentUserIndex();

    std::string title = data::getTitleNameByTID(d->tid);
    t->status->setStatus(ui::getUICString("threadStatusDeletingSaveData", 0), title.c_str());
    if(R_SUCCEEDED(fsDeleteSaveDataFileSystemBySaveDataSpaceId((FsSaveDataSpaceId)d->saveInfo.save_data_space_id, d->saveInfo.save_data_id)))
    {
        data::loadUsersTitles(false);
        if(u->titleInfo.size() == 0)
        {
            //Kick back to user
            ui::ttlOptsPanel->closePanel();//JIC
            ttlOpts->setActive(false);
            ttlViews[userIndex]->setActive(false, false);
            ui::usrMenu->setActive(true);
            ui::changeState(USR_SEL);
        }
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataDeleteSuccess", 0), title.c_str());
        ttlViews[userIndex]->refresh();
    }
    t->finished = true;
}

static void ttlOptsDeleteSaveData(void *a)
{
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    if(d->saveInfo.save_data_type != FsSaveDataType_System)
    {
        std::string title = data::getTitleNameByTID(d->tid);
        ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdDel"], ttlOptsDeleteSaveData_t, NULL, NULL, ui::getUICString("confirmDeleteSaveData", 0), title.c_str());
        ui::confirm(conf);
    }
}

static void ttlOptsExtendSaveData(void *a)
{
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    if(d->saveInfo.save_data_type != FsSaveDataType_System)
    {
        std::string expSizeStr = util::getStringInput(SwkbdType_NumPad, "", ui::getUICString("swkbdExpandSize", 0), 4, 0, NULL);
        uint64_t extMB = strtoul(expSizeStr.c_str(), NULL, 10) * 0x100000;
        fs::extendSaveDataThreaded(d, extMB);
    }
}

static void ttlOptsExportSVI(void *a)
{
    data::userTitleInfo *ut = data::getCurrentUserTitleInfo();
    data::titleInfo *t = data::getTitleInfoByTID(ut->tid);
    std::string out = fs::getWorkDir() + "svi/";
    fs::mkDir(out.substr(0, out.length() - 1));
    out += util::getIDStr(ut->tid) + ".svi";
    FILE *svi = fopen(out.c_str(), "wb");
    if(svi)
    {
        //Grab icon
        NsApplicationControlData *ctrlData = new NsApplicationControlData;
        uint64_t ctrlSize = 0;
        nsGetApplicationControlData(NsApplicationControlSource_Storage, ut->tid, ctrlData, sizeof(NsApplicationControlData), &ctrlSize);
        size_t jpegSize = ctrlSize - sizeof(ctrlData->nacp);

        fwrite(&ut->tid, sizeof(uint64_t), 1, svi);
        fwrite(&t->nacp, sizeof(NacpStruct), 1, svi);
        fwrite(ctrlData->icon, 1, jpegSize, svi);
        fclose(svi);
        delete ctrlData;
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popSVIExported", 0));
    }
}

static void infoPanelDraw(void *a)
{
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    data::titleInfo *t = data::getTitleInfoByTID(d->tid);
    SDL_Texture *panel = (SDL_Texture *)a;
    int width = 0, rectWidth = 0, iconX = 0, drawY = 310;
    SDL_QueryTexture(panel, NULL, NULL, &width, 0);
    rectWidth = width - 20;

    iconX = (width / 2) - 128;
    gfx::texDraw(panel, data::getTitleIconByTID(d->tid), iconX, 24);

    gfx::drawRect(panel, &ui::rectLt, 10, drawY, rectWidth, 38);
    gfx::drawTextf(panel, 18, 20, drawY + 10, &ui::txtCont, data::getTitleNameByTID(d->tid).c_str());
    drawY += 40;

    gfx::drawRect(panel, &ui::rectSh, 10, drawY, rectWidth, 38);
    gfx::drawTextf(panel, 18, 20, drawY + 10, &ui::txtCont, ui::getUICString("infoStatus", 4), t->author.c_str());
    drawY += 40;

    gfx::drawRect(panel, &ui::rectLt, 10, drawY, rectWidth, 38);
    gfx::drawTextf(panel, 18, 20, drawY + 10, &ui::txtCont, ui::getUICString("infoStatus", 0), d->tid);
    drawY += 40;

    gfx::drawRect(panel, &ui::rectSh, 10, drawY, rectWidth, 38);
    gfx::drawTextf(panel, 18, 20, drawY + 10, &ui::txtCont, ui::getUICString("infoStatus", 1), d->saveInfo.save_data_id);
    drawY += 40;

    uint32_t hours, mins;
    hours = ((d->playStats.playtime / 1e+9) / 60) / 60;
    mins = ((d->playStats.playtime / 1e+9) / 60) - (hours * 60);
    gfx::drawRect(panel, &ui::rectLt, 10, drawY, rectWidth, 38);
    gfx::drawTextf(panel, 18, 20, drawY + 10, &ui::txtCont, ui::getUICString("infoStatus", 2), hours, mins);
    drawY += 40;

    gfx::drawRect(panel, &ui::rectSh, 10, drawY, rectWidth, 38);
    gfx::drawTextf(panel, 18, 20, drawY + 10, &ui::txtCont, ui::getUICString("infoStatus", 3), d->playStats.total_launches);
    drawY += 40;

    gfx::drawRect(panel, &ui::rectLt, 10, drawY, rectWidth, 38);
    gfx::drawTextf(panel, 18, 20, drawY + 10, &ui::txtCont, ui::getUICString("infoStatus", 5), ui::getUICString("saveDataTypeText", d->saveInfo.save_data_type));
    drawY += 40;

    uint8_t saveType = d->saveInfo.save_data_type;
    if(saveType == FsSaveDataType_Cache)
    {
        gfx::drawRect(panel, &ui::rectSh, 10, drawY, rectWidth, 38);
        gfx::drawTextf(panel, 18, 20, drawY + 10, &ui::txtCont, ui::getUICString("infoStatus", 6), d->saveInfo.save_data_index);
        drawY += 40;
    }

    gfx::drawRect(panel, saveType == FsSaveDataType_Cache ? &ui::rectLt : &ui::rectSh, 10, drawY, rectWidth, 38);
    gfx::drawTextf(panel, 18, 20, drawY + 10, &ui::txtCont, ui::getUICString("infoStatus", 7), data::getCurrentUser()->getUsername().c_str());
}

static void infoPanelCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            infoPanel->closePanel();
            ui::ttlOptsPanel->openPanel();
            ttlOpts->setActive(true);
            ui::updateInput();
            break;
    }
}

void ui::ttlInit()
{
    ttlHelpX = 1220 - gfx::getTextWidth(ui::getUICString("helpTitle", 0), 18);

    for(data::user& u : data::users)
        ttlViews.emplace_back(new ui::titleview(u, 128, 128, 16, 16, 7, ttlViewCallback));

    ttlOpts = new ui::menu(10, 32, 390, 20, 7);
    ttlOpts->setCallback(ttlOptsCallback, NULL);
    ttlOpts->setActive(false);

    ttlOptsPanel = new ui::slideOutPanel(410, 720, 0, ui::SLD_RIGHT, ttlOptsPanelDraw);
    ui::registerPanel(ttlOptsPanel);

    infoPanel = new ui::slideOutPanel(410, 720, 0, ui::SLD_RIGHT, infoPanelDraw);
    ui::registerPanel(infoPanel);
    infoPanel->setCallback(infoPanelCallback, NULL);

    ttlOpts->setActive(false);
    for(int i = 0; i < 9; i++)
        ttlOpts->addOpt(NULL, ui::getUIString("titleOptions", i));

    //Information
    ttlOpts->optAddButtonEvent(0, HidNpadButton_A, ttlOptsShowInfoPanel, NULL);
    //Blacklist
    ttlOpts->optAddButtonEvent(1, HidNpadButton_A, ttlOptsBlacklistTitle, NULL);
    //Title Define
    ttlOpts->optAddButtonEvent(2, HidNpadButton_A, ttlOptsDefinePath, NULL);
    //File Mode
    ttlOpts->optAddButtonEvent(3, HidNpadButton_A, ttlOptsToFileMode, NULL);
    //Delete all backups
    ttlOpts->optAddButtonEvent(4, HidNpadButton_A, ttlOptsDeleteAllBackups, NULL);
    //Reset Save
    ttlOpts->optAddButtonEvent(5, HidNpadButton_A, ttlOptsResetSaveData, NULL);
    //Delete Save
    ttlOpts->optAddButtonEvent(6, HidNpadButton_A, ttlOptsDeleteSaveData, NULL);
    //Extend
    ttlOpts->optAddButtonEvent(7, HidNpadButton_A, ttlOptsExtendSaveData, NULL);
    //Export NACP
    ttlOpts->optAddButtonEvent(8, HidNpadButton_A, ttlOptsExportSVI, NULL);
}

void ui::ttlExit()
{
    for(ui::titleview *t : ttlViews)
        delete t;
        
    delete ttlOptsPanel;
    delete ttlOpts;
    delete infoPanel;
}

void ui::ttlSetActive(int usr, bool _set, bool _showSel)
{
    ttlViews[usr]->setActive(_set, _showSel);
}

void ui::ttlUpdate()
{
    unsigned curUserIndex = data::getCurrentUserIndex();

    ttlOpts->update();
    infoPanel->update();
    ui::fldUpdate();
    ttlViews[curUserIndex]->update();
}

void ui::ttlDraw(SDL_Texture *target)
{
    unsigned curUserIndex = data::getCurrentUserIndex();

    mutexLock(&ttlViewLock);
    ttlViews[curUserIndex]->draw(target);
    mutexUnlock(&ttlViewLock);
    if(ui::mstate == TTL_SEL && !fldPanel->isOpen())
        gfx::drawTextf(NULL, 18, ttlHelpX, 673, &ui::txtCont, ui::getUICString("helpTitle", 0));
}
