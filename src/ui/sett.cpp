#include <switch.h>
#include <SDL.h>

#include "ui.h"
#include "file.h"
#include "sett.h"
#include "util.h"

ui::menu *ui::settMenu;

static unsigned optHelpX = 0;

static inline std::string getBoolText(const bool& b)
{
    return b ? ui::on : ui::off;
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
            data::restoreDefaultConfig();
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
            toggleBool(data::config["incDev"]);
            break;

        case 3:
            toggleBool(data::config["autoBack"]);
            break;

        case 4:
            toggleBool(data::config["ovrClk"]);
            break;

        case 5:
            toggleBool(data::config["holdDel"]);
            break;

        case 6:
            toggleBool(data::config["holdRest"]);
            break;

        case 7:
            toggleBool(data::config["holdOver"]);
            break;

        case 8:
            toggleBool(data::config["forceMount"]);
            break;

        case 9:
            toggleBool(data::config["accSysSave"]);
            break;

        case 10:
            toggleBool(data::config["sysSaveWrite"]);
            break;

        case 11:
            toggleBool(data::config["directFsCmd"]);
            break;

        case 12:
            toggleBool(data::config["zip"]);
            break;

        case 13:
            toggleBool(data::config["langOverride"]);
            break;

        case 14:
            toggleBool(data::config["trashBin"]);
            break;

        case 15:
            if(++data::sortType > 2)
                data::sortType = 0;
            data::loadUsersTitles(false);
            ui::refreshAllViews();
            break;

        case 16:
            ui::animScale += 0.5f;
            if(ui::animScale > 8)
                ui::animScale = 1;
            break;
    }
}

static void updateMenuText()
{
    ui::settMenu->editOpt(2, NULL, ui::optMenuStr[2] + getBoolText(data::config["incDev"]));
    ui::settMenu->editOpt(3, NULL, ui::optMenuStr[3] + getBoolText(data::config["autoBack"]));
    ui::settMenu->editOpt(4, NULL, ui::optMenuStr[4] + getBoolText(data::config["ovrClk"]));
    ui::settMenu->editOpt(5, NULL, ui::optMenuStr[5] + getBoolText(data::config["holdDel"]));
    ui::settMenu->editOpt(6, NULL, ui::optMenuStr[6] + getBoolText(data::config["holdRest"]));
    ui::settMenu->editOpt(7, NULL, ui::optMenuStr[7] + getBoolText(data::config["holdOver"]));
    ui::settMenu->editOpt(8, NULL, ui::optMenuStr[8] + getBoolText(data::config["forceMount"]));
    ui::settMenu->editOpt(9, NULL, ui::optMenuStr[9] + getBoolText(data::config["accSysSave"]));
    ui::settMenu->editOpt(10, NULL, ui::optMenuStr[10] + getBoolText(data::config["sysSaveWrite"]));
    ui::settMenu->editOpt(11, NULL, ui::optMenuStr[11] + getBoolText(data::config["directFsCmd"]));
    ui::settMenu->editOpt(12, NULL, ui::optMenuStr[12] + getBoolText(data::config["zip"]));
    ui::settMenu->editOpt(13, NULL, ui::optMenuStr[13] + getBoolText(data::config["langOverride"]));
    ui::settMenu->editOpt(14, NULL, ui::optMenuStr[14] + getBoolText(data::config["trashBin"]));
    ui::settMenu->editOpt(15, NULL, ui::optMenuStr[15] + ui::sortString[data::sortType]);

    char tmp[16];
    sprintf(tmp, "%.1f", ui::animScale);
    ui::settMenu->editOpt(16, NULL, ui::optMenuStr[16] + std::string(tmp));
}

void ui::settInit()
{
    ui::settMenu = new ui::menu;
    ui::settMenu->setParams(200, 24, 1002, 24, 4);
    ui::settMenu->setCallback(settMenuCallback, NULL);
    ui::settMenu->setActive(false);

    optHelpX = 1220 - gfx::getTextWidth(ui::optHelp.c_str(), 18);

    for(unsigned i = 0; i < 17; i++)
    {
        ui::settMenu->addOpt(NULL, ui::optMenuStr[i]);
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
        gfx::drawTextf(NULL, 18, optHelpX, 673, &ui::txtCont, ui::optHelp.c_str());
}
