#pragma once

namespace ui
{
    void ttlInit();
    void ttlExit();
    void ttlSetActive(int usr);
    void refreshAllViews();
    void populateFldMenu();

    //JIC for func ptr
    void ttlReset();
    void ttlUpdate();
    void ttlDraw(SDL_Texture *target);
}
