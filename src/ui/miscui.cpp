#include <cstring>
#include <cstdarg>
#include <switch.h>

#include "gfx.h"
#include "ui.h"
#include "miscui.h"
#include "util.h"

static bool popDraw = false;
static std::string popText;
static const char *okt = "OK \ue0e0";
static unsigned popY, popX, popWidth, popState, frameCount, frameHold;

static const SDL_Color divLight  = {0x6D, 0x6D, 0x6D, 0xFF};
static const SDL_Color divDark   = {0xCC, 0xCC, 0xCC, 0xFF};
static const SDL_Color shadow    = {0x66, 0x66, 0x66, 0xFF};

static const SDL_Color fillBack  = {0x66, 0x66, 0x66, 0xFF};
static const SDL_Color fillLight = {0x00, 0xFF, 0xC5, 0xFF};
static const SDL_Color fillDark  = {0x32, 0x50, 0xF0, 0xFF};

static const SDL_Color menuColorLight = {0x32, 0x50, 0xF0, 0xFF};
static const SDL_Color menuColorDark  = {0x00, 0xFF, 0xC5, 0xFF};

enum popStates
{
    popRise,
    popShow,
    popFall
};

//8
static const std::string loadGlyphArray[] =
{
    "\ue020", "\ue021", "\ue022", "\ue023",
    "\ue024", "\ue025", "\ue026", "\ue027"
};

void ui::menu::setParams(const unsigned& _x, const unsigned& _y, const unsigned& _rW, const unsigned& _fS, const unsigned& _mL)
{
    x = _x;
    y = _y;
    rW = _rW;
    rY = _y;
    fSize = _fS;
    mL = _mL;
    rH = _fS + 24;
}

void ui::menu::editParam(int _param, unsigned newVal)
{
    switch(_param)
    {
        case MENU_X:
            x = newVal;
            break;

        case MENU_Y:
            y = newVal;
            break;

        case MENU_RECT_WIDTH:
            rW = newVal;
            break;

        case MENU_FONT_SIZE:
            fSize = newVal;
            break;

        case MENU_MAX_SCROLL:
            mL = newVal;
            break;
    }
}

int ui::menu::addOpt(SDL_Texture *_icn, const std::string& add)
{
    ui::menuOpt newOpt;
    if((int)gfx::getTextWidth(add.c_str(), fSize) < rW - 56 || rW == 0)
        newOpt.txt = add;
    else
    {
        std::string tmp;
        for(unsigned i = 0; i < add.length(); )
        {
            uint32_t tmpChr = 0;
            ssize_t untCnt = decode_utf8(&tmpChr, (uint8_t *)&add.c_str()[i]);

            tmp += add.substr(i, untCnt);
            i += untCnt;
            if((int)gfx::getTextWidth(tmp.c_str(), fSize) >= rW - 48)
            {
                newOpt.txt = tmp;
                break;
            }
        }
    }
    newOpt.icn = _icn;
    opt.push_back(newOpt);
    return opt.size() - 1;
}

void ui::menu::editOpt(int ind, SDL_Texture *_icn, const std::string& ch)
{
    if(!opt[ind].icn && _icn)
        opt[ind].icn = _icn;

    opt[ind].txt = ch;
}

void ui::menu::setOptFunc(unsigned _ind, unsigned _funcbtn, funcPtr _func, void *args)
{
    opt[_ind].func[_funcbtn] = _func;
    opt[_ind].argPtr[_funcbtn] = args;
}

int ui::menu::getOptPos(const std::string& txt)
{
    for(unsigned i = 0; i < opt.size(); i++)
    {
        if(opt[i].txt == txt)
            return i;
    }
    return -1;
}


ui::menu::~menu()
{
    opt.clear();
}

