#include <cstring>
#include <cstdarg>
#include <switch.h>

#include "gfx.h"
#include "ui.h"
#include "miscui.h"
#include "util.h"

static bool popDraw = false;
static std::string popText;
static const char *yt = "Yes \ue0e0", *nt = "No \ue0e1", *okt = "OK \ue0e0";
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

namespace ui
{
    void progBar::update(const uint64_t& _prog)
    {
        prog = _prog;

        width = (float)(((float)prog / (float)max) * 576);
    }

    void progBar::draw(const std::string& text, const std::string& head)
    {
        size_t headWidth = textGetWidth(head.c_str(), ui::shared, 20);
        unsigned headX = (1280 / 2) - (headWidth / 2);

        texDraw(diaBox, frameBuffer, 320, 150);
        drawRect(frameBuffer, 320, 206, 640, 2, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFF6D6D6D) : clrCreateU32(0xFFCCCCCC));
        drawRect(frameBuffer, 352, 530, 576, 12, clrCreateU32(0xFF666666));
        drawRect(frameBuffer, 352, 530, (unsigned)width, 12, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFFC5FF00) : clrCreateU32(0xFFF05032));
        texDraw(ui::progCovLeft, frameBuffer, 352, 530);
        texDraw(ui::progCovRight, frameBuffer, 920, 530);

        drawText(head.c_str(), frameBuffer, ui::shared, headX, 168, 20, txtClr);
        drawTextWrap(text.c_str(), frameBuffer, ui::shared, 352, 230, 16, txtClr, 576);
    }

    void showMessage(const char *head, const char *fmt, ...)
    {
        //fake focus
        drawRectAlpha(frameBuffer, 0, 0, 1280, 720, clrCreateU32(0xAA0D0D0D));

        char tmp[512];
        va_list args;
        va_start(args, fmt);
        vsprintf(tmp, fmt, args);

        unsigned headX = (640 / 2) - (textGetWidth(head, ui::shared, 20) / 2);
        unsigned okX = (640 / 2) - (textGetWidth(okt, ui::shared, 20) / 2);

        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);

            if(down)
                break;

            gfxBeginFrame();
            texDraw(diaBox, frameBuffer, 320, 150);
            drawText(head, frameBuffer, ui::shared, 320 + headX, 168, 20, ui::txtClr);
            drawTextWrap(tmp, frameBuffer, ui::shared, 352, 230, 16, ui::txtClr, 576);
            drawText(okt, frameBuffer, ui::shared, 320 + okX, 530, 20, ui::txtClr);
            gfxEndFrame();
        }
    }

    bool confirm(bool hold, const char *fmt, ...)
    {
        //fake focus
        drawRectAlpha(frameBuffer, 0, 0, 1280, 720, clrCreateU32(0xAA0D0D0D));

        char tmp[512];
        va_list args;
        va_start(args, fmt);
        vsprintf(tmp, fmt, args);

        bool ret = false, heldDown = false;
        unsigned loadFrame = 0, holdCount = 0;
        uint8_t holdClrDiff = 0;
        clr holdClr = ui::txtClr;

        unsigned headX = (640 / 2) - (textGetWidth("Confirm", ui::shared, 20) / 2);
        unsigned yesX = 160 - (textGetWidth(yt, ui::shared, 20) / 2);
        unsigned noX = 160 - (textGetWidth(nt, ui::shared, 20) / 2);

        std::string yesText = yt;

        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
            uint64_t held = hidKeysHeld(CONTROLLER_P1_AUTO);

            if(hold && held & KEY_A)
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
                    yesText = "(Hold) ";
                else if(holdCount <= 80)
                    yesText = "(Keep Holding) ";
                else if(holdCount < 120)
                    yesText = "(Almost There!) ";

                yesText += loadGlyphArray[loadFrame];
                yesX = 160 - (textGetWidth(yesText.c_str(), ui::shared, 20) / 2);
            }
            else if(hold && heldDown)
            {
                //Reset everything
                heldDown= false;
                holdCount = 0, loadFrame = 0, holdClrDiff = 0;
                yesX = 160 - (textGetWidth(yt, ui::shared, 20) / 2);
                yesText = yt;
                holdClr = ui::txtClr;
            }
            else if(down & KEY_A)
            {
                ret = true;
                break;
            }
            else if(down & KEY_B)
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
            drawText("Confirm", frameBuffer, ui::shared, 320 + headX, 168, 20, ui::txtClr);
            drawText(yesText.c_str(), frameBuffer, ui::shared, 320 + yesX, 530, 20, holdClr);
            drawText(nt, frameBuffer, ui::shared, 860    - noX, 530, 20, ui::txtClr);
            drawTextWrap(tmp, frameBuffer, ui::shared, 352, 230, 16, ui::txtClr, 576);
            gfxEndFrame();
        }

        return ret;
    }

    bool confirmTransfer(const std::string& f, const std::string& t)
    {
        return confirm(false, ui::confCopy.c_str(), f.c_str(), t.c_str());
    }

    bool confirmDelete(const std::string& p)
    {
        return confirm(data::holdDel, ui::confDel.c_str(), p.c_str());
    }

    void drawTextbox(tex *target, int x, int y, int w, int h)
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

    void drawTextboxInvert(tex *target, int x, int y, int w, int h)
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

    void showPopup(const std::string& mess, unsigned frames)
    {
        frameCount = 0;
        frameHold = frames;
        popWidth = textGetWidth(mess.c_str(), ui::shared, 24) + 32;
        popX = 640 - (popWidth / 2);

        popText = mess;
        popY = 721;
        popState = popRise;
        popDraw = true;
    }

    void drawPopup(const uint64_t& down)
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
        drawText(popText.c_str(), frameBuffer, ui::shared, popX + 16, popY + 20, 24, ui::txtClr);
    }
}
