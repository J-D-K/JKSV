#include <string>
#include <vector>
#include <switch.h>

#include "data.h"
#include "ui.h"
#include "uiupdate.h"
#include "file.h"
#include "util.h"

void ui::drawUserMenu()
{
    //Static so they don't get reset every loop
    static int start = 0;

    static uint8_t clrShft = 0;
    static bool clrAdd = true;

    static unsigned selRectX = 276, selRectY = 160;

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

    unsigned x = 280, y = 164;
    unsigned endUser = data::users.size();

    drawBoundBox(selRectX, selRectY, 140, 140, clrShft);

    for(unsigned i = start; i < endUser; y += 136)
    {
        unsigned endRow = i + 5;
        for(unsigned tX = x; i < endRow; i++, tX += 144)
        {
            if(i == endUser)
                break;

            if((int)i == data::selUser)
            {
                selRectX = tX - 6;
                selRectY = y - 6;

                unsigned userWidth = gfx::getTextWidth(data::curUser.getUsername().c_str(), 18);
                int userRectWidth = userWidth + 32, userRectX = (tX + 64) - (userRectWidth  / 2);
                if(userRectX < 16)
                    userRectX = 16;
                else if(userRectX + userRectWidth > 1264)
                    userRectX = 1264 - userRectWidth;

                drawTextbox(userRectX, y - 50, userRectWidth, 38);
                gfx::drawTextf(18, userRectX + 16, y - 40, &ui::txtDiag, data::curUser.getUsername().c_str());
            }
            data::users[i].drawIconHalf(tX, y);
        }
    }
}

void ui::updateUserMenu(const uint64_t& down, const uint64_t& held)
{
    switch(down)
    {
        case HidNpadButton_A:
            if(data::curUser.titles.size() > 0)
                ui::changeState(TTL_SEL);
            else
                ui::showPopup(POP_FRAME_DEFAULT, ui::noSavesFound.c_str(), data::curUser.getUsername().c_str());
            break;

        case HidNpadButton_X:
            ui::textMode = true;
            ui::changeState(TXT_USR);
            break;

        case HidNpadButton_Y:
            {
                bool cont = true;
                for(unsigned i = 0; i < data::users.size() - 2; i++)
                {
                    if(cont)
                        cont = fs::dumpAllUserSaves(data::users[i]);
                }
            }
            break;

        case HidNpadButton_R:
            util::checkForUpdate();
            break;

        case HidNpadButton_ZR:
            ui::changeState(EX_MNU);
            break;

        case HidNpadButton_Minus:
            ui::changeState(OPT_MNU);
            break;

        case HidNpadButton_StickLUp:
        case HidNpadButton_Up:
            data::selUser - 5 < 0 ? data::selUser = 0 : data::selUser -= 4;
            break;

        case HidNpadButton_StickLDown:
        case HidNpadButton_Down:
            data::selUser + 5 > (int)data::users.size() - 1 ? data::selUser = data::users.size() - 1 : data::selUser += 5;
            break;

        case HidNpadButton_StickLLeft:
        case HidNpadButton_Left:
            if(data::selUser > 0)
                --data::selUser;
            break;

        case HidNpadButton_StickLRight:
        case HidNpadButton_Right:
            if(data::selUser < (int)data::users.size() - 1)
                ++data::selUser;
            break;
    }
}
