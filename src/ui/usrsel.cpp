#include <string>
#include <vector>
#include <switch.h>

#include "data.h"
#include "ui.h"
#include "uiupdate.h"
#include "file.h"

namespace ui
{
    void updateUserMenu(const uint64_t& down, const uint64_t& held)
    {
        //Static so they don't get reset every loop
        static int start = 0;

        static uint8_t clrShft = 0;
        static bool clrAdd = true;

        static unsigned selRectX = 364, selRectY = 160;

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

        unsigned x = 368, y = 164;
        unsigned endUser = data::users.size();

        drawBoundBox(selRectX, selRectY, 140, 140, clrShft);

        for(unsigned i = start; i < endUser; y += 136)
        {
            unsigned endRow = i + 4;
            for(unsigned tX = x; i < endRow; i++, tX += 144)
            {
                if(i == endUser)
                    break;

                if((int)i == data::selUser)
                {
                    selRectX = tX - 6;
                    selRectY = y - 6;

                    std::string username = data::users[data::selUser].getUsername();
                    unsigned userWidth = textGetWidth(username.c_str(), ui::shared, 18);
                    int userRectWidth = userWidth + 32, userRectX = (tX + 64) - (userRectWidth  / 2);
                    if(userRectX < 16)
                        userRectX = 16;
                    else if(userRectX + userRectWidth > 1264)
                        userRectX = 1264 - userRectWidth;

                    drawTextbox(frameBuffer, userRectX, y - 50, userRectWidth, 38);
                    drawText(username.c_str(), frameBuffer, ui::shared, userRectX + 16, y - 40, 18, ui::txtDiag);
                }
                data::users[i].drawIconHalf(tX, y);
            }
        }

        if(down & KEY_RIGHT && data::selUser < (int)data::users.size() - 1)
                data::selUser++;
        else if(down & KEY_LEFT && data::selUser > 0)
                data::selUser--;
        else if(down & KEY_UP)
        {
            data::selUser -= 4;
            if(data::selUser < 0)
                data::selUser = 0;
        }
        else if(down & KEY_DOWN)
        {
            data::selUser += 4;
            if(data::selUser > (int)data::users.size() - 1)
                data::selUser = data::users.size() - 1;
        }
        else if(down & KEY_A)
        {
            if(data::users[data::selUser].titles.size() > 0)
            {
                data::curUser = data::users[data::selUser];
                mstate = TTL_SEL;
            }
            else
                ui::showPopup("No Saves available for " + data::users[data::selUser].getUsername() + ".", POP_FRAME_DEFAULT);
        }
        else if(down & KEY_Y)
        {
            bool cont = true;
            for(unsigned i = 0; i < data::users.size() - 2; i++)
            {
                if(cont)
                    cont = fs::dumpAllUserSaves(data::users[i]);
            }
        }
        else if(down & KEY_X)
        {
            ui::textMode = true;
            mstate = TXT_USR;
        }
        else if(down & KEY_ZR)
        {
            fs::unmountSave();
            ui::mstate = EX_MNU;
        }
        else if(down & KEY_MINUS)
        {
            ui::mstate = OPT_MNU;
        }
    }
}
