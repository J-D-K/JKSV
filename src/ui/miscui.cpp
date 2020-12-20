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
    size_t headWidth = textGetWidth(head.c_str(), ui::shared, 20);
    unsigned headX = (1280 / 2) - (headWidth / 2);

    texDraw(diaBox, frameBuffer, 320, 150);
    drawRect(frameBuffer, 320, 206, 640, 2, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFF6D6D6D) : clrCreateU32(0xFFCCCCCC));
    drawRect(frameBuffer, 352, 530, 576, 12, clrCreateU32(0xFF666666));
    drawRect(frameBuffer, 352, 530, (unsigned)width, 12, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFFC5FF00) : clrCreateU32(0xFFF05032));
    texDraw(ui::progCovLeft, frameBuffer, 352, 530);
    texDraw(ui::progCovRight, frameBuffer, 920, 530);

    drawText(head.c_str(), frameBuffer, ui::shared, headX, 168, 20, ui::txtDiag);
    drawTextWrap(text.c_str(), frameBuffer, ui::shared, 352, 230, 16, ui::txtDiag, 576);
}

void ui::showMessage(const char *head, const char *fmt, ...)
{
    //fake focus
    drawRectAlpha(frameBuffer, 0, 0, 1280, 720, clrCreateU32(0xAA0D0D0D));

    char tmp[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    unsigned headX = (640 / 2) - (textGetWidth(head, ui::shared, 20) / 2);
    unsigned okX = (640 / 2) - (textGetWidth(okt, ui::shared, 20) / 2);

    while(true)
    {
        ui::updatePad();

        if(ui::padKeysDown())
            break;

        gfxBeginFrame();
        texDraw(diaBox, frameBuffer, 320, 150);
        drawText(head, frameBuffer, ui::shared, 320 + headX, 168, 20, ui::txtDiag);
        drawTextWrap(tmp, frameBuffer, ui::shared, 352, 230, 16, ui::txtDiag, 576);
        drawText(okt, frameBuffer, ui::shared, 320 + okX, 522, 20, ui::txtDiag);
        gfxEndFrame();
    }
}

bool ui::confirm(bool hold, const char *fmt, ...)
{
    //fake focus
    drawRectAlpha(frameBuffer, 0, 0, 1280, 720, clrCreateU32(0xAA0D0D0D));

    char tmp[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    bool ret = false, heldDown = false;
    unsigned loadFrame = 0, holdCount = 0;
    uint8_t holdClrDiff = 0;
    clr holdClr = ui::txtDiag;

    unsigned headX = (640 / 2) - (textGetWidth("Confirm", ui::shared, 20) / 2);
    unsigned yesX = 160 - (textGetWidth(ui::yt.c_str(), ui::shared, 20) / 2);
    unsigned noX = 160 - (textGetWidth(ui::nt.c_str(), ui::shared, 20) / 2);

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
            yesX = 160 - (textGetWidth(yesText.c_str(), ui::shared, 20) / 2);
        }
        else if(hold && heldDown)
        {
            //Reset everything
            heldDown= false;
            holdCount = 0, loadFrame = 0, holdClrDiff = 0;
            yesX = 160 - (textGetWidth(ui::yt.c_str(), ui::shared, 20) / 2);
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
                holdClr = clrCreateRGBA(0xFF, 0xFF - holdClrDiff, 0xFF - holdClrDiff, 0xFF);
            else
                holdClr = clrCreateRGBA(0x25 + holdClrDiff, 0x00, 0x00, 0xFF);
        }

        gfxBeginFrame();
        texDraw(diaBox, frameBuffer, 320, 150);
        drawText(ui::confirmHead.c_str(), frameBuffer, ui::shared, 320 + headX, 168, 20, ui::txtDiag);
        drawText(yesText.c_str(), frameBuffer, ui::shared, 320 + yesX, 522, 20, holdClr);
        drawText(ui::nt.c_str(), frameBuffer, ui::shared, 640 + noX, 522, 20, ui::txtDiag);
        drawTextWrap(tmp, frameBuffer, ui::shared, 352, 230, 16, ui::txtDiag, 576);
        gfxEndFrame();
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

void ui::drawTextbox(tex *target, int x, int y, int w, int h)
{
    //Top
    texDraw(ui::cornerTopLeft, target, x, y);
    drawRect(target, x + 32, y, w - 64, 32, ui::tboxClr);
    texDraw(ui::cornerTopRight, target, (x + w) - 32, y);

    //middle
    drawRect(target, x, y + 32,  w, h - 64, tboxClr);

    //bottom
    texDraw(ui::cornerBottomLeft, target, x, (y + h) - 32);
    drawRect(target, x + 32, (y + h) - 32, w - 64, 32, tboxClr);
    texDraw(ui::cornerBottomRight, target, (x + w) - 32, (y + h) - 32);

}

void ui::drawTextboxInvert(tex *target, int x, int y, int w, int h)
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
    popWidth = textGetWidth(tmp, ui::shared, 24) + 32;
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

    drawTextbox(frameBuffer, popX, popY, popWidth, 64);
    drawText(popText.c_str(), frameBuffer, ui::shared, popX + 16, popY + 20, 24, ui::txtDiag);
}

