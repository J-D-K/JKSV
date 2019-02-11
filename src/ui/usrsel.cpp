#include <string>
#include <fstream>
#include <vector>
#include <switch.h>

#include "ui.h"
#include "uiupdate.h"
#include "file.h"

extern std::vector<ui::button> usrNav;

namespace ui
{
    void updateUserMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        //Static so they don't get reset every loop
        static int start = 0, selected = 0;

        static uint8_t clrShft = 0;
        static bool clrAdd = true;

        static unsigned selRectX = 86, selRectY = 91;

        static ui::touchTrack track;

        //Color swapping selBox
        clr clrPrev = clrCreateRGBA(0x00, 0x60 + clrShft, 0xBB + clrShft, 0xFF);

        if(clrAdd)
        {
            clrShft += 6;
            if(clrShft > 63)
                clrAdd = false;
        }
        else
        {
            clrShft--;
            if(clrShft == 0)
                clrAdd = true;
        }

        //Update selBox color
        clr clrUpdt = clrCreateRGBA(0x00, 0x60 + clrShft, 0xBB + clrShft, 0xFF);

        unsigned x = 93, y = 98;
        unsigned endUser = start + 18;
        if(start + 18 > (int)data::users.size())
            endUser = data::users.size();

        texSwapColors(ui::selBox, clrPrev, clrUpdt);
        texDraw(ui::selBox, frameBuffer, selRectX, selRectY);

        for(unsigned i = start; i < endUser; y += 184)
        {
            unsigned endRow = i + 8;
            for(unsigned tX = x; i < endRow; i++, tX += 184)
            {
                if(i == endUser)
                    break;

                if((int)i == selected)
                {
                    if(selRectX != tX - 7 || selRectY != y - 7)
                    {
                        selRectX = tX - 7;
                        selRectY = y - 7;
                    }

                    std::string username = data::users[selected].getUsername();
                    unsigned userWidth = textGetWidth(username.c_str(), ui::shared, 16);
                    int userRectWidth = userWidth + 32, userRectX = (tX + 64) - (userRectWidth  / 2);
                    if(userRectX < 16)
                        userRectX = 16;

                    if(userRectX + userRectWidth > 1264)
                        userRectX = 1264 - userRectWidth;

                    drawTextbox(userRectX, y - 50, userRectWidth, 38);
                    drawText(username.c_str(), frameBuffer, ui::shared, userRectX + 16, y - 38, 16, txtClr);
                }
                data::users[i].drawIconHalf(tX, y);
            }
        }


        //Update invisible buttons
        for(int i = 0; i < 18; i++)
        {
            selButtons[i].update(p);
            if(selected == i && selButtons[i].getEvent() == BUTTON_RELEASED)
            {
                data::curUser = data::users[selected];
                mstate = TTL_SEL;
            }
            else if(selButtons[i].getEvent() == BUTTON_RELEASED)
            {
                if(start + i < (int)data::users.size())
                    selected = start + i;
            }
        }

        //Update nav
        for(unsigned i = 0; i < usrNav.size(); i++)
            usrNav[i].update(p);

        //Update touch tracking
        track.update(p);

        if(down & KEY_RIGHT)
        {
            if(selected < (int)data::users.size() - 1)
                selected++;

            if(selected >= (int)start + 18)
                start += 6;
        }
        else if(down & KEY_LEFT)
        {
            if(selected > 0)
                selected--;

            if(selected < start)
                start -= 6;
        }
        else if(down & KEY_UP)
        {
            selected -= 6;
            if(selected < 0)
                selected = 0;

            if(selected - start >= 18)
                start -= 6;
        }
        else if(down & KEY_DOWN)
        {
            selected += 6;
            if(selected > (int)data::users.size() - 1)
                selected = data::users.size() - 1;

            if(selected - start >= 18)
                start += 6;
        }
        else if(down & KEY_A || usrNav[0].getEvent() == BUTTON_RELEASED)
        {
            data::curUser = data::users[selected];
            //Reset this
            start = 0;
            selected = 0;
            selRectX = 86, selRectY = 91;
            mstate = TTL_SEL;
        }
        else if(down & KEY_Y || usrNav[1].getEvent() == BUTTON_RELEASED)
        {
            for(unsigned i = 0; i < data::users.size(); i++)
                fs::dumpAllUserSaves(data::users[i]);
        }
        else if(down & KEY_X || usrNav[2].getEvent() == BUTTON_RELEASED)
        {
            //Just create file so user doesn't have to constantly enable
            std::fstream cls(fs::getWorkDir() + "cls.txt", std::ios::out);
            cls.close();
            clsUserPrep();
            mstate = CLS_USR;
            clsMode = true;
        }
        else if(down & KEY_MINUS || usrNav[3].getEvent() == BUTTON_RELEASED)
        {
            fsdevUnmountDevice("sv");
            ui::exMenuPrep();
            ui::mstate = EX_MNU;
        }
    }
}
