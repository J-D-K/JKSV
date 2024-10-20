#include <string>
#include <vector>
#include <algorithm>
#include <switch.h>

#include "file.h"
#include "data.h"
#include "ui.h"
#include "util.h"
#include "type.h"

#include "usr.h"
#include "ttl.h"

static const char *settText = ui::getUICString("mainMenuSettings", 0), *extText = ui::getUICString("mainMenuExtras", 0);

//Main menu/Users + options, folder
ui::menu *ui::usrMenu;
ui::slideOutPanel *ui::usrSelPanel;

static ui::menu *usrOptMenu, *saveCreateMenu, *deviceSaveMenu, *bcatSaveMenu, *cacheSaveMenu;
//All save types have different entries.
static ui::slideOutPanel *usrOptPanel, *saveCreatePanel, *deviceSavePanel, *bcatSavePanel, *cacheSavePanel;

//Icons for settings + extras
static SDL_Texture *sett, *ext;

//This stores save ids to match with saveCreateMenu
//Probably needs/should be changed
static std::vector<uint64_t> accSids, devSids, bcatSids, cacheSids;

static unsigned usrHelpX = 0;

//Sort save create tids alphabetically by title from data
static struct
{
    bool operator()(const uint64_t& tid1, const uint64_t& tid2)
    {
        std::string tid1Title = data::getTitleNameByTID(tid1);
        std::string tid2Title = data::getTitleNameByTID(tid2);

        uint32_t pointA = 0, pointB = 0;
        for(unsigned i = 0, j = 0; i < tid1Title.length(); )
        {
            ssize_t tid1Cnt = decode_utf8(&pointA, (const uint8_t *)&tid1Title.c_str()[i]);
            ssize_t tid2Cnt = decode_utf8(&pointB, (const uint8_t *)&tid2Title.c_str()[j]);

            pointA = tolower(pointA), pointB = tolower(pointB);
            if(pointA != pointB)
                return pointA < pointB;

            i += tid1Cnt, j += tid2Cnt;
        }
        return false;
    }
} sortCreateTIDs;

static void onMainChange(void *a)
{
    if(ui::usrMenu->getSelected() < (int)data::users.size())
    {
        unsigned setUser = ui::usrMenu->getSelected();
        data::setUserIndex(setUser);
    }
}

static void _usrSelPanelDraw(void *a)
{
    SDL_Texture *target = (SDL_Texture *)a;
    gfx::texDraw(target, ui::sideBar, 0, 0);
    ui::usrMenu->draw(target, &ui::txtCont, false);
}

static void toOPT(void *a)
{
    ui::changeState(OPT_MNU);
    ui::usrMenu->setActive(false);
    ui::settMenu->setActive(true);
}

static void toEXT(void *a)
{
    ui::changeState(EX_MNU);
    ui::usrMenu->setActive(false);
    ui::extMenu->setActive(true);
}

static void usrOptCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            usrOptPanel->closePanel();
            ui::usrMenu->setActive(true);
            usrOptMenu->setActive(false);
            break;
    }
}

static void saveCreateCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            usrOptMenu->setActive(true);
            usrOptPanel->openPanel();
            saveCreateMenu->setActive(false);
            saveCreatePanel->closePanel();
            deviceSaveMenu->setActive(false);
            deviceSavePanel->closePanel();
            bcatSaveMenu->setActive(false);
            bcatSavePanel->closePanel();
            cacheSaveMenu->setActive(false);
            cacheSavePanel->closePanel();
            break;
    }
}

static void usrOptSaveCreate(void *a)
{
    ui::menu *m = (ui::menu *)a;
    int devPos = m->getOptPos(ui::getUICString("saveTypeMainMenu", 0));
    int bcatPos = m->getOptPos(ui::getUICString("saveTypeMainMenu", 1));
    int cachePos = m->getOptPos(ui::getUICString("saveTypeMainMenu", 2));

    ui::updateInput();
    int sel = m->getSelected();
    bool closeUsrOpt = false;
    if(sel == devPos && deviceSaveMenu->getOptCount() > 0)
    {
        deviceSaveMenu->setActive(true);
        deviceSavePanel->openPanel();
        closeUsrOpt = true;
    }
    else if(sel == bcatPos && bcatSaveMenu->getOptCount() > 0)
    {
        bcatSaveMenu->setActive(true);
        bcatSavePanel->openPanel();
        closeUsrOpt = true;
    }
    else if(sel == cachePos && cacheSaveMenu->getOptCount() > 0)
    {
        cacheSaveMenu->setActive(true);
        cacheSavePanel->openPanel();
        closeUsrOpt = true;
    }
    else if(sel < devPos)
    {
        saveCreateMenu->setActive(true);
        saveCreatePanel->openPanel();
        closeUsrOpt = true;
    }

    if(closeUsrOpt)
    {
        usrOptMenu->setActive(false);
        usrOptPanel->closePanel();
    }
}

