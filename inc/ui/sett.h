#pragma once

#include <SDL2/SDL.h>

namespace ui
{
    void settInit();
    void settExit();
    void settUpdate();
    void settDraw(SDL_Texture *target);

    extern ui::menu *settMenu;
}
