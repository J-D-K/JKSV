#pragma once

#include <SDL.h>

namespace ui
{
    void settInit();
    void settExit();
    void settUpdate();
    void settDraw(SDL_Texture *target);
}
