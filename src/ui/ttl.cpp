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
    for(int i = 0; i < data::users.size(); i++)
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
    fldMenu->setOptFunc(0, FUNC_A, fs::createNewBackup, backargs);

    for(unsigned i = 0; i < fldList->getCount(); i++)
    {
        fldMenu->addOpt(NULL, fldList->getItem(i));

        fldMenu->setOptFunc(i + 1, FUNC_A, fs::overwriteBackup, backargs);
        fldMenu->setOptFunc(i + 1, FUNC_X, fs::deleteBackup, backargs);
        fldMenu->setOptFunc(i + 1, FUNC_Y, fs::restoreBackup, backargs);
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
            ttlViews[data::selUser]->setActive(false);
            ui::usrMenuSetActive(true);
            ui::changeState(USR_SEL);
            break;

        case HidNpadButton_X:
            if(data::curUser.getUID128() != 0)//system
            {
                data::selData = ttlViews[data::selUser]->getSelected();
                ttlOpts->setActive(true);
                ttlOptsPanel->openPanel();
            }
            break;

        case HidNpadButton_Y:
            {
                uint64_t sid = data::curData.saveID;
                data::favoriteTitle(sid);
                int newSel = data::getTitleIndexInUser(data::curUser, sid);
                ttlViews[data::selUser]->refresh();
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

static void ttlOptsResetSaveData(void *a)
{
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    if(ui::confirm(data::holdDel, ui::saveDataReset.c_str(), title.c_str()) && fs::mountSave(data::curData.saveInfo))
    {
        fs::delDir("sv:/");
        fsdevCommitDevice("sv:/");
        fs::unmountSave();
        ui::showPopup(POP_FRAME_DEFAULT, ui::saveDataResetSuccess.c_str(), title.c_str());
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
}

static void ttlOptsDeleteSaveData(void *a)
{
    FsSaveDataAttribute attr;
    attr.application_id = data::curData.saveID;
    attr.uid = data::curUser.getUID();
    attr.system_save_data_id = 0;
    attr.save_data_type = data::curData.saveInfo.save_data_type;
    attr.save_data_rank = data::curData.saveInfo.save_data_rank;
    attr.save_data_index = data::curData.saveInfo.save_data_index;

    std::string title = data::getTitleNameByTID(data::curData.saveID);
    if(ui::confirm(data::holdDel, ui::confEraseNand.c_str(), title.c_str()) && R_SUCCEEDED(fsDeleteSaveDataFileSystemBySaveDataAttribute(FsSaveDataSpaceId_User, &attr)))
    {
        data::loadUsersTitles(false);
        if(data::curUser.titleInfo.size() == 0)
        {
            //Kick back to user
            ttlOptsPanel->closePanel();//JIC
            ttlViews[data::selUser]->setActive(false);
            ui::usrMenuSetActive(true);
            ui::changeState(USR_SEL);
        }
        ui::showPopup(POP_FRAME_DEFAULT, ui::saveDataDeleteSuccess.c_str(), title.c_str());
        ttlViews[data::selUser]->refresh();
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
    fldMenu->setParams(10, 32, 390, 18, 8);
    fldMenu->setCallback(fldMenuCallback, NULL);
    fldMenu->setActive(false);

    ttlOptsPanel = new ui::slideOutPanel(410, 720, 0, ttlOptsPanelDraw);
    ui::registerPanel(ttlOptsPanel);

    infoPanel = new ui::slideOutPanel(410, 720, 0, infoPanelDraw);
    ui::registerPanel(infoPanel);
    infoPanel->setCallback(infoPanelCallback, NULL);

    fldPanel = new ui::slideOutPanel(410, 720, 0, fldPanelDraw);
    ui::registerPanel(fldPanel);

    fldList = new fs::dirList;
    backargs = new fs::backupArgs;

    ttlOpts->setActive(false);
    ttlOpts->addOpt(NULL, ui::titleOptString[0]);
    ttlOpts->setOptFunc(0, FUNC_A, ttlOptsShowInfoPanel, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[1]);
    ttlOpts->setOptFunc(1, FUNC_A, ttlOptsBlacklistTitle, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[2]);
    ttlOpts->setOptFunc(2, FUNC_A, ttlOptsResetSaveData, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[3]);
    ttlOpts->setOptFunc(3, FUNC_A, ttlOptsDeleteSaveData, NULL);
}

void ui::ttlExit()
{
    for(ui::titleview *t : ttlViews)
        delete t;

    delete ttlOptsPanel;
    delete ttlOpts;
    delete infoPanel;
    delete fldPanel;
    delete ttlOpts;
    delete fldMenu;
    delete fldList;
    delete backargs;
}

void ui::ttlSetActive(int usr)
{
    ttlViews[usr]->setActive(true);
}

void ui::ttlUpdate()
{
    ttlOpts->update();
    infoPanel->update();
    fldMenu->update();

    //todo: this better
    if(ttlOptsPanel->isOpen() || infoPanel->isOpen() || fldPanel->isOpen())
        return;

    ttlViews[data::selUser]->update();
}

void ui::ttlDraw(SDL_Texture *target)
{
    ttlViews[data::selUser]->draw(target);
    if(ui::mstate == TTL_SEL)
        gfx::drawTextf(NULL, 18, ttlHelpX, 673, &ui::txtCont, ui::titleHelp.c_str());
}
