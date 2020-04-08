#include <cstring>
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

    button::button(const std::string& _txt, unsigned _x, unsigned _y, unsigned _w, unsigned _h)
    {
        x = _x;
        y = _y;
        w = _w;
        h = _h;
        text = _txt;

        unsigned tw = textGetWidth(text.c_str(), ui::shared, 20);
        unsigned th = 20;

        tx = x + (w / 2) - (tw / 2);
        ty = y + (h / 2) - (th / 2);
    }

    void button::setText(const std::string& _txt)
    {
        text = _txt;

        unsigned tw = textGetWidth(text.c_str(), ui::shared, 20);
        unsigned th = 20;

        tx = x + (w / 2) - (tw / 2);
        ty = y + (h / 2) - (th / 2);
    }

    void button::update(const touchPosition& p)
    {
        prev = cur;
        cur  = p;

        //If button was first thing pressed
        if(isOver() && prev.px == 0 && prev.py == 0)
        {
            first = true;
            pressed = true;
            retEvent = BUTTON_PRESSED;
        }
        else if(retEvent == BUTTON_PRESSED && hidTouchCount() == 0 && wasOver())
        {
            first = false;
            pressed = false;
            retEvent = BUTTON_RELEASED;
        }
        else if(retEvent != BUTTON_NOTHING && hidTouchCount() == 0)
        {
            first = false;
            pressed = false;
            retEvent = BUTTON_NOTHING;
        }
    }

    bool button::isOver()
    {
        return (cur.px > x && cur.px < x + w && cur.py > y && cur.py < y + h);
    }

    bool button::wasOver()
    {
        return (prev.px > x && prev.px < x + w && prev.py > y && prev.py < y + h);
    }

    void button::draw()
    {
        if(pressed)
        {
            ui::drawTextboxInvert(x, y, w, h);
            drawText(text.c_str(), frameBuffer, ui::shared, tx, ty, 20, mnuTxt);
        }
        else
        {
            ui::drawTextbox(x, y, w, h);
            drawText(text.c_str(), frameBuffer, ui::shared, tx, ty, 20, txtClr);
        }
    }

    void button::draw(const clr& _txt)
    {
        if(pressed)
        {
            ui::drawTextboxInvert(x, y, w, h);
            drawText(text.c_str(), frameBuffer, ui::shared, tx, ty, 20, _txt);
        }
        else
        {
            ui::drawTextbox(x, y, w, h);
            drawText(text.c_str(), frameBuffer, ui::shared, tx, ty, 20, _txt);
        }
    }

    void touchTrack::update(const touchPosition& p)
    {
        if(hidTouchCount() > 0)
        {
            pos[curPos++] = p;
            if(curPos == 5)
            {
                curPos = 0;

                for(unsigned i = 1; i < 5; i++)
                {
                    touchPosition c = pos[i], p = pos[i - 1];
                    avX += c.px - p.px;
                    avY += c.py - p.py;
                }

                avX /= 5;
                avY /= 5;

                if(avY <= -8)
                    retTrack = TRACK_SWIPE_UP;
                else if(avY >= 8)
                    retTrack = TRACK_SWIPE_DOWN;
                else if(retTrack <= -8)
                    retTrack = TRACK_SWIPE_LEFT;
                else if(retTrack >= 8)
                    retTrack = TRACK_SWIPE_RIGHT;
                else
                    retTrack = TRACK_NOTHING;

                std::memset(pos, 0, sizeof(touchPosition) * 5);
            }
            else
                retTrack = TRACK_NOTHING;
        }
        else
        {
            retTrack = TRACK_NOTHING;
            curPos = 0;
        }

    }

    void showMessage(const std::string& mess, const std::string& head)
    {
        button ok("OK \ue0e0 ", 320, 506, 640, 64);

        //center head text width
        size_t headWidth = textGetWidth(head.c_str(), ui::shared, 20);
        unsigned headX = (1280 / 2) - (headWidth / 2);

        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
            touchPosition p;
            hidTouchRead(&p, 0);

            ok.update(p);

            if(down || ok.getEvent() == BUTTON_RELEASED)
                break;

            gfxBeginFrame();
            ui::drawTextbox(320, 150, 640, 420);
            drawText(head.c_str(), frameBuffer, ui::shared, headX, 168, 20, txtClr);
            drawRect(frameBuffer, 320, 206, 640, 2, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFF6D6D6D) : clrCreateU32(0xFFCCCCCC));
            drawTextWrap(mess.c_str(), frameBuffer, ui::shared, 352, 230, 16, txtClr, 576);
            ok.draw();
            gfxEndFrame();
        }
    }

    bool confirm(const std::string& mess, bool hold)
    {
        bool ret = false, heldDown = false;
        unsigned loadFrame = 0, holdCount = 0;
        uint8_t holdClrDiff = 0;
        clr holdClr;

        button yes("Yes \ue0e0", 320, 506, 320, 64);
        button no("No \ue0e1", 640, 506, 320, 64);

        size_t headWidth = textGetWidth("Confirm", ui::shared, 20);
        unsigned headX = (1280 / 2) - (headWidth / 2);


        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
            uint64_t held = hidKeysHeld(CONTROLLER_P1_AUTO);
            uint64_t up   = hidKeysUp(CONTROLLER_P1_AUTO);

            touchPosition p;
            hidTouchRead(&p, 0);

            yes.update(p);
            no.update(p);

            std::string holdText;

            if(hold && (held & KEY_A || yes.getEvent() == BUTTON_PRESSED))
            {
                heldDown = true;
                holdCount++, holdClrDiff++;
                if(holdCount % 4 == 0)
                {
                    loadFrame++;
                    if(loadFrame > 7)
                        loadFrame = 0;
                }

                if(holdCount >= 180)
                {
                    ret = true;
                    break;
                }

                if(holdCount <= 60)
                    holdText = "(Hold) ";
                else if(holdCount <= 120)
                    holdText = "(Keep Holding) ";
                else if(holdCount < 180)
                    holdText = "(Almost There!) ";

                holdText += loadGlyphArray[loadFrame];
                yes.setText(holdText);
            }
            else if((hold && heldDown) && (up & KEY_A || yes.getEvent() == BUTTON_RELEASED))
            {
                //Reset everything
                heldDown= false;
                holdCount = 0, loadFrame = 0, holdClrDiff = 0;
                yes.setText("Yes \ue0e0");
            }
            else if(down & KEY_A || yes.getEvent() == BUTTON_RELEASED)
            {
                ret = true;
                break;
            }
            else if(down & KEY_B || no.getEvent() == BUTTON_RELEASED)
            {
                ret = false;
                break;
            }

            gfxBeginFrame();
            ui::drawTextbox(320, 150, 640, 420);
            drawText("Confirm", frameBuffer, ui::shared, headX, 168, 20, txtClr);
            drawRect(frameBuffer, 320, 206, 640, 2, ui::thmID == ColorSetId_Light ? clrCreateU32(0xFF6D6D6D) : clrCreateU32(0xFFCCCCCC));
            drawTextWrap(mess.c_str(), frameBuffer, ui::shared, 352, 230, 16, txtClr, 576);
            if(hold && heldDown)
            {
                if(ui::thmID == ColorSetId_Light)
                    holdClr = clrCreateRGBA(0xFF, 0xFF - holdClrDiff, 0xFF - holdClrDiff, 0xFF);
                else
                    holdClr = clrCreateRGBA(0x25 + holdClrDiff, 0x00, 0x00, 0xFF);

                yes.draw(holdClr);
            }
            else
                yes.draw();

            no.draw();
            gfxEndFrame();
        }

        return ret;
    }

    bool confirmTransfer(const std::string& f, const std::string& t)
    {
        std::string confMess = "Are you sure you want to copy #" + f + "# to #" + t +"#?";

        return confirm(confMess, false);
    }

    bool confirmDelete(const std::string& p)
    {
        std::string confMess = "Are you 100% sure you want to delete #" + p + "#? *This is permanent*!";

        return confirm(confMess, true);
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
