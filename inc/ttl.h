#pragma once

namespace ui
{
    void ttlInit();
    void ttlExit();

    //JIC for func ptr
    void setupTiles(void *);
    void ttlReset();
    void ttlUpdate();
    void ttlDraw(SDL_Texture *target);
}
