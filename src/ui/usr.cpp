#include <string>
#include <vector>
#include <switch.h>

#include "file.h"
#include "data.h"
#include "ui.h"
#include "util.h"
#include "type.h"

#include "usr.h"
#include "ttl.h"

//Main menu/Users + options, folder
ui::menu *ui::usrMenu;

static ui::menu *usrOptMenu, *saveCreateMenu, *deviceSaveMenu, *bcatSaveMenu, *cacheSaveMenu;
//All save types have different entries.
static ui::slideOutPanel *usrOptPanel, *saveCreatePanel, *deviceSavePanel, *bcatSavePanel, *cacheSavePanel;

//Icons for settings + extras
static SDL_Texture *sett, *ext;

//Struct to send args to createFunc
typedef struct
{
    FsSaveDataType type;
    ui::menu *m;
} svCreateArgs;

static svCreateArgs accCreate, devCreate, bcatCreate, cacheCreate;

//This stores save ids to match with saveCreateMenu
//Probably needs/should be changed
static std::vector<uint64_t> accSids, devSids, bcatSids, cacheSids;

static unsigned usrHelpX = 0;

static void onMainChange(void *a)
{
    if(ui::usrMenu->getSelected() < (int)data::users.size())
        data::selUser = ui::usrMenu->getSelected();
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
    int devPos = m->getOptPos("Device");
    int bcatPos = m->getOptPos("BCAT");
    int cachePos = m->getOptPos("Cache");

    ui::updateInput();//Todo: Need to go through with fine tooth comb so this isn't needed
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
    data::user *u = &data::users[data::selUser];
    for(data::userTitleInfo& tinf : u->titleInfo)
    {
        t->status = "Deleting " + data::getTitleNameByTID(tinf.saveID);
        fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId_User, tinf.saveInfo.save_data_id);;
    }
    data::loadUsersTitles(false);
    ui::refreshAllViews();
    t->finished = true;
}

static void usrOptDeleteAllUserSaves(void *a)
{
    data::user *u = &data::users[data::selUser];
    if(ui::confirm(true, "*ARE YOU SURE YOU WANT TO DELETE ALL SAVE DATA FOR %s?*", u->getUsername().c_str()))
        ui::newThread(usrOptDeleteAllUserSaves_t, NULL);
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

static void createSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    svCreateArgs *in = (svCreateArgs *)t->argPtr;

    FsSaveDataType type = in->type;
    ui::menu *m = in->m;
    uint64_t sid;
    switch(type)
    {
        case FsSaveDataType_Account:
            sid = accSids[m->getSelected()];
            break;

        case FsSaveDataType_Device:
            sid = devSids[m->getSelected()];
            break;

        case FsSaveDataType_Bcat:
            sid = bcatSids[m->getSelected()];
            break;

        case FsSaveDataType_Cache:
            sid = cacheSids[m->getSelected()];
            break;

        default:
            return;
            break;
    }
    data::titleInfo *create = data::getTitleInfoByTID(sid);
    t->status = "Creating save data for " + create->title;

    FsSaveDataAttribute attr;
    memset(&attr, 0, sizeof(FsSaveDataAttribute));
    attr.application_id = sid;
    attr.uid = type == FsSaveDataType_Account ? data::curUser.getUID() : util::u128ToAccountUID(0);
    attr.system_save_data_id = 0;
    attr.save_data_type = type;
    attr.save_data_rank = 0;
    attr.save_data_index = 0;//Todo: Let user input this

    FsSaveDataCreationInfo svCreate;
    memset(&svCreate, 0, sizeof(FsSaveDataCreationInfo));
    int64_t saveSize = 0, journalSize = 0;
    switch(type)
    {
        case FsSaveDataType_Account:
            saveSize = create->nacp.user_account_save_data_size;
            journalSize = create->nacp.user_account_save_data_journal_size;
            break;

        case FsSaveDataType_Device:
            saveSize = create->nacp.device_save_data_size;
            journalSize = create->nacp.device_save_data_journal_size;
            break;

        case FsSaveDataType_Bcat:
            saveSize = create->nacp.bcat_delivery_cache_storage_size;
            journalSize = create->nacp.bcat_delivery_cache_storage_size;//This needs to be fixed
            break;

        case FsSaveDataType_Cache:
            saveSize = 32 * 1024 * 1024;//Todo: Add target folder/zip selection for size
            journalSize = create->nacp.cache_storage_journal_size;
            break;

        default:
            return;
            break;
    }
    svCreate.save_data_size = saveSize;
    svCreate.journal_size = journalSize;
    svCreate.available_size = 0x4000;
    svCreate.owner_id = create->nacp.save_data_owner_id;
    svCreate.flags = 0;
    svCreate.save_data_space_id = FsSaveDataSpaceId_User;

    FsSaveDataMetaInfo meta;
    memset(&meta, 0, sizeof(FsSaveDataMetaInfo));
    meta.size = 0x40060;
    meta.type = FsSaveDataMetaType_Thumbnail;

    Result res = 0;
    if(R_SUCCEEDED(res = fsCreateSaveDataFileSystem(&attr, &svCreate, &meta)))
    {
        data::loadUsersTitles(false);
        ui::refreshAllViews();
    }
    else
    {
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::saveCreateFailed.c_str());
        fs::logWrite("SaveCreate Failed -> %X\n", res);
    }

    t->finished = true;
}

