#include <SDL2/SDL.h>

#include "ui.h"
#include "gfx.h"

ui::slideOutPanel::slideOutPanel(int _w, int _h, int _y, ui::slidePanelOrientation _side, funcPtr _draw)
{
    w = _w;
    h = _h;
    y = _y;
    sldSide = _side;
    if(_side == ui::SLD_LEFT)
        x = -w;
    else
        x = 1280;

    drawFunc = _draw;
    panel = gfx::texMgr->textureCreate(w, h);
}

void ui::slideOutPanel::resizePanel(int _w, int _h, int _y)
{
    w = _w;
    h = _h;
    y = _y;
    if(sldSide == ui::SLD_LEFT)
        x = -w;

    gfx::texMgr->textureResize(&panel, w, h);
}

void ui::slideOutPanel::update()
{
    if(open && callback)
        (*callback)(cbArgs);
}

void ui::slideOutPanel::draw(const SDL_Color *backCol)
{
    gfx::clearTarget(panel, backCol);

    if(open && sldSide == ui::SLD_LEFT && x < 0)
    {
        float add = (float)x / ui::animScale;
        x -= ceil(add);
    }
    else if(open && sldSide == ui::SLD_RIGHT && x > 1280 - w)
    {
        float add = ((1280.0f - (float)w) - (float)x) / ui::animScale;
        x += ceil(add);
    }
    else if(!open && sldSide == ui::SLD_LEFT && x > -w)
    {
        float sub = ((float)w - (float)x) / ui::animScale;
        x -= ceil(sub);
    }
    else if(!open && sldSide == ui::SLD_RIGHT && x < 1280)
    {
        float add = (1280.0f - (float)x) / ui::animScale;
        x += ceil(add);
    }

    if((sldSide == ui::SLD_LEFT && x > -w) || (sldSide == ui::SLD_RIGHT && x < 1280))
    {
        (*drawFunc)(panel);
        gfx::texDraw(NULL, panel, x, y);
    }
}
