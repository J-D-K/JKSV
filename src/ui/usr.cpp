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
static ui::menu *usrMenu, *usrOptMenu, *saveCreateMenu;
static ui::slideOutPanel *usrOptPanel, *saveCreatePanel;

//Icons for settings + extras
static SDL_Texture *sett, *ext;

//This stores save ids to match with saveCreateMenu
//Probably needs/should be changed
static std::vector<uint64_t> sids;

static unsigned optsPos = 0, extPos = 0, usrHelpX = 0;

static void onMainChange(void *a)
{
    if(usrMenu->getSelected() < (int)data::users.size())
        data::selUser = usrMenu->getSelected();
}

static void toOPT(void *a)
{
    ui::changeState(OPT_MNU);
    ui::usrMenuSetActive(false);
}

static void toEXT(void *a)
{
    ui::changeState(EX_MNU);
    ui::usrMenuSetActive(false);
}

static void toMAIN(void *a)
{
    if(ui::padKeysDown() & HidNpadButton_B)
    {
        ui::changeState(USR_SEL);
        ui::usrMenuSetActive(true);
    }
}

static void usrOptCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            usrOptPanel->closePanel();
            usrMenu->setActive(true);
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
            saveCreateMenu->setActive(false);
            saveCreatePanel->closePanel();
            usrOptPanel->openPanel();
            break;
    }
}

static void usrOptSaveCreate(void *a)
{
    ui::updateInput();
    usrOptMenu->setActive(false);
    saveCreateMenu->setActive(true);
    usrOptPanel->closePanel();
    saveCreatePanel->openPanel();
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

static void createSaveData(void *a)
{
    ui::menu *in = (ui::menu *)a;
    uint64_t sid = sids[in->getSelected()];
    data::titleInfo *create = data::getTitleInfoByTID(sid);

    FsSaveDataAttribute attr;
    memset(&attr, 0, sizeof(FsSaveDataAttribute));
    attr.application_id = sid;
    attr.uid = data::curUser.getUID();
    attr.system_save_data_id = 0;
    attr.save_data_type = FsSaveDataType_Account;
    attr.save_data_rank = 0;
    attr.save_data_index = 0;

    FsSaveDataCreationInfo svCreate;
    memset(&svCreate, 0, sizeof(FsSaveDataCreationInfo));
    svCreate.save_data_size = create->nacp.user_account_save_data_size;
    svCreate.journal_size = create->nacp.user_account_save_data_journal_size;
    svCreate.available_size = create->nacp.user_account_save_data_size;
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
        ui::showPopup(POP_FRAME_DEFAULT, ui::saveCreated.c_str(), create->title.c_str());
        data::loadUsersTitles(false);
        ui::refreshAllViews();
    }
    else
    {
        ui::showPopup(POP_FRAME_DEFAULT, ui::saveCreateFailed.c_str());
    }
}

void ui::usrInit()
{
    usrMenu = new ui::menu;
    usrOptMenu = new ui::menu;
    saveCreateMenu = new ui::menu;

    usrMenu->setParams(64, 16, 0, 96, 4);
    usrOptMenu->setParams(8, 32, 390, 18, 8);
    usrOptMenu->setCallback(usrOptCallback, NULL);

    saveCreateMenu->setParams(8, 32, 390, 18, 8);
    saveCreateMenu->setCallback(saveCreateCallback, NULL);

    for(data::user u : data::users)
    {
        usrMenu->addOpt(u.getUserIcon(), u.getUsername());
        unsigned ind = usrMenu->getOptPos(u.getUsername());
        usrMenu->setOptFunc(ind, FUNC_A, toTTL, NULL);
    }

    sett = util::createIconGeneric("Settings", 40);
    usrMenu->addOpt(sett, "Settings");
    optsPos = usrMenu->getOptPos("Settings");
    usrMenu->setOptFunc(optsPos, FUNC_A, toOPT, NULL);

    ext = util::createIconGeneric("Extras", 40);
    usrMenu->addOpt(ext, "Extras");
    extPos = usrMenu->getOptPos("Extras");
    usrMenu->setOptFunc(extPos, FUNC_A, toEXT, NULL);

    usrMenu->setOnChangeFunc(onMainChange);

    usrMenu->editParam(MENU_RECT_WIDTH, 126);

    usrOptPanel = new ui::slideOutPanel(410, 720, 0, usrOptPanelDraw);
    ui::registerPanel(usrOptPanel);
    usrOptMenu->addOpt(NULL, ui::usrOptString[0]);
    usrOptMenu->setOptFunc(0, FUNC_A, usrOptSaveCreate, NULL);
    usrOptMenu->setActive(false);

    saveCreatePanel = new ui::slideOutPanel(410, 720, 0, saveCreatePanelDraw);
    saveCreateMenu->setActive(false);
    ui::registerPanel(saveCreatePanel);
    unsigned i = 0;
    for(auto& t : data::titles)
    {
        if(t.second.nacp.user_account_save_data_size > 0)
        {
            saveCreateMenu->addOpt(NULL, t.second.title);
            saveCreateMenu->setOptFunc(i++, FUNC_A, createSaveData, saveCreateMenu);
            sids.push_back(t.first);
        }
    }
    usrHelpX = 1220 - gfx::getTextWidth(ui::userHelp.c_str(), 18);
}

void ui::usrExit()
{
    delete usrOptPanel;
    delete saveCreatePanel;
    delete usrMenu;
    delete usrOptMenu;
    delete saveCreateMenu;
    SDL_DestroyTexture(sett);
    SDL_DestroyTexture(ext);
}

void ui::usrMenuSetActive(bool _set)
{
    usrMenu->setActive(_set);
}

void ui::usrUpdate()
{
    usrMenu->update();
    usrOptMenu->update();
    saveCreateMenu->update();

    if(!usrOptMenu->getActive() && !saveCreateMenu->getActive())
    {
        switch(ui::padKeysDown())
        {
            case HidNpadButton_X:
                usrOptMenu->setActive(true);
                usrMenu->setActive(false);
                usrOptPanel->openPanel();
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
