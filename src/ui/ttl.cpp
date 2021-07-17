#include <vector>

#include "ui.h"
#include "ttl.h"
#include "file.h"
#include "util.h"

int ttlHelpX = 0;
static std::vector<ui::titleview *> ttlViews;
static ui::menu *ttlOpts, *fldMenu;
static ui::slideOutPanel *ttlOptsPanel, *infoPanel, *fldPanel;//There's no reason to have a separate folder section
static fs::dirList *fldList;
static fs::backupArgs *backargs;
static std::string infoPanelString;

void ui::refreshAllViews()
{
    for(int i = 0; i < (int)data::users.size(); i++)
        ttlViews[i]->refresh();
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

        fldMenu->optAddButtonEvent(i + 1, HidNpadButton_A, fs::overwriteBackup, backargs);
        fldMenu->optAddButtonEvent(i + 1, HidNpadButton_X, fs::deleteBackup, backargs);
        fldMenu->optAddButtonEvent(i + 1, HidNpadButton_Y, fs::restoreBackup, backargs);
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
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    if(ui::confirm(false, ui::confBlacklist.c_str(), title.c_str()))
    {
        data::blacklistAdd(data::curData.saveID);
        ui::refreshAllViews();
    }
}

static void ttlOptsDefinePath(void *a)
{
    uint64_t tid = data::curData.saveID;
    std::string safeTitle = data::getTitleInfoByTID(tid)->safeTitle;
    std::string newSafeTitle = util::getStringInput(safeTitle, "Input New Output Folder", 32, 0, NULL);
    if(!newSafeTitle.empty())
        data::pathDefAdd(tid, newSafeTitle);
}

static void ttlOptsResetSaveData(void *a)
{
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    if(ui::confirm(data::config["holdDel"], ui::saveDataReset.c_str(), title.c_str()) && fs::mountSave(data::curData.saveInfo))
    {
        fs::delDir("sv:/");
        fsdevCommitDevice("sv:/");
        fs::unmountSave();
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::saveDataResetSuccess.c_str(), title.c_str());
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
    fldMenu->draw(target, &ui::txtCont, true);
    gfx::drawLine(target, &ui::divClr, 10, 648, 502, 648);
    gfx::drawTextf(target, 18, 32, 673, &ui::txtCont, ui::folderHelp.c_str());
}

static void ttlOptsDeleteSaveData(void *a)
{
    FsSaveDataAttribute attr;
    attr.application_id = data::curData.saveID;
    attr.uid = data::curData.saveInfo.save_data_type == FsSaveDataType_Account ? data::curUser.getUID() : util::u128ToAccountUID(0);
    attr.system_save_data_id = 0;
    attr.save_data_type = data::curData.saveInfo.save_data_type;
    attr.save_data_rank = data::curData.saveInfo.save_data_rank;
    attr.save_data_index = data::curData.saveInfo.save_data_index;

    std::string title = data::getTitleNameByTID(data::curData.saveID);
    if(ui::confirm(data::config["holdDel"], ui::confEraseNand.c_str(), title.c_str()) && R_SUCCEEDED(fsDeleteSaveDataFileSystemBySaveDataAttribute(FsSaveDataSpaceId_User, &attr)))
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
}

static void ttlOptsExtendSaveData(void *a)
{
    std::string expSizeStr = util::getStringInput("", "Enter New Size in MB", 4, 0, NULL);
    if(!expSizeStr.empty())
    {
        uint64_t expMB = strtoul(expSizeStr.c_str(), NULL, 10);

        data::titleInfo *extend = data::getTitleInfoByTID(data::curData.saveID);
        FsSaveDataSpaceId space = (FsSaveDataSpaceId)data::curData.saveInfo.save_data_space_id;
        uint64_t sid = data::curData.saveInfo.save_data_id;
        uint64_t expSize = expMB * 1024 * 1024;
        uint64_t journ = extend->nacp.user_account_save_data_journal_size;

        Result res = 0;
        if(R_SUCCEEDED(res = fs::extendSaveDataFileSystem(space, sid, expSize, journ)))
        {
            ui::showPopMessage(POP_FRAME_DEFAULT, "Save data expanded for %s!", extend->title.c_str());
            fs::logWrite("Extend Succeeded!\n");
        }
        else
        {
            int64_t totalSize = 0;
            fs::mountSave(data::curData.saveInfo);
            fsFsGetTotalSpace(fsdevGetDeviceFileSystem("sv"), "/", &totalSize);
            fs::unmountSave();

            fs::logWrite("Extend Failed: %uMB to %uMB  -> %X\n", totalSize / 1024 / 1024, expSize / 1024 / 1024, res);
            ui::showPopMessage(POP_FRAME_DEFAULT, "Failed to expand save data.");
        }
    }
}

static void infoPanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    gfx::texDraw(panel, data::getTitleIconByTID(data::curData.saveID), 77, 32);
    gfx::drawTextfWrap(panel, 18, 32, 320, 362, &ui::txtCont, infoPanelString.c_str());
}

void ui::ttlInit()
{
    ttlHelpX = 1220 - gfx::getTextWidth(ui::titleHelp.c_str(), 18);

    for(data::user& u : data::users)
        ttlViews.emplace_back(new ui::titleview(u, 128, 128, 16, 16, 7, ttlViewCallback));

    ttlOpts = new ui::menu;
    ttlOpts->setParams(10, 32, 390, 18, 8);
    ttlOpts->setCallback(ttlOptsCallback, NULL);
    ttlOpts->setActive(false);

    fldMenu = new ui::menu;
    fldMenu->setParams(10, 32, 492, 18, 8);
    fldMenu->setCallback(fldMenuCallback, NULL);
    fldMenu->setActive(false);

    ttlOptsPanel = new ui::slideOutPanel(410, 720, 0, ttlOptsPanelDraw);
    ui::registerPanel(ttlOptsPanel);

    infoPanel = new ui::slideOutPanel(410, 720, 0, infoPanelDraw);
    ui::registerPanel(infoPanel);
    infoPanel->setCallback(infoPanelCallback, NULL);

    fldPanel = new ui::slideOutPanel(512, 720, 0, fldPanelDraw);
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
    if(ui::mstate == TTL_SEL)
        gfx::drawTextf(NULL, 18, ttlHelpX, 673, &ui::txtCont, ui::titleHelp.c_str());
}
