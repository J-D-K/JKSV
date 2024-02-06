#include <switch.h>
#include <SDL2/SDL.h>

#include "ui.h"
#include "file.h"
#include "sett.h"
#include "cfg.h"
#include "util.h"

ui::menu *ui::settMenu;
static ui::slideOutPanel *blEditPanel;
static ui::menu *blEditMenu;

//This is the name of strings used here
static const char *settMenuStr = "settingsMenu";

static unsigned optHelpX = 0;

static inline std::string getBoolText(const bool& b)
{
    return b ? ui::getUIString("settingsOn", 0) : ui::getUIString("settingsOff", 0);
}

static inline void toggleBool(bool& b)
{
    if(b)
        b = false;
    else
        b = true;
}

//Declaration, implementation further down
static void blEditMenuPopulate();

static void settMenuCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            ui::usrMenu->setActive(true);
            ui::settMenu->setActive(false);
            ui::changeState(USR_SEL);
            break;

        case HidNpadButton_X:
            cfg::resetConfig();
            break;
    }
}

static void settMenuDeleteAllBackups_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(ui::getUICString("threadStatusDeletingFile", 0));

    fs::dirList *jksvDir = new fs::dirList(fs::getWorkDir());
    for(unsigned i = 0; i < jksvDir->getCount(); i++)
    {
        if(jksvDir->isDir(i) && jksvDir->getItem(i) != "svi")
        {
            std::string delTarget = fs::getWorkDir() + jksvDir->getItem(i) + "/";
            fs::delDir(delTarget);
        }
    }
    delete jksvDir;
    t->finished = true;
}

static void settMenuDeleteAllBackups(void *a)
{
    ui::confirmArgs *send = ui::confirmArgsCreate(true, settMenuDeleteAllBackups_t, NULL, NULL, ui::getUICString("confirmDeleteBackupsAll", 0));
    ui::confirm(send);
}

static void blEditDrawFunc(void *a)
{
    SDL_Texture *target = (SDL_Texture *)a;
    blEditMenu->draw(target, &ui::txtCont, true);
}

static void blEditMenuCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            ui::updateInput();
            blEditMenu->setActive(false);
            ui::settMenu->setActive(true);
            blEditPanel->closePanel();
            break;
    }
}

static void blEditMenuRemoveTitle(void *a)
{
    uint64_t remTID = cfg::blacklist[blEditMenu->getSelected()];
    cfg::removeTitleFromBlacklist(remTID);
    if(cfg::blacklist.size() > 0)
        blEditMenuPopulate();
    else
    {
        blEditMenu->setActive(false);
        ui::settMenu->setActive(true);
        blEditPanel->closePanel();
    }
}

static void blEditMenuPopulate()
{
    blEditMenu->reset();
    for(unsigned i = 0; i < cfg::blacklist.size(); i++)
    {
        blEditMenu->addOpt(NULL, data::getTitleNameByTID(cfg::blacklist[i]));
        blEditMenu->optAddButtonEvent(i, HidNpadButton_A, blEditMenuRemoveTitle, NULL);
    }
}

//Todo: this different
static void toggleOpt(void *a)
{
    switch(ui::settMenu->getSelected())
    {
        case 0:
            fs::delDir(fs::getWorkDir() + "_TRASH_/");
            mkdir(std::string(fs::getWorkDir() + "_TRASH_").c_str(), 777);
            ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popTrashEmptied", 0));
            break;

        case 1:
            ui::newThread(util::checkForUpdate, NULL, NULL);
            break;

        case 2:
            {
                std::string oldWD = fs::getWorkDir();
                std::string getWD = util::getStringInput(SwkbdType_QWERTY, fs::getWorkDir(), ui::getUIString("swkbdSetWorkDir", 0), 64, 0, NULL);
                if(!getWD.empty())
                {
                    if(getWD[getWD.length() - 1] != '/')
                        getWD += "/";

                    rename(oldWD.c_str(), getWD.c_str());
                    fs::setWorkDir(getWD);
                }
            }
            break;

        case 3:
            if(cfg::blacklist.size() > 0)
            {
               blEditMenuPopulate();
               ui::settMenu->setActive(false);
               blEditMenu->setActive(true);
               blEditPanel->openPanel();
            }
            break;

        case 4:
            settMenuDeleteAllBackups(NULL);
            break;

        case 5:
            toggleBool(cfg::config["incDev"]);
            break;

        case 6:
            toggleBool(cfg::config["autoBack"]);
            break;

        case 7:
            toggleBool(cfg::config["autoName"]);
            break;

        case 8:
            toggleBool(cfg::config["ovrClk"]);
            break;

        case 9:
            toggleBool(cfg::config["holdDel"]);
            break;

        case 10:
            toggleBool(cfg::config["holdRest"]);
            break;

        case 11:
            toggleBool(cfg::config["holdOver"]);
            break;

        case 12:
            toggleBool(cfg::config["forceMount"]);
            break;

        case 13:
            toggleBool(cfg::config["accSysSave"]);
            break;

        case 14:
            toggleBool(cfg::config["sysSaveWrite"]);
            break;

        case 15:
            toggleBool(cfg::config["directFsCmd"]);
            break;

        case 16:
            toggleBool(cfg::config["zip"]);
            break;

        case 17:
            toggleBool(cfg::config["langOverride"]);
            break;

        case 18:
            toggleBool(cfg::config["trashBin"]);
            break;

        case 19:
            if(++cfg::sortType > 2)
                cfg::sortType = 0;
            data::loadUsersTitles(false);
            ui::ttlRefresh();
            break;

        case 20:
            ui::animScale += 0.5f;
            if(ui::animScale > 8)
                ui::animScale = 1;
            break;

        case 21:
            toggleBool(cfg::config["autoUpload"]);
            break;
    }
}