static void createSaveData(void *a)
{
    ui::newThread(createSaveData_t, a);
}

void ui::usrInit()
{
    usrMenu = new ui::menu;
    usrOptMenu = new ui::menu;
    saveCreateMenu = new ui::menu;
    deviceSaveMenu = new ui::menu;
    bcatSaveMenu = new ui::menu;
    cacheSaveMenu = new ui::menu;

    usrMenu->setParams(64, 16, 0, 96, 4);
    usrOptMenu->setParams(8, 32, 390, 18, 8);
    usrOptMenu->setCallback(usrOptCallback, NULL);

    saveCreateMenu->setParams(8, 32, 390, 18, 8);
    saveCreateMenu->setActive(false);
    saveCreateMenu->setCallback(saveCreateCallback, NULL);

    deviceSaveMenu->setParams(8, 32, 390, 18, 8);
    deviceSaveMenu->setActive(false);
    deviceSaveMenu->setCallback(saveCreateCallback, NULL);

    bcatSaveMenu->setParams(8, 32, 390, 18, 8);
    bcatSaveMenu->setActive(false);
    bcatSaveMenu->setCallback(saveCreateCallback, NULL);

    cacheSaveMenu->setParams(8, 32, 390, 18, 8);
    cacheSaveMenu->setActive(false);
    cacheSaveMenu->setCallback(saveCreateCallback, NULL);

    for(data::user u : data::users)
    {
        int usrPos = usrMenu->addOpt(u.getUserIcon(), u.getUsername());
        usrMenu->optAddButtonEvent(usrPos, HidNpadButton_A, toTTL, NULL);
    }

    sett = util::createIconGeneric("Settings", 40);
    int pos = usrMenu->addOpt(sett, "Settings");
    usrMenu->optAddButtonEvent(pos, HidNpadButton_A, toOPT, NULL);

    ext = util::createIconGeneric("Extras", 40);
    pos = usrMenu->addOpt(ext, "Extras");
    usrMenu->optAddButtonEvent(pos, HidNpadButton_A, NULL, NULL);

    usrMenu->setOnChangeFunc(onMainChange);
    usrMenu->editParam(MENU_RECT_WIDTH, 126);

    usrOptPanel = new ui::slideOutPanel(410, 720, 0, usrOptPanelDraw);
    ui::registerPanel(usrOptPanel);

    usrOptMenu->addOpt(NULL, ui::usrOptString[0]);
    usrOptMenu->optAddButtonEvent(0, HidNpadButton_A, usrOptSaveCreate, usrMenu);
    usrOptMenu->addOpt(NULL, ui::usrOptString[1]);
    usrOptMenu->optAddButtonEvent(1, HidNpadButton_A, usrOptDeleteAllUserSaves, NULL);
    usrOptMenu->setActive(false);

    saveCreatePanel = new ui::slideOutPanel(410, 720, 0, saveCreatePanelDraw);
    ui::registerPanel(saveCreatePanel);

    deviceSavePanel = new ui::slideOutPanel(410, 720, 0, deviceSavePanelDraw);
    ui::registerPanel(deviceSavePanel);

    bcatSavePanel = new ui::slideOutPanel(410, 720, 0, bcatSavePanelDraw);
    ui::registerPanel(bcatSavePanel);

    cacheSavePanel = new ui::slideOutPanel(410, 720, 0, cacheSavePanelDraw);
    ui::registerPanel(cacheSavePanel);

    accCreate = {FsSaveDataType_Account, saveCreateMenu};
    devCreate = {FsSaveDataType_Device, deviceSaveMenu};
    bcatCreate = {FsSaveDataType_Bcat, bcatSaveMenu};
    cacheCreate = {FsSaveDataType_Cache, cacheSaveMenu};
    for(auto& t : data::titles)
    {
        NacpStruct *nacp = &t.second.nacp;

        if(nacp->user_account_save_data_size > 0)
        {
            int optPos = saveCreateMenu->addOpt(NULL, t.second.title);
            saveCreateMenu->optAddButtonEvent(optPos, HidNpadButton_A, createSaveData, &accCreate);
            accSids.push_back(t.first);
        }

        if(nacp->device_save_data_size > 0)
        {
            int optPos = deviceSaveMenu->addOpt(NULL,  t.second.title);
            deviceSaveMenu->optAddButtonEvent(optPos, HidNpadButton_A, createSaveData, &devCreate);
            devSids.push_back(t.first);
        }

        if(nacp->bcat_delivery_cache_storage_size > 0)
        {
            int optPos = bcatSaveMenu->addOpt(NULL, t.second.title);
            bcatSaveMenu->optAddButtonEvent(optPos, HidNpadButton_A, createSaveData, &bcatCreate);
            bcatSids.push_back(t.first);
        }

        if(nacp->cache_storage_size > 0)
        {
            int optPos = cacheSaveMenu->addOpt(NULL, t.second.title);
            cacheSaveMenu->optAddButtonEvent(optPos, HidNpadButton_A, createSaveData, &cacheCreate);
            cacheSids.push_back(t.first);
        }
    }
    usrHelpX = 1220 - gfx::getTextWidth(ui::userHelp.c_str(), 18);
}

void ui::usrExit()
{
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

    SDL_DestroyTexture(sett);
    SDL_DestroyTexture(ext);
}

void ui::usrUpdate()
{
    usrMenu->update();
    usrOptMenu->update();
    saveCreateMenu->update();
    deviceSaveMenu->update();
    bcatSaveMenu->update();
    cacheSaveMenu->update();

    //Todo: Not this
    if(!usrOptMenu->getActive() && !saveCreateMenu->getActive() && !deviceSaveMenu->getActive() && !bcatSaveMenu->getActive() && !cacheSaveMenu->getActive())
    {
        switch(ui::padKeysDown())
        {
            case HidNpadButton_X:
                {
                    int cachePos = usrMenu->getOptPos("Cache");
                    if(usrMenu->getSelected() <= cachePos)
                    {
                        usrOptMenu->setActive(true);
                        usrMenu->setActive(false);
                        usrOptPanel->openPanel();
                    }
                }
                break;
        }
    }
}

void ui::usrDraw(SDL_Texture *target)
{
    usrMenu->draw(target, &ui::txtCont, false);
    if(ui::mstate == USR_SEL)
        gfx::drawTextf(NULL, 18, usrHelpX, 673, &ui::txtCont, ui::userHelp.c_str());
}
