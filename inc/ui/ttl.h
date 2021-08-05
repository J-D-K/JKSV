#pragma once

namespace ui
{
    void ttlInit();
    void ttlExit();
    void ttlSetActive(int usr);
    void ttlRefresh();
    void populateFldMenu();

    //JIC for func ptr
    void ttlReset();
    void ttlUpdate();
    void ttlDraw(SDL_Texture *target);

    //File mode needs access to this.
    extern ui::slideOutPanel *ttlOptsPanel;
}
