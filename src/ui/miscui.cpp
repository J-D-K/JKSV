#include <cstring>
#include <cstdarg>
#include <switch.h>

#include "gfx.h"
#include "ui.h"
#include "miscui.h"
#include "util.h"

static bool popDraw = false;
static std::string popText;
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

        ui::drawTextbox(320, 150, 640, 420);
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
        char tmp[512];
        va_list args;
        va_start(args, fmt);
        vsprintf(tmp, fmt, args);

        //center head text width
        size_t headWidth = textGetWidth(head, ui::shared, 20);
        unsigned headX = (1280 / 2) - (headWidth / 2);

        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);

            if(down)
                break;

            gfxBeginFrame();
            ui::drawTextbox(320, 150, 640, 420);
            drawText(head, frameBuffer, ui::shared, headX, 168, 20, txtClr);
            drawRect(frameBuffer, 320, 206, 640, 2, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFF6D6D6D) : clrCreateU32(0xFFCCCCCC));
            drawTextWrap(tmp, frameBuffer, ui::shared, 352, 230, 16, txtClr, 576);
            gfxEndFrame();
        }
    }

    bool confirm(bool hold, const char *fmt, ...)
    {
        char tmp[512];
        va_list args;
        va_start(args, fmt);
        vsprintf(tmp, fmt, args);

        bool ret = false, heldDown = false;
        unsigned loadFrame = 0, holdCount = 0;
        uint8_t holdClrDiff = 0;
        clr holdClr;

        size_t headWidth = textGetWidth("Confirm", ui::shared, 20);
        unsigned headX = (1280 / 2) - (headWidth / 2);


        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
            uint64_t held = hidKeysHeld(CONTROLLER_P1_AUTO);

            std::string holdText;

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
                    holdText = "(Hold) ";
                else if(holdCount <= 80)
                    holdText = "(Keep Holding) ";
                else if(holdCount < 120)
                    holdText = "(Almost There!) ";

                holdText += loadGlyphArray[loadFrame];
            }
            else if(hold && heldDown)
            {
                //Reset everything
                heldDown= false;
                holdCount = 0, loadFrame = 0, holdClrDiff = 0;
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

            gfxBeginFrame();
            ui::drawTextbox(320, 150, 640, 420);
            drawText("Confirm", frameBuffer, ui::shared, headX, 168, 20, txtClr);
            drawRect(frameBuffer, 320, 206, 640, 2, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFF6D6D6D) : clrCreateU32(0xFFCCCCCC));
            drawTextWrap(tmp, frameBuffer, ui::shared, 352, 230, 16, txtClr, 576);
            if(hold && heldDown)
            {
                if(ui::thmID == ColorSetId_Light)
                    holdClr = clrCreateRGBA(0xFF, 0xFF - holdClrDiff, 0xFF - holdClrDiff, 0xFF);
                else
                    holdClr = clrCreateRGBA(0x25 + holdClrDiff, 0x00, 0x00, 0xFF);
            }
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

    void drawTextbox(int x, int y, int w, int h)
    {
        //Top
        texDraw(ui::cornerTopLeft, frameBuffer, x, y);
        drawRect(frameBuffer, x + 32, y, w - 64, 32, ui::tboxClr);
        texDraw(ui::cornerTopRight, frameBuffer, (x + w) - 32, y);

        //middle
        drawRect(frameBuffer, x, y + 32,  w, h - 64, tboxClr);

        //bottom
        texDraw(ui::cornerBottomLeft, frameBuffer, x, (y + h) - 32);
        drawRect(frameBuffer, x + 32, (y + h) - 32, w - 64, 32, tboxClr);
        texDraw(ui::cornerBottomRight, frameBuffer, (x + w) - 32, (y + h) - 32);

    }

    void drawTextboxInvert(int x, int y, int w, int h)
    {
        clr temp = ui::tboxClr;
        clrInvert(&temp);

        //Top
        texDrawInvert(ui::cornerTopLeft, frameBuffer, x, y);
        drawRect(frameBuffer, x + 32, y, w - 64, 32, temp);
        texDrawInvert(ui::cornerTopRight, frameBuffer, (x + w) - 32, y);

        //middle
        drawRect(frameBuffer, x, y + 32,  w, h - 64, temp);

        //bottom
        texDrawInvert(ui::cornerBottomLeft, frameBuffer, x, (y + h) - 32);
        drawRect(frameBuffer, x + 32, (y + h) - 32, w - 64, 32, temp);
        texDrawInvert(ui::cornerBottomRight, frameBuffer, (x + w) - 32, (y + h) - 32);
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

        drawTextbox(popX, popY, popWidth, 64);
        drawText(popText.c_str(), frameBuffer, ui::shared, popX + 16, popY + 20, 24, ui::txtClr);
    }
}
