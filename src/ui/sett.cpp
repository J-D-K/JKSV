#include <switch.h>
#include <SDL.h>

#include "ui.h"
#include "sett.h"

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
            toggleBool(data::config["incDev"]);
            break;

        case 1:
            toggleBool(data::config["autoBack"]);
            break;

        case 2:
            toggleBool(data::config["ovrClk"]);
            break;

        case 3:
            toggleBool(data::config["holdDel"]);
            break;

        case 4:
            toggleBool(data::config["holdRest"]);
            break;

        case 5:
            toggleBool(data::config["holdOver"]);
            break;

        case 6:
            toggleBool(data::config["forceMount"]);
            break;

        case 7:
            toggleBool(data::config["accSysSave"]);
            break;

        case 8:
            toggleBool(data::config["sysSaveWrite"]);
            break;

        case 9:
            toggleBool(data::config["directFsCmd"]);
            break;

        case 10:
            toggleBool(data::config["zip"]);
            break;

        case 11:
            toggleBool(data::config["langOverride"]);
            break;

        case 12:
            if(++data::sortType > 2)
                data::sortType = 0;
            data::loadUsersTitles(false);
            ui::refreshAllViews();
            break;
    }
}

static void updateMenuText()
{
    ui::settMenu->editOpt(0, NULL, ui::optMenuStr[0] + getBoolText(data::config["incDev"]));
    ui::settMenu->editOpt(1, NULL, ui::optMenuStr[1] + getBoolText(data::config["autoBack"]));
    ui::settMenu->editOpt(2, NULL, ui::optMenuStr[2] + getBoolText(data::config["ovrClk"]));
    ui::settMenu->editOpt(3, NULL, ui::optMenuStr[3] + getBoolText(data::config["holdDel"]));
    ui::settMenu->editOpt(4, NULL, ui::optMenuStr[4] + getBoolText(data::config["holdRest"]));
    ui::settMenu->editOpt(5, NULL, ui::optMenuStr[5] + getBoolText(data::config["holdOver"]));
    ui::settMenu->editOpt(6, NULL, ui::optMenuStr[6] + getBoolText(data::config["forceMount"]));
    ui::settMenu->editOpt(7, NULL, ui::optMenuStr[7] + getBoolText(data::config["accSysSave"]));
    ui::settMenu->editOpt(8, NULL, ui::optMenuStr[8] + getBoolText(data::config["sysSaveWrite"]));
    ui::settMenu->editOpt(9, NULL, ui::optMenuStr[9] + getBoolText(data::config["directFsCmd"]));
    ui::settMenu->editOpt(10, NULL, ui::optMenuStr[10] + getBoolText(data::config["zip"]));
    ui::settMenu->editOpt(11, NULL, ui::optMenuStr[11] + getBoolText(data::config["langOverride"]));
    ui::settMenu->editOpt(12, NULL, ui::optMenuStr[12] + ui::sortString[data::sortType]);
}

void ui::settInit()
{
    ui::settMenu = new ui::menu;
    ui::settMenu->setParams(32, 32, 1016, 20, 8);
    ui::settMenu->setCallback(settMenuCallback, NULL);
    ui::settMenu->setActive(false);

    optHelpX = 1220 - gfx::getTextWidth(ui::optHelp.c_str(), 18);

    for(unsigned i = 0; i < 13; i++)
    {
        ui::settMenu->addOpt(NULL, ui::optMenuStr[i]);
        ui::settMenu->setOptFunc(i, FUNC_A, toggleOpt, NULL);
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
