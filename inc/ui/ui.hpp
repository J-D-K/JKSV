#pragma once
#include <SDL2/SDL.h>
#include "ui/strings.hpp"
#include "ui/menu.hpp"
#include "ui/iconMenu.hpp"
#include "ui/slidePanel.hpp"
#include "ui/titleSelection.hpp"
#include "ui/popMessage.hpp"

namespace ui
{
    // Loads textures to render various parts of UI
    void init(void);

    // Considering moving these to a different place. Not sure where they really belong
    void renderSelectionBox(SDL_Texture *target, int x, int y, int width, int height, uint8_t colorMod);
    void renderDialogBox(SDL_Texture *target, int x, int y, int width, int height);
    void renderLoadingGlyph(void);
}
