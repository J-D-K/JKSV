#pragma once

namespace ui
{
    void usrInit();
    void usrExit();
    void usrUpdate();
    void usrDraw(SDL_Texture *target);

    //A lot of stuff needs access to this
    extern ui::menu *usrMenu;
}
