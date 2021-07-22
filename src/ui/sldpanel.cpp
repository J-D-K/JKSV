#include <SDL.h>

#include "ui.h"
#include "gfx.h"

ui::slideOutPanel::slideOutPanel(int _w, int _h, int _y, funcPtr _draw)
{
    w = _w;
    h = _h;
    y = _y;
    drawFunc = _draw;
    panel = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_SetTextureBlendMode(panel, SDL_BLENDMODE_BLEND);
}

ui::slideOutPanel::~slideOutPanel()
{
    SDL_DestroyTexture(panel);
}

void ui::slideOutPanel::update()
{
    if(open && callback)
        (*callback)(cbArgs);
}

void ui::slideOutPanel::draw(const SDL_Color *backCol)
{
    gfx::clearTarget(panel, backCol);

    if(open && x > 1280 - w)
    {
        float add = ((1280 - (float)w) - (float)x) / ui::animScale;
        x += ceil(add);
    }
    else if(!open && x < 1280)
    {
        float add = (1280 - (float)x) / ui::animScale;
        x += ceil(add);
    }

    //don't waste time drawing if you can't even see it.
    if(x < 1280)
    {
        (*drawFunc)(panel);
        gfx::texDraw(NULL, panel, x, y);
    }
}
