#include <string>
#include <switch.h>

#include "gfx.h"
#include "menu.h"
#include "ui.h"
#include "miscui.h"

static const clr menuColorLight = clrCreateU32(0xFFF05032), menuColorDark = clrCreateU32(0xFFC5FF00);

namespace ui
{
    void menu::setParams(const unsigned& _x, const unsigned& _y, const unsigned& _rW)
    {
        x = _x;
        y = _y;
        rW = _rW;
        rY = _y;
    }

    void menu::addOpt(const std::string& add)
    {
        if(textGetWidth(add.c_str(), ui::shared, 18) < rW - 32 || rW == 0)
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
                if(textGetWidth(tmp.c_str(), ui::shared, 18) >= rW - 32)
                {
                    opt.push_back(tmp);
                    break;
                }
            }
        }
    }

    void menu::editOpt(int ind, const std::string& ch)
    {
        opt[ind] = ch;
    }

    menu::~menu()
    {
        opt.clear();
    }

    void menu::handleInput(const uint64_t& down, const uint64_t& held)
    {
        if( (held & KEY_UP) || (held & KEY_DOWN))
            fc++;
        else
            fc = 0;
        if(fc > 10)
            fc = 0;

        int size = opt.size() - 1;
        if((down & KEY_UP) || ((held & KEY_UP) && fc == 10))
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
        else if((down & KEY_DOWN) || ((held & KEY_DOWN) && fc == 10))
        {
            selected++;
            if(selected > size)
                selected = 0;

            if((selected > (start + 14)) && ((start + 14) < size))
                start++;
            if(selected == 0)
                start = 0;
        }
        else if(down & KEY_RIGHT)
        {
            selected += 7;
            if(selected > size)
                selected = size;
            if((selected - 14) > start)
                start = selected - 14;
        }
        else if(down & KEY_LEFT)
        {
            selected -= 7;
            if(selected < 0)
                selected = 0;
            if(selected < start)
                start = selected;
        }
    }

    void menu::draw(const clr& textClr)
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
                drawBoundBox(x, y + ((i - start) * 36), rW, 36, clrSh);
                drawText(opt[i].c_str(), frameBuffer, shared, x + 8, (y + 8) + ((i - start) * 36), 18, ui::thmID == ColorSetId_Light ? menuColorLight : menuColorDark);
            }
            else
                drawText(opt[i].c_str(), frameBuffer, shared, x + 8, (y + 8) + ((i - start) * 36), 18, textClr);
        }
    }

    void menu::reset()
    {
        opt.clear();
        start = 0;
        selected = 0;

        fc = 0;
    }

    void menu::adjust()
    {
        if(selected > (int)opt.size() - 1)
            selected = opt.size() - 1;

        if(opt.size() < 14)
            start = 0;
        else if(opt.size() > 14 && start + 14 > (int)opt.size() - 1)
            start = opt.size() - 15;
    }
}
