#include <switch.h>
#include <SDL.h>

#include "ui.h"
#include "sett.h"

static ui::menu *settMenu;

void ui::settInit()
{
    settMenu = new ui::menu;
    settMenu->setParams(32, 32, 710, 24, 8);
    settMenu->setActive(false);

    for(unsigned i = 0; i < 15; i++)
        settMenu->addOpt(NULL, ui::optMenuStr[i]);
}

void ui::settExit()
{
    delete settMenu;
}

void ui::settUpdate()
{
    settMenu->update();
}

void ui::settDraw(SDL_Texture *target)
{
    settMenu->draw(target, &ui::txtCont, true);
}
