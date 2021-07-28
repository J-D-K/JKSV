#include <vector>

#include "ui.h"
#include "ttl.h"
#include "file.h"
#include "util.h"

int ttlHelpX = 0, fldHelpWidth = 0;
static std::vector<ui::titleview *> ttlViews;
static ui::menu *ttlOpts, *fldMenu;
static ui::slideOutPanel *ttlOptsPanel, *infoPanel, *fldPanel;//There's no reason to have a separate folder section
static fs::dirList *fldList;
static fs::backupArgs *backargs;
static std::string infoPanelString;
static SDL_Texture *fldBuffer;//This is so folder menu doesn't draw over guide

void ui::refreshAllViews()
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

    ui::confirmArgs *conf = ui::confirmArgsCreate(data::config["holdOver"], fs::overwriteBackup, a, true, ui::confOverwrite.c_str(), itm.c_str());
    ui::confirm(conf);
}

static void fldFuncDelete(void *a)
{
    fs::backupArgs *b = (fs::backupArgs *)a;
    fs::dirList *d = (fs::dirList *)b->d;
    ui::menu *m = (ui::menu *)b->m;

    int sel = m->getSelected() - 1;//Skip 'New'
    std::string itm = d->getItem(sel);

    ui::confirmArgs *conf = ui::confirmArgsCreate(data::config["holdDel"], fs::deleteBackup, a, true, ui::confDel.c_str(), itm.c_str());
    ui::confirm(conf);
}

static void fldFuncRestore(void *a)
{
    fs::backupArgs *b = (fs::backupArgs *)a;
    fs::dirList *d = (fs::dirList *)b->d;
    ui::menu *m = (ui::menu *)b->m;

    int sel = m->getSelected() - 1;//Skip 'New'
    std::string itm = d->getItem(sel);

    ui::confirmArgs *conf = ui::confirmArgsCreate(data::config["holdRest"], fs::restoreBackup, a, true, ui::confRestore.c_str(), itm.c_str());
    ui::confirm(conf);
}

void ui::populateFldMenu()
{
    fldMenu->reset();

    util::createTitleDirectoryByTID(data::curData.saveID);
    std::string targetDir = util::generatePathByTID(data::curData.saveID);

    fldList->reassign(targetDir);
    fs::loadPathFilters(targetDir + "pathFilters.txt");

    *backargs = {fldMenu, fldList};

    fldMenu->addOpt(NULL, "New");
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
    data::selData = ttlViews[data::selUser]->getSelected();
    switch(ui::padKeysDown())
    {
        case HidNpadButton_A:
            if(fs::mountSave(data::curData.saveInfo))
                ui::populateFldMenu();
            break;

        case HidNpadButton_B:
            ttlViews[data::selUser]->setActive(false, false);
            ui::usrMenu->setActive(true);
            ui::changeState(USR_SEL);
            break;

        case HidNpadButton_X:
            if(data::curUser.getUID128() != 0)//system
            {
                data::selData = ttlViews[data::selUser]->getSelected();
                ttlViews[data::selUser]->setActive(false, true);
                ttlOpts->setActive(true);
                ttlOptsPanel->openPanel();
            }
            break;

        case HidNpadButton_Y:
            {
                uint64_t sid = data::curData.saveID;
                data::favoriteTitle(sid);
                int newSel = data::getTitleIndexInUser(data::curUser, sid);
                ui::refreshAllViews();
                ttlViews[data::selUser]->setSelected(newSel);
            }
            break;
    }
}

static void ttlOptsCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            ttlOpts->setActive(false);
            ttlOptsPanel->closePanel();
            ttlViews[data::selUser]->setActive(true, true);
            ui::updateInput();
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
    ttlOptsPanel->closePanel();
    infoPanelString = util::getInfoString(data::curUser, data::curData.saveID);
    infoPanel->openPanel();
}

static void ttlOptsBlacklistTitle(void *a)
{
    uint64_t *sendTid = new uint64_t;
    *sendTid = data::curData.saveID;
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    ui::confirmArgs *conf = ui::confirmArgsCreate(false, data::blacklistAdd, sendTid, true, ui::confBlacklist.c_str(), title.c_str());
    ui::confirm(conf);
}

static void ttlOptsDefinePath(void *a)
{
    uint64_t tid = data::curData.saveID;
    std::string safeTitle = data::getTitleInfoByTID(tid)->safeTitle;
    std::string newSafeTitle = util::getStringInput(SwkbdType_QWERTY, safeTitle, "Input New Output Folder", 0x200, 0, NULL);
    if(!newSafeTitle.empty())
        data::pathDefAdd(tid, newSafeTitle);
}

static void ttlOptsResetSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    t->status->setStatus("Resetting save data for " + title + "...");

    fs::mountSave(data::curData.saveInfo);
    fs::delDir("sv:/");
    fsdevCommitDevice("sv:/");
    fs::unmountSave();
    ui::showPopMessage(POP_FRAME_DEFAULT, ui::saveDataResetSuccess.c_str(), title.c_str());
    t->finished = true;
}

static void ttlOptsResetSaveData(void *a)
{
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    ui::confirmArgs *conf = ui::confirmArgsCreate(data::config["holdDel"], ttlOptsResetSaveData_t, NULL, true, ui::saveDataReset.c_str(), title.c_str());
    ui::confirm(conf);
}

