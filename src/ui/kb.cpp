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
        key enter("Entr", ' ', 1120, 352, 128, 80);
        key cancel("Cancl", ' ', 1120, 448, 128, 80);

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

        drawRect(frameBuffer, 0, 176, 1280, 64, clrCreateU32(0xFFFFFFFF));
        drawRect(frameBuffer, 0, 240, 1280, 480, clrCreateU32(0xFF2D2D2D));

        clr rectClr = clrCreateRGBA(0x00, 0x60 + clrSh, 0xBB + clrSh, 0xFF);

        //Draw sel rectangle around key for controller
        drawRect(frameBuffer, keys[selKey].getX() - 4, keys[selKey].getY() - 4, keys[selKey].getW() + 8, keys[selKey].getH() + 8, rectClr);

        for(unsigned i = 0; i < keys.size(); i++)
            keys[i].draw();

        drawText(str.c_str(), frameBuffer, ui::shared, 16, 192, 32, clrCreateU32(0xFF000000));
    }

    std::string keyboard::getString(const std::string& def)
    {
        str = def;
        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
            if(down & KEY_R)
                str += util::getDateTime();

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

            for(unsigned i = 41; i < 44; i++)
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

        return str;
    }
}
