#include <switch.h>
#include <SDL.h>

#include "ui.h"
#include "file.h"
#include "sett.h"
#include "cfg.h"
#include "util.h"

ui::menu *ui::settMenu;

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

//Todo: this different
static void toggleOpt(void *a)
{
    switch(ui::settMenu->getSelected())
    {
        case 0:
            fs::delDir(fs::getWorkDir() + "_TRASH_/");
            mkdir(std::string(fs::getWorkDir() + "_TRASH_").c_str(), 777);
            ui::showPopMessage(POP_FRAME_DEFAULT, "Trash emptied.");
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
            toggleBool(cfg::config["incDev"]);
            break;

        case 4:
            toggleBool(cfg::config["autoBack"]);
            break;

        case 5:
            toggleBool(cfg::config["ovrClk"]);
            break;

        case 6:
            toggleBool(cfg::config["holdDel"]);
            break;

        case 7:
            toggleBool(cfg::config["holdRest"]);
            break;

        case 8:
            toggleBool(cfg::config["holdOver"]);
            break;

        case 9:
            toggleBool(cfg::config["forceMount"]);
            break;

        case 10:
            toggleBool(cfg::config["accSysSave"]);
            break;

        case 11:
            toggleBool(cfg::config["sysSaveWrite"]);
            break;

        case 12:
            toggleBool(cfg::config["directFsCmd"]);
            break;

        case 13:
            toggleBool(cfg::config["zip"]);
            break;

        case 14:
            toggleBool(cfg::config["langOverride"]);
            break;

        case 15:
            toggleBool(cfg::config["trashBin"]);
            break;

        case 16:
            if(++cfg::sortType > 2)
                cfg::sortType = 0;
            data::loadUsersTitles(false);
            ui::ttlRefresh();
            break;

        case 17:
            ui::animScale += 0.5f;
            if(ui::animScale > 8)
                ui::animScale = 1;
            break;
    }
}

static void updateMenuText()
{
    ui::settMenu->editOpt(3, NULL, ui::getUIString(settMenuStr, 3) + getBoolText(cfg::config["incDev"]));
    ui::settMenu->editOpt(4, NULL, ui::getUIString(settMenuStr, 4) + getBoolText(cfg::config["autoBack"]));
    ui::settMenu->editOpt(5, NULL, ui::getUIString(settMenuStr, 5) + getBoolText(cfg::config["ovrClk"]));
    ui::settMenu->editOpt(6, NULL, ui::getUIString(settMenuStr, 6) + getBoolText(cfg::config["holdDel"]));
    ui::settMenu->editOpt(7, NULL, ui::getUIString(settMenuStr, 7) + getBoolText(cfg::config["holdRest"]));
    ui::settMenu->editOpt(8, NULL, ui::getUIString(settMenuStr, 8) + getBoolText(cfg::config["holdOver"]));
    ui::settMenu->editOpt(9, NULL, ui::getUIString(settMenuStr, 9) + getBoolText(cfg::config["forceMount"]));
    ui::settMenu->editOpt(10, NULL, ui::getUIString(settMenuStr, 10) + getBoolText(cfg::config["accSysSave"]));
    ui::settMenu->editOpt(11, NULL, ui::getUIString(settMenuStr, 11) + getBoolText(cfg::config["sysSaveWrite"]));
    ui::settMenu->editOpt(12, NULL, ui::getUIString(settMenuStr, 12) + getBoolText(cfg::config["directFsCmd"]));
    ui::settMenu->editOpt(13, NULL, ui::getUIString(settMenuStr, 13) + getBoolText(cfg::config["zip"]));
    ui::settMenu->editOpt(14, NULL, ui::getUIString(settMenuStr, 14) + getBoolText(cfg::config["langOverride"]));
    ui::settMenu->editOpt(15, NULL, ui::getUIString(settMenuStr, 15) + getBoolText(cfg::config["trashBin"]));
    ui::settMenu->editOpt(16, NULL, ui::getUIString(settMenuStr, 16) + ui::getUICString("sortType", cfg::sortType));

    char tmp[16];
    sprintf(tmp, "%.1f", ui::animScale);
    ui::settMenu->editOpt(17, NULL, ui::getUIString(settMenuStr, 17) + std::string(tmp));
}

void ui::settInit()
{
    ui::settMenu = new ui::menu(200, 24, 1002, 24, 4);
    ui::settMenu->setCallback(settMenuCallback, NULL);
    ui::settMenu->setActive(false);

    optHelpX = 1220 - gfx::getTextWidth(ui::getUICString("helpSettings", 0), 18);

    for(unsigned i = 0; i < 18; i++)
    {
        ui::settMenu->addOpt(NULL, ui::getUIString("settingsMenu", i));
        ui::settMenu->optAddButtonEvent(i, HidNpadButton_A, toggleOpt, NULL);
    }
}

void ui::settExit()
{
    delete ui::settMenu;
}

void ui::settUpdate()
{
    ui::settMenu->update();
}

void ui::settDraw(SDL_Texture *target)
{
    updateMenuText();
    ui::settMenu->draw(target, &ui::txtCont, true);
    if(ui::mstate == OPT_MNU)
        gfx::drawTextf(NULL, 18, optHelpX, 673, &ui::txtCont, ui::getUICString("helpSettings", 0));
}
