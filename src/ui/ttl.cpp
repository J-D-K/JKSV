#include <vector>

#include "ui.h"
#include "ttl.h"
#include "file.h"
#include "util.h"
#include "cfg.h"

static int ttlHelpX = 0, fldHelpWidth = 0;
static std::vector<ui::titleview *> ttlViews;
static ui::menu *ttlOpts, *fldMenu;
ui::slideOutPanel *ui::ttlOptsPanel;
static ui::slideOutPanel *infoPanel, *fldPanel;//There's no reason to have a separate folder section
static fs::dirList *fldList;
static fs::backupArgs *backargs;
static std::string infoPanelString;
static SDL_Texture *fldBuffer;//This is so folder menu doesn't draw over guide

void ui::ttlRefresh()
{
    for(int i = 0; i < (int)data::users.size(); i++)
        ttlViews[i]->refresh();
}

static void fldFuncOverwrite(void *a)
{
    fs::backupArgs *b = (fs::backupArgs *)a;
    fs::dirList *d = (fs::dirList *)b->d;
    ui::menu *m = (ui::menu *)b->m;

    int sel = m->getSelected() - 1;//Skip 'New'
    std::string itm = d->getItem(sel);

    ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdOver"], fs::overwriteBackup, a, true, ui::getUICString("confirmOverwrite", 0), itm.c_str());
    ui::confirm(conf);
}

static void fldFuncDelete(void *a)
{
    fs::backupArgs *b = (fs::backupArgs *)a;
    fs::dirList *d = (fs::dirList *)b->d;
    ui::menu *m = (ui::menu *)b->m;

    int sel = m->getSelected() - 1;//Skip 'New'
    std::string itm = d->getItem(sel);

    ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdDel"], fs::deleteBackup, a, true, ui::getUICString("confirmDelete", 0), itm.c_str());
    ui::confirm(conf);
}

static void fldFuncRestore(void *a)
{
    fs::backupArgs *b = (fs::backupArgs *)a;
    fs::dirList *d = (fs::dirList *)b->d;
    ui::menu *m = (ui::menu *)b->m;

    int sel = m->getSelected() - 1;//Skip 'New'
    std::string itm = d->getItem(sel);

    ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdRest"], fs::restoreBackup, a, true, ui::getUICString("confirmRestore", 0), itm.c_str());
    ui::confirm(conf);
}

void ui::populateFldMenu()
{
    fldMenu->reset();

    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    util::createTitleDirectoryByTID(d->tid);
    std::string targetDir = util::generatePathByTID(d->tid);

    fldList->reassign(targetDir);

    char filterPath[128];
    sprintf(filterPath, "sdmc:/config/JKSV/0x%016lX_filter.txt", d->tid);
    fs::loadPathFilters(d->tid);

    *backargs = {fldMenu, fldList};

    fldMenu->addOpt(NULL, ui::getUICString("newFolderPopFldMenu", 0));
    fldMenu->optAddButtonEvent(0, HidNpadButton_A, fs::createNewBackup, backargs);

    for(unsigned i = 0; i < fldList->getCount(); i++)
    {
        fldMenu->addOpt(NULL, fldList->getItem(i));

        fldMenu->optAddButtonEvent(i + 1, HidNpadButton_A, fldFuncOverwrite, backargs);
        fldMenu->optAddButtonEvent(i + 1, HidNpadButton_X, fldFuncDelete, backargs);
        fldMenu->optAddButtonEvent(i + 1, HidNpadButton_Y, fldFuncRestore, backargs);
    }
    fldMenu->setActive(true);
    fldPanel->openPanel();
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
                ui::populateFldMenu();
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

static void ttlOptsShowInfoPanel(void *a)
{
    ttlOpts->setActive(false);
    ui::ttlOptsPanel->closePanel();
    infoPanelString = util::getInfoString(*data::getCurrentUser(), data::getCurrentUserTitleInfo()->tid);
    infoPanel->openPanel();
}

static void ttlOptsBlacklistTitle(void *a)
{
    std::string title = data::getTitleNameByTID(data::getCurrentUserTitleInfo()->tid);
    ui::confirmArgs *conf = ui::confirmArgsCreate(false, cfg::addTitleToBlacklist, NULL, true, ui::getUICString("confirmBlacklist", 0), title.c_str());
    ui::confirm(conf);
}

static void ttlOptsDefinePath(void *a)
{
    uint64_t tid = data::getCurrentUserTitleInfo()->tid;
    std::string safeTitle = data::getTitleInfoByTID(tid)->safeTitle;
    std::string newSafeTitle = util::getStringInput(SwkbdType_QWERTY, safeTitle, ui::getUICString("newSafeTitle", 0), 0x200, 0, NULL);
    if(!newSafeTitle.empty())
        cfg::pathDefAdd(tid, newSafeTitle);
}

static void ttlOptsToFileMode(void *a)
{
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    if(fs::mountSave(d->saveInfo))
    {
        ui::fmPrep((FsSaveDataType)d->saveInfo.save_data_type, "sv:/", true);
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
    ui::confirmArgs *send = ui::confirmArgsCreate(cfg::config["holdDel"], ttlOptsDeleteAllBackups_t, NULL, true, ui::getUICString("confirmDeleteBackupsTitle", 0), currentTitle.c_str());
    ui::confirm(send);
}

static void ttlOptsResetSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    std::string title = data::getTitleNameByTID(d->tid);
    t->status->setStatus(ui::getUICString("threadStatusResettingSaveData", 0), title.c_str());

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
        ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdDel"], ttlOptsResetSaveData_t, NULL, true, ui::getUICString("confirmResetSaveData", 0), title.c_str());
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
        ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdDel"], ttlOptsDeleteSaveData_t, NULL, true, ui::getUICString("confirmDeleteSaveData", 0), title.c_str());
        ui::confirm(conf);
    }
}

