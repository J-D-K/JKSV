#include <switch.h>

#include "gfx.h"
#include "kb.h"
#include "util.h"

static const char qwerty[] =
{
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '_',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ':', '.', '/'
};

namespace ui
{
    key::key(const std::string& _txt, const char& _l, unsigned _x, unsigned _y, unsigned _w, unsigned _h) : button(_txt, _x, _y, _w, _h)
    {
        let = _l;
    }

    void key::updateText(const std::string& txt)
    {
        text = txt;
    }

    void key::toCaps()
    {
        let = toupper(let);

        char tmp[2];
        sprintf(tmp, "%c", let);
        updateText(tmp);
    }

    void key::toLower()
    {
        let = tolower(let);

        char tmp[2];
        sprintf(tmp, "%c", let);
        updateText(tmp);
    }

    keyboard::keyboard()
    {

        for(int y = 256, i = 0; i < 40; y += 96)
        {
            int endRow = i + 10;
            for(int x = 160; i < endRow; x += 96, i++)
            {
                char tmp[2];
                sprintf(tmp, "%c", qwerty[i]);
                key newKey(tmp, qwerty[i], x, y, 80, 80);
                keys.push_back(newKey);
            }
        }

        //spc key
        key shift("Shift", ' ', 16, 544, 128, 80);
        //Space bar needs to be trimmed back so we don't try to draw off buffer
        key space(" ", ' ', 240, 640, 800, 72);
        key bckSpc("Back", ' ', 1120, 256, 128, 80);
        key enter("Enter", ' ', 1120, 352, 128, 80);
        key cancel("Cancel", ' ', 1120, 448, 128, 80);

        keys.push_back(space);
        keys.push_back(shift);
        keys.push_back(bckSpc);
        keys.push_back(enter);
        keys.push_back(cancel);
    }

    keyboard::~keyboard()
    {
        keys.clear();
    }

    void keyboard::draw()
    {
        if(clrAdd)
        {
            clrSh += 4;
            if(clrSh > 63)
                clrAdd = false;
        }
        else
        {
            clrSh--;
            if(clrSh == 0)
                clrAdd = true;
        }

        texDraw(backTemp, frameBuffer, 0, 0);

        clr rectClr = clrCreateRGBA(0x00, 0x60 + clrSh, 0xBB + clrSh, 0xFF);

        //Draw sel rectangle around key for controller
        drawRect(frameBuffer, keys[selKey].getX() - 4, keys[selKey].getY() - 4, keys[selKey].getW() + 8, keys[selKey].getH() + 8, rectClr);

        for(unsigned i = 0; i < keys.size(); i++)
            keys[i].draw();

        drawText(str.c_str(), frameBuffer, ui::shared, 16, 104, 32, ui::mnuTxt);
    }

    std::string keyboard::getString(const std::string& def)
    {
        str = def;

        //Trick to not kill framerate
        backTemp = texCreate(1280, 720);
        memcpy(backTemp->data, frameBuffer->data, 1280 * 720 * sizeof(uint32_t));
        //draw alpha rect over top
        clr topRect = ui::rectSh;
        clr botRect = ui::clearClr;
        topRect.a = 0xDD;
        botRect.a = 0xDD;
        drawRectAlpha(backTemp, 0, 0, 1280, 240, topRect);
        drawRectAlpha(backTemp, 0, 240, 1280, 480, botRect);

        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
            if(down & KEY_R)
                str += util::getDateTime(util::DATE_FMT_YMD);
            else if(down & KEY_L)
                str += util::getDateTime(util::DATE_FMT_YDM);
            else if(down & KEY_ZL)
                str += util::getDateTime(util::DATE_FMT_HOYSTE);

            touchPosition p;
            hidTouchRead(&p, 0);

            //Controller input for keyboard
            if(down & KEY_RIGHT)
                selKey++;
            else if(down & KEY_LEFT && selKey > 0)
                selKey--;
            else if(down & KEY_DOWN)
            {
                selKey += 10;
                if(selKey > 40)
                    selKey = 40;
            }
            else if(down & KEY_UP)
            {
                selKey -= 10;
                if(selKey < 0)
                    selKey = 0;
            }
            else if(down & KEY_A)
            {
                if(selKey < 41)
                {
                    str += keys[selKey].getLet();
                }
            }
            else if(down & KEY_X)
            {
                str += ' ';
            }

            //Stndrd key
            for(unsigned i = 0; i < 41; i++)
            {
                keys[i].update(p);
                if(keys[i].getEvent() == BUTTON_RELEASED)
                {
                    str += keys[i].getLet();
                }
            }

            for(unsigned i = 41; i < 45; i++)
                keys[i].update(p);

            //shift
            if(keys[41].getEvent() == BUTTON_RELEASED || down & KEY_LSTICK)
            {
                if(keys[10].getLet() == 'q')
                {
                    for(unsigned i = 10; i < 41; i++)
                        keys[i].toCaps();
                }
                else
                {
                    for(unsigned i = 10; i < 41; i++)
                        keys[i].toLower();
                }
            }
            //bckspace
            else if(keys[42].getEvent() == BUTTON_RELEASED || down & KEY_Y)
            {
                if(!str.empty())
                    str.erase(str.end() - 1, str.end());
            }
            //enter
            else if(keys[43].getEvent() == BUTTON_RELEASED || down & KEY_PLUS)
                break;
            //cancel
            else if(keys[44].getEvent() == BUTTON_RELEASED || down & KEY_B)
            {
                str.erase(str.begin(), str.end());
                break;
            }

            draw();

            gfxHandleBuffs();
        }

        texDestroy(backTemp);

        return str;
    }
}