void ui::menu::update()
{
    if(!isActive)
        return;

    uint64_t down = ui::padKeysDown();
    uint64_t held = ui::padKeysHeld();

    if(held & HidNpadButton_AnyUp || held & HidNpadButton_AnyDown)
        ++fc;
    else
        fc = 0;

    if(fc > 10)
        fc = 0;

    int oldSel = selected;
    int mSize = opt.size() - 1, scrL = mL - 1;
    if( (down & HidNpadButton_AnyUp) || ((held & HidNpadButton_AnyUp) && fc == 10) )
    {
        --selected;
        if(selected < 0)
            selected = mSize;

        if(mSize > (int)mL)
        {
            if(start > selected)
                --start;
            else if(selected - scrL > start)
                start = selected - scrL;
        }
    }
    else if( (down & HidNpadButton_AnyDown) || ((held & HidNpadButton_AnyDown) && fc == 10))
    {
        ++selected;
        if(selected > mSize)
            selected = 0;

        if(selected > (start + scrL) && (start + scrL) < mSize)
            ++start;
        if(selected == 0)
            start = 0;
    }
    else if(down & HidNpadButton_AnyLeft)
    {
        selected -= scrL / 2;
        if(selected < 0)
            selected = 0;
        if(selected < start)
            start = selected;
    }
    else if(down & HidNpadButton_AnyRight)
    {
        selected += scrL / 2;
        if(selected > mSize)
            selected = mSize;
        if(selected - scrL > start)
            start = selected - scrL;
    }

    //func exec
    switch(down)
    {
        case HidNpadButton_A:
            if(opt[selected].func[FUNC_A])
                (*opt[selected].func[FUNC_A])(opt[selected].argPtr[FUNC_A]);
            break;

        case HidNpadButton_B:
            if(opt[selected].func[FUNC_B])
                (*opt[selected].func[FUNC_B])(opt[selected].argPtr[FUNC_B]);
            break;

        case HidNpadButton_X:
            if(opt[selected].func[FUNC_X])
                (*opt[selected].func[FUNC_X])(opt[selected].argPtr[FUNC_X]);
            break;

        case HidNpadButton_Y:
            if(opt[selected].func[FUNC_Y])
                (*opt[selected].func[FUNC_Y])(opt[selected].argPtr[FUNC_Y]);
            break;
    }

    if(selected != oldSel && onChange)
        (*onChange)(NULL);

    if(callback)
        (*callback)(callbackArgs);
}

void ui::menu::draw(SDL_Texture *target, const SDL_Color *textClr, bool drawText)
{
    if(opt.size() < 1)
        return;

    if(clrAdd)
    {
        clrSh += 6;
        if(clrSh >= 0x72)
            clrAdd = false;
    }
    else
    {
        clrSh -= 3;
        if(clrSh <= 0x00)
            clrAdd = true;
    }

    for(int i = start; i < (int)opt.size(); i++)
    {
        if(i == selected)
        {
            if(isActive)
                ui::drawBoundBox(target, x, y + ((i - start) * rH), rW, rH, clrSh);

            gfx::drawRect(target, ui::thmID == ColorSetId_Light ? &menuColorLight : &menuColorDark, x + 10, ((y + (rH / 2 - fSize / 2)) + ((i - start) * rH)) - 2, 4, fSize + 4);
            if(drawText)
                gfx::drawTextf(target, fSize, x + 20, (y + (rH / 2 - fSize / 2)) + ((i - start) * rH), ui::thmID == ColorSetId_Light ? &menuColorLight : &menuColorDark, opt[i].txt.c_str());
        }
        else
        {
            if(drawText)
                gfx::drawTextf(target, fSize, x + 20, (y + (rH / 2 - fSize / 2)) + ((i - start) * rH), textClr, opt[i].txt.c_str());
        }

        int w, h;
        if(opt[i].icn && SDL_QueryTexture(opt[i].icn, NULL, NULL, &w, &h) == 0)
        {
            float scale = (float)fSize / (float)h;
            int dW = scale * w;
            int dH = scale * h;

            gfx::texDrawStretch(target, opt[i].icn, x + 20, (y + (rH / 2 - fSize / 2)) + ((i - start) * rH), dW, dH);
        }

    }
}

void ui::menu::reset()
{
    opt.clear();
    start = 0;
    selected = 0;

    fc = 0;
}

void ui::menu::setActive(bool _set)
{
    isActive = _set;
}

void ui::progBar::update(const uint64_t& _prog)
{
    prog = _prog;

    width = (float)(((float)prog / (float)max) * 576);
}

void ui::progBar::draw(const std::string& text, const std::string& head)
{
    size_t headWidth = gfx::getTextWidth(head.c_str(), 20);
    unsigned headX = (1280 / 2) - (headWidth / 2);

    ui::drawTextbox(NULL, 320, 150, 640, 420);
    gfx::drawLine(NULL, ui::thmID == ColorSetId_Light ? &divLight : &divDark, 320, 206, 959, 206);
    gfx::drawRect(NULL, &fillBack, 352, 530, 576, 12);
    gfx::drawRect(NULL, ui::thmID == ColorSetId_Light ? &fillLight : &fillDark, 352, 530, (int)width, 12);
    gfx::texDraw(NULL, ui::progCovLeft, 352, 530);
    gfx::texDraw(NULL, ui::progCovRight, 920, 530);

    gfx::drawTextf(NULL, 20, headX, 168, &ui::txtDiag, head.c_str());
    gfx::drawTextfWrap(NULL, 16, 352, 230, 576, &ui::txtDiag, text.c_str());
}