static void updateMenuText()
{
    ui::settMenu->editOpt(5, NULL, ui::getUIString(settMenuStr, 5) + getBoolText(cfg::config["incDev"]));
    ui::settMenu->editOpt(6, NULL, ui::getUIString(settMenuStr, 6) + getBoolText(cfg::config["autoBack"]));
    ui::settMenu->editOpt(7, NULL, ui::getUIString(settMenuStr, 7) + getBoolText(cfg::config["autoName"]));
    ui::settMenu->editOpt(8, NULL, ui::getUIString(settMenuStr, 8) + getBoolText(cfg::config["ovrClk"]));
    ui::settMenu->editOpt(9, NULL, ui::getUIString(settMenuStr, 9) + getBoolText(cfg::config["holdDel"]));
    ui::settMenu->editOpt(10, NULL, ui::getUIString(settMenuStr, 10) + getBoolText(cfg::config["holdRest"]));
    ui::settMenu->editOpt(11, NULL, ui::getUIString(settMenuStr, 11) + getBoolText(cfg::config["holdOver"]));
    ui::settMenu->editOpt(12, NULL, ui::getUIString(settMenuStr, 12) + getBoolText(cfg::config["forceMount"]));
    ui::settMenu->editOpt(13, NULL, ui::getUIString(settMenuStr, 13) + getBoolText(cfg::config["accSysSave"]));
    ui::settMenu->editOpt(14, NULL, ui::getUIString(settMenuStr, 14) + getBoolText(cfg::config["sysSaveWrite"]));
    ui::settMenu->editOpt(15, NULL, ui::getUIString(settMenuStr, 15) + getBoolText(cfg::config["directFsCmd"]));
    ui::settMenu->editOpt(16, NULL, ui::getUIString(settMenuStr, 16) + getBoolText(cfg::config["zip"]));
    ui::settMenu->editOpt(17, NULL, ui::getUIString(settMenuStr, 17) + getBoolText(cfg::config["langOverride"]));
    ui::settMenu->editOpt(18, NULL, ui::getUIString(settMenuStr, 18) + getBoolText(cfg::config["trashBin"]));
    ui::settMenu->editOpt(19, NULL, ui::getUIString(settMenuStr, 19) + ui::getUICString("sortType", cfg::sortType));

    char tmp[16];
    sprintf(tmp, "%.1f", ui::animScale);
    ui::settMenu->editOpt(20, NULL, ui::getUIString(settMenuStr, 20) + std::string(tmp));
    ui::settMenu->editOpt(21, NULL, ui::getUIString(settMenuStr, 21) + getBoolText(cfg::config["autoUpload"]));
}

void ui::settInit()
{
    ui::settMenu = new ui::menu(200, 24, 1002, 24, 4);
    ui::settMenu->setCallback(settMenuCallback, NULL);
    ui::settMenu->setActive(false);

    blEditPanel = new ui::slideOutPanel(512, 720, 0, ui::SLD_RIGHT, blEditDrawFunc);
    blEditMenu = new ui::menu(8, 32, 492, 20, 6);
    blEditMenu->setCallback(blEditMenuCallback, NULL);
    blEditMenu->setActive(false);
    ui::registerPanel(blEditPanel);

    optHelpX = 1220 - gfx::getTextWidth(ui::getUICString("helpSettings", 0), 18);

    for(unsigned i = 0; i < 22; i++)
    {
        ui::settMenu->addOpt(NULL, ui::getUIString("settingsMenu", i));
        ui::settMenu->optAddButtonEvent(i, HidNpadButton_A, toggleOpt, NULL);
    }
}

void ui::settExit()
{
    delete ui::settMenu;
    delete blEditMenu;
    delete blEditPanel;
}

void ui::settUpdate()
{
    blEditMenu->update();
    ui::settMenu->update();
}

void ui::settDraw(SDL_Texture *target)
{
    updateMenuText();
    ui::settMenu->draw(target, &ui::txtCont, true);
    if(ui::mstate == OPT_MNU)
        gfx::drawTextf(NULL, 18, optHelpX, 673, &ui::txtCont, ui::getUICString("helpSettings", 0));
}