//nsDeleteUserSaveDataAll only works if the user is selected at system level
static void usrOptDeleteAllUserSaves_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    data::user *u = data::getCurrentUser();
    int curUserIndex = data::getCurrentUserIndex();
    int devUser = ui::usrMenu->getOptPos(ui::getUICString("saveTypeMainMenu", 0));

    for(data::userTitleInfo& tinf : u->titleInfo)
    {
        if(tinf.saveInfo.save_data_type != FsSaveDataType_System && (tinf.saveInfo.save_data_type != FsSaveDataType_Device || curUserIndex == devUser))
        {
            t->status->setStatus(ui::getUICString("threadStatusDeletingSaveData", 0), data::getTitleNameByTID(tinf.tid).c_str());
            fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId_User, tinf.saveInfo.save_data_id);
        }
    }
    data::loadUsersTitles(false);
    ui::ttlRefresh();
    t->finished = true;
}

static void usrOptDeleteAllUserSaves(void *a)
{
    data::user *u = data::getCurrentUser();
    ui::confirmArgs *conf = ui::confirmArgsCreate(true, usrOptDeleteAllUserSaves_t, NULL, NULL, ui::getUICString("saveDataDeleteAllUser", 0), u->getUsername().c_str());
    ui::confirm(conf);
}

static void usrOptPanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    usrOptMenu->draw(panel, &ui::txtCont, true);
}

static void saveCreatePanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    saveCreateMenu->draw(panel, &ui::txtCont, true);
}

static void deviceSavePanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    deviceSaveMenu->draw(panel, &ui::txtCont, true);
}

static void bcatSavePanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    bcatSaveMenu->draw(panel, &ui::txtCont, true);
}

static void cacheSavePanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    cacheSaveMenu->draw(panel, &ui::txtCont, true);
}

static void usrOptDumpAllUserSaves(void *a)
{
    ui::newThread(fs::dumpAllUserSaves, NULL, fs::fileDrawFunc);
}

static void createSaveData(void *a)
{
    data::user *u = data::getCurrentUser();
    u128 uid = u->getUID128();

    //Device, BCAT, and Cache are hardcoded user IDs in JKSV's data
    uint64_t tid = 0;
    switch(uid)
    {
        case 0://This is system
            break;

        case 2:
            tid = bcatSids[bcatSaveMenu->getSelected()];
            fs::createSaveDataThreaded(FsSaveDataType_Bcat, tid, util::u128ToAccountUID(0));
            break;

        case 3:
            tid = devSids[deviceSaveMenu->getSelected()];
            fs::createSaveDataThreaded(FsSaveDataType_Device, tid, util::u128ToAccountUID(0));
            break;

        case 5:
            tid = cacheSids[cacheSaveMenu->getSelected()];
            fs::createSaveDataThreaded(FsSaveDataType_Cache, tid, util::u128ToAccountUID(0));
            break;

        default:
            tid = accSids[saveCreateMenu->getSelected()];
            fs::createSaveDataThreaded(FsSaveDataType_Account, tid, u->getUID());
            break;
    }
}

static void usrOptCreateAllSaves_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    data::user *u = data::getCurrentUser();
    int devPos = ui::usrMenu->getOptPos(ui::getUICString("saveTypeMainMenu", 0));
    int bcatPos = ui::usrMenu->getOptPos(ui::getUICString("saveTypeMainMenu", 1));
    int sel = ui::usrMenu->getSelected();
    if(sel < devPos)
    {
        AccountUid uid = u->getUID();
        for(unsigned i = 0; i < accSids.size(); i++)
            fs::createSaveData(FsSaveDataType_Account, accSids[i], uid, t);
    }
    else if(sel == devPos)
    {
        for(unsigned i = 0; i < devSids.size(); i++)
            fs::createSaveData(FsSaveDataType_Device, devSids[i], util::u128ToAccountUID(0), t);
    }
    else if(sel == bcatPos)
    {
        for(unsigned i = 0; i < bcatSids.size(); i++)
            fs::createSaveData(FsSaveDataType_Bcat, bcatSids[i], util::u128ToAccountUID(0), t);
    }
    t->finished = true;
}

