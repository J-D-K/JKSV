#pragma once

namespace ui
{
    void ttlInit();
    void ttlExit();
    void ttlSetActive(int usr, bool _set, bool _showSel);
    void ttlRefresh();

    //JIC for func ptr
    void ttlReset();
    void ttlUpdate();
    void ttlDraw(SDL_Texture *target);

    //File mode needs access to this.
    extern ui::slideOutPanel *ttlOptsPanel;
}
