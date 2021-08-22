#pragma once

namespace ui
{
    extern ui::menu *extMenu;
    void extInit();
    void extExit();
    void extUpdate();
    void extDraw(SDL_Texture *target);
}