static void ttlOptsExtendSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();

    std::string expSizeStr = util::getStringInput(SwkbdType_NumPad, "", ui::getUICString("expandSize", 0), 4, 0, NULL);
    if(!expSizeStr.empty())
    {
        int64_t journ = 0, expSize;
        data::titleInfo *extend = data::getTitleInfoByTID(d->tid);
        t->status->setStatus(ui::getUICString("threadStatusExtendingSaveData", 0), extend->title.c_str());
        //Get journal size
        switch(d->saveInfo.save_data_type)
        {
            case FsSaveDataType_Account:
                if(extend->nacp.user_account_save_data_journal_size_max > extend->nacp.user_account_save_data_journal_size)
                    journ = extend->nacp.user_account_save_data_journal_size_max;
                else
                    journ = extend->nacp.user_account_save_data_journal_size;
                break;

            case FsSaveDataType_Bcat:
                journ = extend->nacp.bcat_delivery_cache_storage_size;
                break;

            case FsSaveDataType_Cache:
                if(extend->nacp.cache_storage_data_and_journal_size_max > extend->nacp.cache_storage_journal_size)
                    journ = extend->nacp.cache_storage_data_and_journal_size_max;
                else
                    journ = extend->nacp.cache_storage_journal_size;
                break;

            case FsSaveDataType_Device:
                if(extend->nacp.device_save_data_journal_size_max > extend->nacp.device_save_data_journal_size)
                    journ = extend->nacp.device_save_data_journal_size_max;
                else
                    journ = extend->nacp.device_save_data_journal_size;
                break;

            default:
                //will just fail
                journ = 0;
                break;
        }
        uint64_t expMB = strtoul(expSizeStr.c_str(), NULL, 10);
        expSize = expMB * 0x100000;
        FsSaveDataSpaceId space = (FsSaveDataSpaceId)d->saveInfo.save_data_space_id;
        uint64_t sid = d->saveInfo.save_data_id;
        Result res = 0;
        if(R_FAILED(res = fsExtendSaveDataFileSystem(space, sid, expSize, journ)))
        {
            int64_t totalSize = 0;
            fs::mountSave(d->saveInfo);
            fsFsGetTotalSpace(fsdevGetDeviceFileSystem("sv"), "/", &totalSize);
            fs::unmountSave();

            fs::logWrite(ui::getUICString("expandSizeFailed", 0), totalSize / 1024 / 1024, expSize / 1024 / 1024, res);
            ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataExtendFailed", 0));
        }
        else
            ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataExtendSuccess", 0), extend->title.c_str());
    }
    t->finished = true;
}

