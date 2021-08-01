#pragma once

namespace ui
{
    void usrInit();
    void usrExit();
    void usrUpdate();
    void usrDraw(SDL_Texture *target);

    //A lot of stuff needs access to these
    extern ui::menu *usrMenu;
    extern ui::slideOutPanel *usrSelPanel;
}
