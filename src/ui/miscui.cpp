#include <cstring>
#include <switch.h>

#include "gfx.h"
#include "ui.h"
#include "miscui.h"
#include "util.h"

namespace ui
{
    progBar::progBar(const unsigned& _max)
    {
        max = (float)_max;
    }

    void progBar::update(const unsigned& _prog)
    {
        prog = (float)_prog;

        float percent = (float)(prog / max) * 100;
        width = (float)(percent * 1088) / 100;
    }

    void progBar::draw(const std::string& text)
    {
        ui::drawTextbox(64, 240, 1152, 240);
        drawRect(frameBuffer, 96, 400, 1088, 64, clrCreateU32(0xFF000000));
        drawRect(frameBuffer, 96, 400, (unsigned)width, 64, clrCreateU32(0xFF00CC00));

        //char tmp[64];
        //sprintf(tmp, "%u / %u", (unsigned)prog, (unsigned)max);
        drawText(text.c_str(), frameBuffer, ui::shared, 80, 256, 24, txtClr);
        //gfx::drawText(tmp, 80, 320, 64, 0x000000FF);
    }

    button::button(const std::string& _txt, unsigned _x, unsigned _y, unsigned _w, unsigned _h)
    {
        x = _x;
        y = _y;
        w = _w;
        h = _h;
        text = _txt;

        unsigned tw = textGetWidth(text.c_str(), ui::shared, 24);
        unsigned th = 24;

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
            drawText(text.c_str(), frameBuffer, ui::shared, tx, ty, 24, mnuTxt);
        }
        else
        {
            ui::drawTextbox(x, y, w, h);
            drawText(text.c_str(), frameBuffer, ui::shared, tx, ty, 24, txtClr);
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

    void showMessage(const std::string& mess)
    {
        button ok("OK", 256, 496, 768, 96);
        std::string wrapMess = util::getWrappedString(mess, 24, 752);
        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
            touchPosition p;
            hidTouchRead(&p, 0);

            ok.update(p);

            if(down & KEY_A || down & KEY_B || ok.getEvent() == BUTTON_RELEASED)
                break;

            ui::drawTextbox(256, 128, 768, 464);
            drawText(wrapMess.c_str(), frameBuffer, ui::shared, 272, 144, 24, txtClr);
            ok.draw();
            texDrawInvert(ui::buttonA, frameBuffer, ok.getTx() + 56, ok.getTy() - 4);

            gfxHandleBuffs();
        }
    }

    void showError(const std::string& mess, const Result& r)
    {
        button ok("OK (A)", 256, 496, 768, 96);
        char tmp[512];
        std::string wrapMess = util::getWrappedString(mess, 48, 752);
        sprintf(tmp, "%s\n0x%08X", mess.c_str(), (unsigned)r);

        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
            touchPosition p;
            hidTouchRead(&p, 0);

            ok.update(p);

            if(down & KEY_A || down & KEY_B || ok.getEvent() == BUTTON_RELEASED)
                break;

            ui::drawTextbox(256, 128, 768, 464);
            drawText(tmp, frameBuffer, shared, 272, 144, 48, txtClr);
            ok.draw();

            gfxHandleBuffs();
        }
    }

    bool confirm(const std::string& mess)
    {
        bool ret = false;

        button yes("Yes   ", 256, 496, 384, 96);
        button no("No   ", 640, 496, 384, 96);

        std::string wrapMess = util::getWrappedString(mess, 24, 752);

        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
            touchPosition p;
            hidTouchRead(&p, 0);

            yes.update(p);
            no.update(p);

            if(down & KEY_A || yes.getEvent() == BUTTON_RELEASED)
            {
                ret = true;
                break;
            }
            else if(down & KEY_B || no.getEvent() == BUTTON_RELEASED)
            {
                ret = false;
                break;
            }

            ui::drawTextbox(256, 128, 768, 464);
            drawText(wrapMess.c_str(), frameBuffer, ui::shared, 272, 144, 24, txtClr);
            yes.draw();
            texDrawInvert(ui::buttonA, frameBuffer, yes.getTx() + 64, yes.getTy() - 4);
            no.draw();
            texDrawInvert(ui::buttonB, frameBuffer, no.getTx() + 56, no.getTy() - 4);

            gfxHandleBuffs();
        }

        return ret;
    }

    bool confirmTransfer(const std::string& f, const std::string& t)
    {
        std::string confMess = "Are you sure you want to copy \"" + f + "\" to \"" + t +"\"?";

        return confirm(confMess);
    }

    bool confirmDelete(const std::string& p)
    {
        std::string confMess = "Are you 100% sure you want to delete \"" + p + "\"? This is permanent!";

        return confirm(confMess);
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
}