static void ttlOptsExtendSaveData(void *a)
{
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    if(d->saveInfo.save_data_type != FsSaveDataType_System)
        ui::newThread(ttlOptsExtendSaveData_t, NULL, NULL);
}

static void infoPanelDraw(void *a)
{
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    SDL_Texture *panel = (SDL_Texture *)a;
    gfx::texDraw(panel, data::getTitleIconByTID(d->tid), 77, 32);
    gfx::drawTextfWrap(panel, 18, 32, 320, 362, &ui::txtCont, infoPanelString.c_str());
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

static void fldMenuCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            fs::unmountSave();
            fs::freePathFilters();
            fldMenu->setActive(false);
            fldPanel->closePanel();
            break;
    }
    ui::updateInput();
}

static void fldPanelDraw(void *a)
{
    SDL_Texture *target = (SDL_Texture *)a;
    gfx::clearTarget(fldBuffer, &ui::slidePanelColor);
    fldMenu->draw(fldBuffer, &ui::txtCont, true);
    gfx::texDraw(target, fldBuffer, 0, 0);
    gfx::drawLine(target, &ui::divClr, 10, 648, fldHelpWidth + 54, 648);
    gfx::drawTextf(target, 18, 32, 673, &ui::txtCont, ui::getUICString("helpFolder", 0));
}

void ui::ttlInit()
{
    ttlHelpX = 1220 - gfx::getTextWidth(ui::getUICString("helpTitle", 0), 18);
    fldHelpWidth = gfx::getTextWidth(ui::getUICString("helpFolder", 0), 18);

    for(data::user& u : data::users)
        ttlViews.emplace_back(new ui::titleview(u, 128, 128, 16, 16, 7, ttlViewCallback));

    ttlOpts = new ui::menu(10, 32, 390, 20, 7);
    ttlOpts->setCallback(ttlOptsCallback, NULL);
    ttlOpts->setActive(false);

    fldMenu = new ui::menu(10, 8, fldHelpWidth + 44, 20, 6);
    fldMenu->setCallback(fldMenuCallback, NULL);
    fldMenu->setActive(false);

    ttlOptsPanel = new ui::slideOutPanel(410, 720, 0, ui::SLD_RIGHT, ttlOptsPanelDraw);
    ui::registerPanel(ttlOptsPanel);

    infoPanel = new ui::slideOutPanel(410, 720, 0, ui::SLD_RIGHT, infoPanelDraw);
    ui::registerPanel(infoPanel);
    infoPanel->setCallback(infoPanelCallback, NULL);

    fldPanel = new ui::slideOutPanel(fldHelpWidth + 64, 720, 0, ui::SLD_RIGHT, fldPanelDraw);
    fldBuffer = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET, fldHelpWidth + 64, 647);
    ui::registerPanel(fldPanel);

    fldList = new fs::dirList;
    backargs = new fs::backupArgs;

    ttlOpts->setActive(false);
    for(int i = 0; i < 8; i++)
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
}

void ui::ttlExit()
{
    for(ui::titleview *t : ttlViews)
        delete t;

    SDL_DestroyTexture(fldBuffer);
    delete ttlOptsPanel;
    delete ttlOpts;
    delete infoPanel;
    delete fldPanel;
    delete fldMenu;
    delete fldList;
    delete backargs;
}

void ui::ttlSetActive(int usr)
{
    ttlViews[usr]->setActive(true, true);
}

void ui::ttlUpdate()
{
    unsigned curUserIndex = data::getCurrentUserIndex();

    ttlOpts->update();
    infoPanel->update();
    fldMenu->update();
    ttlViews[curUserIndex]->update();
}

void ui::ttlDraw(SDL_Texture *target)
{
    unsigned curUserIndex = data::getCurrentUserIndex();

    ttlViews[curUserIndex]->draw(target);
    if(ui::mstate == TTL_SEL && !fldPanel->isOpen())
        gfx::drawTextf(NULL, 18, ttlHelpX, 673, &ui::txtCont, ui::getUICString("helpTitle", 0));
}
