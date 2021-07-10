#pragma once

namespace ui
{
    void usrInit();
    void usrExit();
    void usrMenuSetActive(bool _set);
    void usrUpdate();
    void usrDraw(SDL_Texture *target);
}
