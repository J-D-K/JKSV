#include <string>
#include <vector>
#include <switch.h>

#include "data.h"
#include "ui.h"
#include "uiupdate.h"
#include "file.h"

extern std::vector<ui::button> usrNav;

namespace ui
{
    void updateUserMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        //Static so they don't get reset every loop
        static int start = 0;

        static uint8_t clrShft = 0;
        static bool clrAdd = true;

        static unsigned selRectX = 66, selRectY = 94;

        static ui::touchTrack track;

        if(clrAdd)
        {
            clrShft += 6;
            if(clrShft >= 0x72)
                clrAdd = false;
        }
        else
        {
            clrShft -= 3;
            if(clrShft <= 0)
                clrAdd = true;
        }

        unsigned x = 70, y = 98;
        unsigned endUser = start + 32;
        if(start + 32 > (int)data::users.size())
            endUser = data::users.size();

        drawBoundBox(selRectX, selRectY, 140, 140, clrShft);

        for(unsigned i = start; i < endUser; y += 136)
        {
            unsigned endRow = i + 8;
            for(unsigned tX = x; i < endRow; i++, tX += 144)
            {
                if(i == endUser)
                    break;

                if((int)i == data::selUser)
                {
                    selRectX = tX - 6;
                    selRectY = y - 6;

                    std::string username = data::users[data::selUser].getUsername();
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
        for(int i = 0; i < 32; i++)
        {
            selButtons[i].update(p);
            if(data::selUser == i && selButtons[i].getEvent() == BUTTON_RELEASED)
            {
                data::curUser = data::users[data::selUser];
                mstate = TTL_SEL;
            }
            else if(selButtons[i].getEvent() == BUTTON_RELEASED)
            {
                if(start + i < (int)data::users.size())
                    data::selUser = start + i;
            }
        }

        //Update nav
        for(unsigned i = 0; i < usrNav.size(); i++)
            usrNav[i].update(p);

        //Update touch tracking
        track.update(p);

        if(down & KEY_RIGHT)
        {
            if(data::selUser < (int)data::users.size() - 1)
                data::selUser++;

            if(data::selUser >= (int)start + 32)
                start += 8;
        }
        else if(down & KEY_LEFT)
        {
            if(data::selUser > 0)
                data::selUser--;

            if(data::selUser < start)
                start -= 8;
        }
        else if(down & KEY_UP)
        {
            data::selUser -= 8;
            if(data::selUser < 0)
                data::selUser = 0;

            if(data::selUser - start >= 32)
                start -= 8;
        }
        else if(down & KEY_DOWN)
        {
            data::selUser += 8;
            if(data::selUser > (int)data::users.size() - 1)
                data::selUser = data::users.size() - 1;

            if(data::selUser - start >= 32)
                start += 8;
        }
        else if(down & KEY_A || usrNav[0].getEvent() == BUTTON_RELEASED)
        {
            data::curUser = data::users[data::selUser];
            mstate = TTL_SEL;
        }
        else if(down & KEY_Y || usrNav[1].getEvent() == BUTTON_RELEASED)
        {
            for(unsigned i = 0; i < data::users.size() - 3; i++)
                fs::dumpAllUserSaves(data::users[i]);
        }
        else if(down & KEY_X || usrNav[2].getEvent() == BUTTON_RELEASED)
        {
            FILE *cls = fopen(std::string(fs::getWorkDir() + "cls.txt").c_str(), "w");
            fclose(cls);
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