static void usrOptCreateAllSaves(void *a)
{
    data::user *u = data::getCurrentUser();
    ui::confirmArgs *conf = ui::confirmArgsCreate(true, usrOptCreateAllSaves_t, NULL, NULL, ui::getUICString("confirmCreateAllSaveData", 0), u->getUsername().c_str());
    ui::confirm(conf);
}

//Sets up save create menus
static void initSaveCreateMenus()
{
    saveCreateMenu->reset();
    deviceSaveMenu->reset();
    bcatSaveMenu->reset();
    cacheSaveMenu->reset();

    accSids.clear();
    devSids.clear();
    bcatSids.clear();
    cacheSids.clear();

    //Group into vectors to match
    for(auto& t : data::titles)
    {
        NacpStruct *nacp = &t.second.nacp;

        if(nacp->user_account_save_data_size > 0)
            accSids.push_back(t.first);

        if(nacp->device_save_data_size > 0)
            devSids.push_back(t.first);

        if(nacp->bcat_delivery_cache_storage_size > 0)
            bcatSids.push_back(t.first);

        if(nacp->cache_storage_size > 0 || nacp->cache_storage_journal_size > 0 || nacp->cache_storage_data_and_journal_size_max > 0)
            cacheSids.push_back(t.first);
    }

    //Sort them alphabetically
    std::sort(accSids.begin(), accSids.end(), sortCreateTIDs);
    std::sort(devSids.begin(), devSids.end(), sortCreateTIDs);
    std::sort(bcatSids.begin(), bcatSids.end(), sortCreateTIDs);
    std::sort(cacheSids.begin(), cacheSids.end(), sortCreateTIDs);

    for(unsigned i = 0; i < accSids.size(); i++)
    {
        saveCreateMenu->addOpt(NULL, data::getTitleNameByTID(accSids[i]));
        saveCreateMenu->optAddButtonEvent(i, HidNpadButton_A, createSaveData, NULL);
    }

    for(unsigned i = 0; i < devSids.size(); i++)
    {
        deviceSaveMenu->addOpt(NULL, data::getTitleNameByTID(devSids[i]));
        deviceSaveMenu->optAddButtonEvent(i, HidNpadButton_A, createSaveData, NULL);
    }

    for(unsigned i = 0; i < bcatSids.size(); i++)
    {
        bcatSaveMenu->addOpt(NULL, data::getTitleNameByTID(bcatSids[i]));
        bcatSaveMenu->optAddButtonEvent(i, HidNpadButton_A, createSaveData, NULL);
    }

    for(unsigned i = 0; i < cacheSids.size(); i++)
    {
        cacheSaveMenu->addOpt(NULL, data::getTitleNameByTID(cacheSids[i]));
        cacheSaveMenu->optAddButtonEvent(i, HidNpadButton_A, createSaveData, NULL);
    }
}