static void ttlOptsDeleteSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    t->status->setStatus("Deleting save data for " + title + "...");
    if(R_SUCCEEDED(fsDeleteSaveDataFileSystemBySaveDataSpaceId((FsSaveDataSpaceId)data::curData.saveInfo.save_data_space_id, data::curData.saveInfo.save_data_id)))
    {
        data::loadUsersTitles(false);
        if(data::curUser.titleInfo.size() == 0)
        {
            //Kick back to user
            ttlOptsPanel->closePanel();//JIC
            ttlOpts->setActive(false);
            ttlViews[data::selUser]->setActive(false, false);
            ui::usrMenu->setActive(true);
            ui::changeState(USR_SEL);
        }
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::saveDataDeleteSuccess.c_str(), title.c_str());
        ttlViews[data::selUser]->refresh();
    }
    t->finished = true;
}

static void ttlOptsDeleteSaveData(void *a)
{
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    ui::confirmArgs *conf = ui::confirmArgsCreate(data::config["holdDel"], ttlOptsDeleteSaveData_t, NULL, true, ui::confEraseNand.c_str(), title.c_str());
    ui::confirm(conf);
}

static void ttlOptsExtendSaveData_t(void *a)
{
    threadInfo *w = (threadInfo *)a;
    std::string expSizeStr = util::getStringInput(SwkbdType_NumPad, "", "Enter New Size in MB", 4, 0, NULL);
    if(!expSizeStr.empty())
    {
        data::titleInfo *extend = data::getTitleInfoByTID(data::curData.saveID);
        w->status->setStatus("Expanding save filesystem for " + extend->title);
        uint64_t expMB = strtoul(expSizeStr.c_str(), NULL, 10);
        FsSaveDataSpaceId space = (FsSaveDataSpaceId)data::curData.saveInfo.save_data_space_id;
        uint64_t sid = data::curData.saveInfo.save_data_id;
        int64_t expSize = expMB * 1024 * 1024;
        int64_t journ = 0;
        switch(data::curData.saveInfo.save_data_type)
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

        Result res = 0;
        if(R_FAILED(res = fsExtendSaveDataFileSystem(space, sid, expSize, journ)))
        {
            int64_t totalSize = 0;
            fs::mountSave(data::curData.saveInfo);
            fsFsGetTotalSpace(fsdevGetDeviceFileSystem("sv"), "/", &totalSize);
            fs::unmountSave();

            fs::logWrite("Extend Failed: %uMB to %uMB  -> %X\n", totalSize / 1024 / 1024, expSize / 1024 / 1024, res);
            ui::showPopMessage(POP_FRAME_DEFAULT, "Failed to expand save data.");
        }
    }
    w->finished = true;
}

static void ttlOptsExtendSaveData(void *a)
{
    ui::newThread(ttlOptsExtendSaveData_t, NULL, NULL);
}

static void infoPanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    gfx::texDraw(panel, data::getTitleIconByTID(data::curData.saveID), 77, 32);
    gfx::drawTextfWrap(panel, 18, 32, 320, 362, &ui::txtCont, infoPanelString.c_str());
}

static void infoPanelCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            infoPanel->closePanel();
            ttlOptsPanel->openPanel();
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
    gfx::drawTextf(target, 18, 32, 673, &ui::txtCont, ui::folderHelp.c_str());
}

void ui::ttlInit()
{
    ttlHelpX = 1220 - gfx::getTextWidth(ui::titleHelp.c_str(), 18);
    fldHelpWidth = gfx::getTextWidth(ui::folderHelp.c_str(), 18);

    for(data::user& u : data::users)
        ttlViews.emplace_back(new ui::titleview(u, 128, 128, 16, 16, 7, ttlViewCallback));

    ttlOpts = new ui::menu;
    ttlOpts->setParams(10, 32, 390, 20, 7);
    ttlOpts->setCallback(ttlOptsCallback, NULL);
    ttlOpts->setActive(false);

    fldMenu = new ui::menu;
    fldMenu->setParams(10, 32, fldHelpWidth + 44, 20, 6);
    fldMenu->setCallback(fldMenuCallback, NULL);
    fldMenu->setActive(false);

    ttlOptsPanel = new ui::slideOutPanel(410, 720, 0, ttlOptsPanelDraw);
    ui::registerPanel(ttlOptsPanel);

    infoPanel = new ui::slideOutPanel(410, 720, 0, infoPanelDraw);
    ui::registerPanel(infoPanel);
    infoPanel->setCallback(infoPanelCallback, NULL);

    fldPanel = new ui::slideOutPanel(fldHelpWidth + 64, 720, 0, fldPanelDraw);
    fldBuffer = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET, fldHelpWidth + 64, 647);
    ui::registerPanel(fldPanel);

    fldList = new fs::dirList;
    backargs = new fs::backupArgs;

    ttlOpts->setActive(false);
    ttlOpts->addOpt(NULL, ui::titleOptString[0]);
    ttlOpts->optAddButtonEvent(0, HidNpadButton_A, ttlOptsShowInfoPanel, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[1]);
    ttlOpts->optAddButtonEvent(1, HidNpadButton_A, ttlOptsBlacklistTitle, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[2]);
    ttlOpts->optAddButtonEvent(2, HidNpadButton_A, ttlOptsDefinePath, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[3]);
    ttlOpts->optAddButtonEvent(3, HidNpadButton_A, ttlOptsResetSaveData, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[4]);
    ttlOpts->optAddButtonEvent(4, HidNpadButton_A, ttlOptsDeleteSaveData, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[5]);
    ttlOpts->optAddButtonEvent(5, HidNpadButton_A, ttlOptsExtendSaveData, NULL);

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
    ttlOpts->update();
    infoPanel->update();
    fldMenu->update();
    ttlViews[data::selUser]->update();
}

void ui::ttlDraw(SDL_Texture *target)
{
    ttlViews[data::selUser]->draw(target);
    if(ui::mstate == TTL_SEL && !fldPanel->isOpen())
        gfx::drawTextf(NULL, 18, ttlHelpX, 673, &ui::txtCont, ui::titleHelp.c_str());
}
