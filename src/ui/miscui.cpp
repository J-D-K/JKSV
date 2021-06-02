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


void ui::progBar::update(const uint64_t& _prog)
{
    prog = _prog;

    width = (float)(((float)prog / (float)max) * 576);
}

void ui::progBar::draw(const std::string& text, const std::string& head)
{
    size_t headWidth = gfx::getTextWidth(head.c_str(), 20);
    unsigned headX = (1280 / 2) - (headWidth / 2);

    ui::drawTextbox(320, 150, 640, 420);
    gfx::drawLine(ui::thmID == ColorSetId_Light ? &divLight : &divDark, 320, 206, 959, 206);
    gfx::drawRect(&fillBack, 352, 530, 576, 12);
    gfx::drawRect(ui::thmID == ColorSetId_Light ? &fillLight : &fillDark, 352, 530, (int)width, 12);
    gfx::texDraw(ui::progCovLeft, 352, 530);
    gfx::texDraw(ui::progCovRight, 920, 530);


    gfx::drawTextf(20, headX, 160, &ui::txtDiag, head.c_str());
    gfx::drawTextfWrap(16, 352, 230, 576, &ui::txtDiag, text.c_str());
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
        ui::updatePad();

        if(ui::padKeysDown())
            break;

        ui::drawTextbox(320, 150, 640, 420);
        gfx::drawTextf(20, 320 + headX, 168, &ui::txtDiag, head);
        gfx::drawTextfWrap(16, 352, 230, 576, &ui::txtDiag, tmp);
        gfx::drawTextf(20, 320 + okX, 522, &ui::txtDiag, okt);
        gfx::present();
    }
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
        ui::updatePad();

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
                holdClr = {0xFF, 0xFF - holdClrDiff, 0xFF - holdClrDiff, 0xFF};
            else
                holdClr = {0x25 + holdClrDiff, 0x00, 0x00, 0xFF};
        }

        ui::drawTextbox(320, 150, 640, 420);
        gfx::drawTextf(20, 320 + headX, 168, &ui::txtDiag, ui::confirmHead.c_str());
        gfx::drawTextf(20, 320 + yesX, 522, &holdClr, yesText.c_str());
        gfx::drawTextf(20, 640 + noX, 522, &ui::txtDiag, ui::nt.c_str());
        gfx::drawTextfWrap(16, 352, 230, 576, &ui::txtDiag, tmp);
        gfx::present();
    }
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

void ui::drawTextbox(int x, int y, int w, int h)
{
    //Top
    gfx::texDraw(ui::cornerTopLeft, x, y);
    gfx::drawRect(&ui::tboxClr, x + 32, y, w - 64, 32);
    gfx::texDraw(ui::cornerTopRight, (x + w) - 32, y);

    //middle
    gfx::drawRect(&ui::tboxClr, x, y + 32,  w, h - 64);

    //bottom
    gfx::texDraw(ui::cornerBottomLeft, x, (y + h) - 32);
    gfx::drawRect(&ui::tboxClr, x + 32, (y + h) - 32, w - 64, 32);
    gfx::texDraw(ui::cornerBottomRight, (x + w) - 32, (y + h) - 32);
}

/*void ui::drawTextboxInvert(tex *target, int x, int y, int w, int h)
{
    clr temp = ui::tboxClr;
    clrInvert(&temp);

    //Top
    texDrawInvert(ui::cornerTopLeft, target, x, y);
    drawRect(target, x + 32, y, w - 64, 32, temp);
    texDrawInvert(ui::cornerTopRight, target, (x + w) - 32, y);

    //middle
    drawRect(target, x, y + 32,  w, h - 64, temp);

    //bottom
    texDrawInvert(ui::cornerBottomLeft, target, x, (y + h) - 32);
    drawRect(target, x + 32, (y + h) - 32, w - 64, 32, temp);
    texDrawInvert(ui::cornerBottomRight, target, (x + w) - 32, (y + h) - 32);
}*/

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

    drawTextbox(popX, popY, popWidth, 64);
    gfx::drawTextf(24, popX + 16, popY + 20, &ui::txtDiag, popText.c_str());
}

