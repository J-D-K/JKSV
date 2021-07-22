#include "ui.h"
#include "ui/ttlview.h"

//Todo make less hardcoded
void ui::titleTile::draw(SDL_Texture *target, int x, int y, bool sel)
{
    unsigned xScale = w * 1.28, yScale = h * 1.28;
    if(sel)
    {
        if(wS < xScale)
            wS += 18;
        if(hS < yScale)
            hS += 18;
    }
    else
    {
        if(wS > w)
            wS -= 9;
        if(hS > h)
            hS -= 9;
    }

    int dX = x - ((wS - w) / 2);
    int dY = y - ((hS - h) / 2);
    gfx::texDrawStretch(target, icon, dX, dY, wS, hS);
    if(fav)
        gfx::drawTextf(target, 20, dX + 8, dY + 8, &ui::heartColor, "♥");
}

ui::titleview::titleview(const data::user& _u, int _iconW, int _iconH, int _horGap, int _vertGap, int _rowCount, funcPtr _callback)
{
    iconW = _iconW;
    iconH = _iconH;
    horGap = _horGap;
    vertGap = _vertGap;
    rowCount = _rowCount;
    callback = _callback;
    u = &_u;

    for(const data::userTitleInfo& t : u->titleInfo)
        tiles.emplace_back(new ui::titleTile(_iconW, _iconH, data::isFavorite(t.saveID), data::getTitleIconByTID(t.saveID)));
}

ui::titleview::~titleview()
{
    for(ui::titleTile *t : tiles)
        delete t;
}

void ui::titleview::refresh()
{
    for(ui::titleTile *t : tiles)
        delete t;

    tiles.clear();
    for(const data::userTitleInfo& t : u->titleInfo)
        tiles.emplace_back(new ui::titleTile(iconW, iconH, data::isFavorite(t.saveID), data::getTitleIconByTID(t.saveID)));

    if(selected > (int)tiles.size() - 1 && selected > 0)
        selected = tiles.size() - 1;
}

void ui::titleview::update()
{
    if(selected > (int)tiles.size() - 1 && selected > 0)
        selected = tiles.size() - 1;

    if(!active)
        return;

    switch(ui::padKeysDown())
    {
        case HidNpadButton_StickLUp:
        case HidNpadButton_StickRUp:
        case HidNpadButton_Up:
            if((selected -= rowCount) < 0)
                selected = 0;
            break;

        case HidNpadButton_StickLDown:
        case HidNpadButton_StickRDown:
        case HidNpadButton_Down:
            if((selected += rowCount) > (int)(tiles.size() - 1))
                selected = tiles.size() - 1;
            break;

        case HidNpadButton_StickLLeft:
        case HidNpadButton_StickRLeft:
        case HidNpadButton_Left:
            if(selected > 0)
                --selected;
            break;

        case HidNpadButton_StickLRight:
        case HidNpadButton_StickRRight:
        case HidNpadButton_Right:
            if(selected < (int)(tiles.size() - 1))
                ++selected;
            break;

        case HidNpadButton_L:
            if((selected -= rowCount * 3) < 0)
                selected = 0;
            break;

        case HidNpadButton_R:
            if((selected += rowCount * 3) > (int)(tiles.size() - 1))
                selected = tiles.size() - 1;
            break;
    }

    if(callback)
        (*callback)(this);
}

void ui::titleview::draw(SDL_Texture *target)
{
    if(tiles.size() <= 0)
        return;

    int tH = 0, tY = 0;
    SDL_QueryTexture(target, NULL, NULL, NULL, &tH);
    tY = tH - 214;
    if(selRectY > tY)
    {
        float add = ((float)tY - (float)selRectY) / ui::animScale;
        y += ceil(add);
    }
    else if(selRectY < 38)
    {
        float add = (38.0f - (float)selRectY) / ui::animScale;
        y += ceil(add);
    }

    if(clrAdd)
    {
        clrShft += 6;
        if(clrShft >= 0x72)
            clrAdd = false;
    }
    else
    {
        clrShft -= 3;
        if(clrShft <= 0)
            clrAdd = true;
    }

    int totalTitles = tiles.size(), selX = 32, selY = 64;
    for(int tY = y, i = 0; i < totalTitles; tY += iconH + vertGap)
    {
        int endRow = i + rowCount;
        for(int tX = x; i < endRow; tX += iconW + horGap, i++)
        {
            if(i >= totalTitles)
                break;

            if(i == selected)
            {
                //save x and y for later so it's draw over top
                selX = tX;
                selY = tY;
                selRectX = tX - 24;
                selRectY = tY - 24;
            }
            else
                tiles[i]->draw(target, tX, tY, false);
        }
    }

    if(showSel)
    {
        ui::drawBoundBox(target, selRectX, selRectY, 176, 176, clrShft);
        tiles[selected]->draw(target, selX, selY, true);
    }
    else
        tiles[selected]->draw(target, selX, selY, false);
}
