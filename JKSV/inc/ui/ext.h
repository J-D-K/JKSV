#pragma once

namespace ui
{
    class menu;
    extern ui::menu *extMenu;
    void extInit();
    void extExit();
    void extUpdate();
    void extDraw(SDL_Texture *target);
} // namespace ui