ui::slideOutPanel::slideOutPanel(int _w, int _h, int _y, funcPtr _draw)
{
    w = _w;
    h = _h;
    y = _y;
    drawFunc = _draw;
    panel = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET, w, h);
    SDL_SetTextureBlendMode(panel, SDL_BLENDMODE_BLEND);

    int getDiv = 99;
    while(w % getDiv != 0) { getDiv--; }
    slideSpd = getDiv;
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
        x -= slideSpd;
    }
    else if(!open && x < 1280)
    {
        x += slideSpd;
    }

    //don't waste time drawing if you can't even see it.
    if(x < 1280)
    {
        (*drawFunc)(panel);
        gfx::texDraw(NULL, panel, x, y);
    }
}

//Todo make less hardcoded
void ui::titleTile::draw(SDL_Texture *target, int x, int y, bool sel)
{
    if(sel)
    {
        unsigned xScale = w * 1.28, yScale = h * 1.28;
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
}

void ui::titleview::update()
{
    if(selected > (int)tiles.size() - 1)
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
    if(selRectY > 264)
        y -= 48;
    else if(selRectY > 144)
        y -= 24;
    else if(selRectY < -82)
        y += 48;
    else if(selRectY < 38)
        y += 24;

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

            if(i == selected && active)
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

    if(active)
    {
        ui::drawBoundBox(target, selRectX, selRectY, 176, 176, clrShft);
        tiles[selected]->draw(target, selX, selY, true);
    }
}

void ui::showMessage(const char *head, const char *fmt, ...)
{
    char tmp[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    unsigned headX = (640 / 2) - (gfx::getTextWidth(head, 20) / 2);
    unsigned okX = (640 / 2) - (gfx::getTextWidth(okt, 20) / 2);

    while(true)
    {
        ui::updateInput();

        if(ui::padKeysDown())
            break;

        ui::drawTextbox(NULL, 320, 150, 640, 420);
        gfx::drawTextf(NULL, 20, 320 + headX, 168, &ui::txtDiag, head);
        gfx::drawTextfWrap(NULL, 16, 352, 230, 576, &ui::txtDiag, tmp);
        gfx::drawTextf(NULL, 20, 320 + okX, 522, &ui::txtDiag, okt);
        gfx::present();
    }
    ui::updateInput();
}

bool ui::confirm(bool hold, const char *fmt, ...)
{
    char tmp[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    bool ret = false, heldDown = false;
    unsigned loadFrame = 0, holdCount = 0;
    uint8_t holdClrDiff = 0;
    SDL_Color holdClr = ui::txtDiag;

    unsigned headX = (640 / 2) - (gfx::getTextWidth("Confirm", 20) / 2);
    unsigned yesX = 160 - (gfx::getTextWidth(ui::yt.c_str(), 20) / 2);
    unsigned noX = 160 - (gfx::getTextWidth(ui::nt.c_str(), 20) / 2);

    std::string yesText = yt;

    while(true)
    {
        ui::updateInput();

        uint64_t down = ui::padKeysDown();
        uint64_t held = ui::padKeysHeld();

        if(hold && held & HidNpadButton_A)
        {
            heldDown = true;
            holdCount++, holdClrDiff++;
            if(holdCount % 4 == 0)
            {
                loadFrame++;
                if(loadFrame > 7)
                    loadFrame = 0;
            }

            if(holdCount >= 120)
            {
                ret = true;
                break;
            }

            if(holdCount <= 40)
                yesText = ui::holdingText[0];
            else if(holdCount <= 80)
                yesText = ui::holdingText[1];
            else if(holdCount < 120)
                yesText = ui::holdingText[2];

            yesText += loadGlyphArray[loadFrame];
            yesX = 160 - (gfx::getTextWidth(yesText.c_str(), 20) / 2);
        }
        else if(hold && heldDown)
        {
            //Reset everything
            heldDown= false;
            holdCount = 0, loadFrame = 0, holdClrDiff = 0;
            yesX = 160 - (gfx::getTextWidth(ui::yt.c_str(), 20) / 2);
            yesText = ui::yt;
            holdClr = ui::txtDiag;
        }
        else if(down & HidNpadButton_A)
        {
            ret = true;
            break;
        }
        else if(down & HidNpadButton_B)
        {
            ret = false;
            break;
        }

        if(hold && heldDown)
        {
            if(ui::thmID == ColorSetId_Light)
                holdClr = {0xFF, (uint8_t)(0xFF - holdClrDiff), (uint8_t)(0xFF - holdClrDiff), 0xFF};
            else
                holdClr = {(uint8_t)(0x25 + holdClrDiff), 0x00, 0x00, 0xFF};
        }

        ui::drawTextbox(NULL, 320, 150, 640, 420);
        gfx::drawTextf(NULL, 20, 320 + headX, 168, &ui::txtDiag, ui::confirmHead.c_str());
        gfx::drawTextf(NULL, 20, 320 + yesX, 522, &holdClr, yesText.c_str());
        gfx::drawTextf(NULL, 20, 640 + noX, 522, &ui::txtDiag, ui::nt.c_str());
        gfx::drawTextfWrap(NULL, 16, 352, 230, 576, &ui::txtDiag, tmp);
        gfx::present();
    }

    ui::updateInput();

    return ret;
}

bool ui::confirmTransfer(const std::string& f, const std::string& t)
{
    return confirm(false, ui::confCopy.c_str(), f.c_str(), t.c_str());
}

bool ui::confirmDelete(const std::string& p)
{
    return confirm(data::holdDel, ui::confDel.c_str(), p.c_str());
}

void ui::drawTextbox(SDL_Texture *target, int x, int y, int w, int h)
{
    //Top
    gfx::texDraw(target, ui::cornerTopLeft, x, y);
    gfx::drawRect(target, &ui::tboxClr, x + 32, y, w - 64, 32);
    gfx::texDraw(target, ui::cornerTopRight, (x + w) - 32, y);

    //middle
    gfx::drawRect(target, &ui::tboxClr, x, y + 32,  w, h - 64);

    //bottom
    gfx::texDraw(target, ui::cornerBottomLeft, x, (y + h) - 32);
    gfx::drawRect(target, &ui::tboxClr, x + 32, (y + h) - 32, w - 64, 32);
    gfx::texDraw(target, ui::cornerBottomRight, (x + w) - 32, (y + h) - 32);
}

void ui::drawBoundBox(SDL_Texture *target, int x, int y, int w, int h, uint8_t clrSh)
{
    SDL_SetRenderTarget(gfx::render, target);
    SDL_Color rectClr;

    if(ui::thmID == ColorSetId_Light)
        rectClr = {0xFD, 0xFD, 0xFD, 0xFF};
    else
        rectClr = {0x21, 0x22, 0x21, 0xFF};

    gfx::drawRect(target, &rectClr, x + 4, y + 4, w - 8, h - 8);

    rectClr = {0x00,(uint8_t)(0x88 + clrSh), (uint8_t)(0xC5 + (clrSh / 2)), 0xFF};

    SDL_SetTextureColorMod(mnuTopLeft, rectClr.r, rectClr.g, rectClr.b);
    SDL_SetTextureColorMod(mnuTopRight, rectClr.r, rectClr.g, rectClr.b);
    SDL_SetTextureColorMod(mnuBotLeft, rectClr.r, rectClr.g, rectClr.b);
    SDL_SetTextureColorMod(mnuBotRight, rectClr.r, rectClr.g, rectClr.b);

    //top
    gfx::texDraw(target, mnuTopLeft, x, y);
    gfx::drawRect(target, &rectClr, x + 4, y, w - 8, 4);
    gfx::texDraw(target, mnuTopRight, (x + w) - 4, y);

    //mid
    gfx::drawRect(target, &rectClr, x, y + 4, 4, h - 8);
    gfx::drawRect(target, &rectClr, (x + w) - 4, y + 4, 4, h - 8);

    //bottom
    gfx::texDraw(target, mnuBotLeft, x, (y + h) - 4);
    gfx::drawRect(target, &rectClr, x + 4, (y + h) - 4, w - 8, 4);
    gfx::texDraw(target, mnuBotRight, (x + w) - 4, (y + h) - 4);
}

void ui::showPopup(unsigned frames, const char *fmt, ...)
{
    char tmp[256];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    frameCount = 0;
    frameHold = frames;
    popWidth = gfx::getTextWidth(tmp, 24) + 32;
    popX = 640 - (popWidth / 2);

    popText = tmp;
    popY = 721;
    popState = popRise;
    popDraw = true;
}

void ui::drawPopup(const uint64_t& down)
{
    if(!popDraw)
        return;

    switch(popState)
    {
        case popRise:
            if(popY > 560)
                popY -= 24;
            else
                popState = popShow;
            break;

        case popShow:
            if(frameCount++ >= frameHold || down)
                popState = popFall;
            break;

        case popFall:
            if(popY < 721)
                popY += 24;
            else
                popDraw = false;
            break;
    }

    drawTextbox(NULL, popX, popY, popWidth, 64);
    gfx::drawTextf(NULL, 24, popX + 16, popY + 20, &ui::txtDiag, popText.c_str());
}