void ui::usrInit()
{
    usrMenu = new ui::menu(50, 16, 0, 112, 1);
    usrOptMenu = new ui::menu(8, 32, 390, 20, 6);
    saveCreateMenu = new ui::menu(8, 32, 492, 20, 6);
    deviceSaveMenu = new ui::menu(8, 32, 492, 20, 6);
    bcatSaveMenu = new ui::menu(8, 32, 492, 20, 6);
    cacheSaveMenu = new ui::menu(8, 32, 492, 20, 6);

    usrOptMenu->setCallback(usrOptCallback, NULL);

    saveCreateMenu->setActive(false);
    saveCreateMenu->setCallback(saveCreateCallback, NULL);

    deviceSaveMenu->setActive(false);
    deviceSaveMenu->setCallback(saveCreateCallback, NULL);

    bcatSaveMenu->setActive(false);
    bcatSaveMenu->setCallback(saveCreateCallback, NULL);

    cacheSaveMenu->setActive(false);
    cacheSaveMenu->setCallback(saveCreateCallback, NULL);

    for(data::user u : data::users)
    {
        int usrPos = usrMenu->addOpt(u.getUserIcon(), u.getUsername());
        usrMenu->optAddButtonEvent(usrPos, HidNpadButton_A, toTTL, NULL);
    }

    sett = util::createIconGeneric(settText, 48, false);
    int pos = usrMenu->addOpt(sett, settText);
    usrMenu->optAddButtonEvent(pos, HidNpadButton_A, toOPT, NULL);

    ext = util::createIconGeneric(extText, 48, false);
    pos = usrMenu->addOpt(ext, extText);
    usrMenu->optAddButtonEvent(pos, HidNpadButton_A, toEXT, NULL);

    usrMenu->setOnChangeFunc(onMainChange);
    usrMenu->editParam(MENU_RECT_WIDTH, 142);
    usrMenu->editParam(MENU_RECT_HEIGHT, 130);

    usrSelPanel = new ui::slideOutPanel(200, 559, 89, ui::SLD_LEFT, _usrSelPanelDraw);
    usrSelPanel->setX(0);
    ui::registerPanel(usrSelPanel);
    usrSelPanel->openPanel();

    usrOptPanel = new ui::slideOutPanel(410, 720, 0, ui::SLD_RIGHT, usrOptPanelDraw);
    ui::registerPanel(usrOptPanel);

    for(int i = 0; i < 4; i++)
        usrOptMenu->addOpt(NULL, ui::getUIString("userOptions", i));

    //Dump All User Saves
    usrOptMenu->optAddButtonEvent(0, HidNpadButton_A, usrOptDumpAllUserSaves, NULL);
    //Create Save Data
    usrOptMenu->optAddButtonEvent(1, HidNpadButton_A, usrOptSaveCreate, usrMenu);
    //Create All
    usrOptMenu->optAddButtonEvent(2, HidNpadButton_A, usrOptCreateAllSaves, NULL);
    //Delete All
    usrOptMenu->optAddButtonEvent(3, HidNpadButton_A, usrOptDeleteAllUserSaves, NULL);
    usrOptMenu->setActive(false);

    saveCreatePanel = new ui::slideOutPanel(512, 720, 0, ui::SLD_RIGHT, saveCreatePanelDraw);
    ui::registerPanel(saveCreatePanel);

    deviceSavePanel = new ui::slideOutPanel(512, 720, 0, ui::SLD_RIGHT, deviceSavePanelDraw);
    ui::registerPanel(deviceSavePanel);

    bcatSavePanel = new ui::slideOutPanel(512, 720, 0, ui::SLD_RIGHT, bcatSavePanelDraw);
    ui::registerPanel(bcatSavePanel);

    cacheSavePanel = new ui::slideOutPanel(512, 720, 0, ui::SLD_RIGHT, cacheSavePanelDraw);
    ui::registerPanel(cacheSavePanel);

    initSaveCreateMenus();

    usrHelpX = 1220 - gfx::getTextWidth(ui::getUICString("helpUser", 0), 18);
}

void ui::usrExit()
{
    delete usrSelPanel;
    delete usrOptPanel;
    delete saveCreatePanel;
    delete deviceSavePanel;
    delete bcatSavePanel;
    delete cacheSavePanel;

    delete usrMenu;
    delete usrOptMenu;
    delete saveCreateMenu;
    delete deviceSaveMenu;
    delete bcatSaveMenu;
    delete cacheSaveMenu;
}

void ui::usrRefresh()
{
    initSaveCreateMenus();
}

void ui::usrUpdate()
{
    if(usrMenu->getActive())
    {
        switch(ui::padKeysDown())
        {
            case HidNpadButton_Y:
                ui::newThread(fs::dumpAllUsersAllSaves, NULL, fs::fileDrawFunc);
                break;

            case HidNpadButton_X:
                {
                    int cachePos = usrMenu->getOptPos(ui::getUIString("saveTypeMainMenu", 2));
                    if(usrMenu->getSelected() <= cachePos)
                    {
                        data::user *u = data::getCurrentUser();
                        usrOptMenu->editOpt(0, NULL, ui::getUIString("userOptions", 0) + u->getUsername());
                        usrOptMenu->setActive(true);
                        usrMenu->setActive(false);
                        usrOptPanel->openPanel();
                    }
                }
                break;
        }
    }
    usrMenu->update();
    usrOptMenu->update();
    saveCreateMenu->update();
    deviceSaveMenu->update();
    bcatSaveMenu->update();
    cacheSaveMenu->update();
}

void ui::usrDraw(SDL_Texture *target)
{
    if(ui::mstate == USR_SEL)
        gfx::drawTextf(NULL, 18, usrHelpX, 673, &ui::txtCont, ui::getUICString("helpUser", 0));
}
