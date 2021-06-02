#include <string>
#include <switch.h>

#include "gfx.h"
#include "menu.h"
#include "ui.h"
#include "miscui.h"

static const SDL_Color menuColorLight = {0x32, 0x50, 0xF0, 0xFF};
static const SDL_Color menuColorDark  = {0x00, 0xFF, 0xC5, 0xFF};

#define HOLD_FC 10

void ui::menu::setParams(const unsigned& _x, const unsigned& _y, const unsigned& _rW)
{
    x = _x;
    y = _y;
    rW = _rW;
    rY = _y;
}

void ui::menu::addOpt(const std::string& add)
{
    if(gfx::getTextWidth(add.c_str(), 18) < rW - 32 || rW == 0)
        opt.push_back(add);
    else
    {
        std::string tmp;
        for(unsigned i = 0; i < add.length(); )
        {
            uint32_t tmpChr = 0;
            ssize_t untCnt = decode_utf8(&tmpChr, (uint8_t *)&add.c_str()[i]);

            tmp += add.substr(i, untCnt);
            i += untCnt;
            if(gfx::getTextWidth(tmp.c_str(), 18) >= rW - 32)
            {
                opt.push_back(tmp);
                break;
            }
        }
    }
}

void ui::menu::editOpt(int ind, const std::string& ch)
{
    opt[ind] = ch;
}

ui::menu::~menu()
{
    opt.clear();
}

void ui::menu::handleInput(const uint64_t& down, const uint64_t& held)
{
    if( (held & HidNpadButton_Up) || (held & HidNpadButton_Down))
        fc++;
    else
        fc = 0;
    if(fc > 10)
        fc = 0;

    int size = opt.size() - 1;
    if((down & HidNpadButton_Up) || ((held & HidNpadButton_Up) && fc == HOLD_FC))
    {
        selected--;
        if(selected < 0)
            selected = size;

        if((start > selected)  && (start > 0))
            start--;
        if(size < 15)
            start = 0;
        if((selected - 14) > start)
            start = selected - 14;
    }
    else if((down & HidNpadButton_Down) || ((held & HidNpadButton_Down) && fc == HOLD_FC))
    {
        selected++;
        if(selected > size)
            selected = 0;

        if((selected > (start + 14)) && ((start + 14) < size))
            start++;
        if(selected == 0)
            start = 0;
    }
    else if(down & HidNpadButton_Right)
    {
        selected += 7;
        if(selected > size)
            selected = size;
        if((selected - 14) > start)
            start = selected - 14;
    }
    else if(down & HidNpadButton_Left)
    {
        selected -= 7;
        if(selected < 0)
            selected = 0;
        if(selected < start)
            start = selected;
    }
}

void ui::menu::draw(const SDL_Color *textClr)
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

    int length = 0;
    if((opt.size() - 1) < 15)
        length = opt.size();
    else
        length = start + 15;

    for(int i = start; i < length; i++)
    {
        if(i == selected)
        {
            ui::drawBoundBox(x, y + ((i - start) * 36), rW, 36, clrSh);
            gfx::drawTextf(18, x + 8, (y + 8) + ((i - start) * 36), ui::thmID == ColorSetId_Light ? &menuColorLight : &menuColorDark, opt[i].c_str());
        }
        else
            gfx::drawTextf(18, x + 8, (y + 8) + ((i - start) * 36), textClr, opt[i].c_str());
    }
}

void ui::menu::reset()
{
    opt.clear();
    start = 0;
    selected = 0;

    fc = 0;
}

void ui::menu::adjust()
{
    if(selected > (int)opt.size() - 1)
        selected = opt.size() - 1;

    if(opt.size() < 14)
        start = 0;
    else if(opt.size() > 14 && start + 14 > (int)opt.size() - 1)
        start = opt.size() - 15;
}

