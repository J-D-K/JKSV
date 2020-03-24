#include <string>
#include <vector>

#include <fstream>

#include "ui.h"
#include "uiupdate.h"
#include "file.h"
#include "util.h"
#include "ex.h"

extern std::vector<ui::button> ttlNav;

namespace ui
{
    void updateTitleMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        //Static vars so they don't change on every loop
        //Where to start in titles, selected title
        static int start = 0;

        //Color shift for rect
        static uint8_t clrShft = 0;
        //Whether or not we're adding or subtracting from clrShft
        static bool clrAdd = true;

        //Selected rectangle X and Y.
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

        unsigned endTitle = start + 32;
        if(start + 32 > (int)data::curUser.titles.size())
            endTitle = data::curUser.titles.size();

        //draw Rect so it's always behind icons
        drawBoundBox(selRectX, selRectY, 140, 140, clrShft);

        for(unsigned i = start; i < endTitle; y += 136)
        {
            unsigned endRow = i + 8;
            for(unsigned tX = x; i < endRow; i++, tX += 144)
            {
                if(i == endTitle)
                    break;

                if((int)i == data::selData)
                {
                    selRectX = tX - 6;
                    selRectY = y - 6;

                    std::string title = data::curUser.titles[data::selData].getTitle();
                    unsigned titleWidth = textGetWidth(title.c_str(), ui::shared, 16);
                    int rectWidth = titleWidth + 32, rectX = (tX + 64) - (rectWidth / 2);
                    if(rectX < 16)
                        rectX = 16;

                    if(rectX + rectWidth > 1264)
                        rectX = 1264 - rectWidth;

                    drawTextbox(rectX, y - 50, rectWidth, 38);
                    drawText(title.c_str(), frameBuffer, ui::shared, rectX + 16, y - 38, 16, txtClr);
                }
                data::curUser.titles[i].icon.drawHalf(tX, y);
            }
        }

        //Buttons
        for(int i = 0; i < 32; i++)
        {
            selButtons[i].update(p);
            if(i == data::selData - start && selButtons[i].getEvent() == BUTTON_RELEASED)
            {
                data::curData = data::curUser.titles[data::selData];
                if(fs::mountSave(data::curUser, data::curData))
                {
                    util::makeTitleDir(data::curUser, data::curData);
                    folderMenuPrepare(data::curUser, data::curData);
                    folderMenuInfo = util::getInfoString(data::curUser, data::curData);

                    mstate = FLD_SEL;
                }
            }
            else if(selButtons[i].getEvent() == BUTTON_RELEASED)
            {
                if(start + i < (int)data::curUser.titles.size())
                    data::selData = start + i;
            }
        }

        //Nav
        for(unsigned i = 0; i < ttlNav.size(); i++)
            ttlNav[i].update(p);

        //Update touchtracking
        track.update(p);

        switch(track.getEvent())
        {
            case TRACK_SWIPE_UP:
                {
                    if(start + 32 < (int)data::curUser.titles.size())
                    {
                        start += 8;
                        data::selData += 8;
                        if(data::selData > (int)data::curUser.titles.size() - 1)
                            data::selData = data::curUser.titles.size() - 1;
                    }
                }
                break;

            case TRACK_SWIPE_DOWN:
                {
                    if(start - 8 >= 0)
                    {
                        start -= 8;
                        data::selData -= 8;
                    }
                }
                break;
        }

        if(down & KEY_RIGHT)
        {
            if(data::selData < (int)data::curUser.titles.size() - 1)
                data::selData++;

            if(data::selData >= (int)start + 32)
                start += 8;
        }
        else if(down & KEY_LEFT)
        {
            if(data::selData > 0)
                data::selData--;

            if(data::selData < (int)start)
                start -= 8;
        }
        else if(down & KEY_UP)
        {
            data::selData -= 8;
            if(data::selData < 0)
                data::selData = 0;

            if(data::selData < start)
                start -= 8;
        }
        else if(down & KEY_DOWN)
        {
            data::selData += 8;
            if(data::selData > (int)data::curUser.titles.size() - 1)
                data::selData = data::curUser.titles.size() - 1;

            if(data::selData - start >= 32)
                start += 8;
        }
        else if(down & KEY_A || ttlNav[0].getEvent() == BUTTON_RELEASED)
        {
            data::curData = data::curUser.titles[data::selData];
            if(fs::mountSave(data::curUser, data::curData))
            {
                util::makeTitleDir(data::curUser, data::curData);
                folderMenuPrepare(data::curUser, data::curData);
                folderMenuInfo = util::getInfoString(data::curUser, data::curData);

                mstate = FLD_SEL;
            }
        }
        else if(down & KEY_Y || ttlNav[1].getEvent() == BUTTON_RELEASED)
        {
            fs::dumpAllUserSaves(data::curUser);
        }
        else if(down & KEY_X || ttlNav[2].getEvent() == BUTTON_RELEASED)
        {
            std::string confStr = "Are you 100% sure you want to add \"" + data::curUser.titles[data::selData].getTitle() + \
                                  "\" to your blacklist?";
            if(ui::confirm(confStr))
                data::blacklistAdd(data::curUser, data::curUser.titles[data::selData]);
        }
        else if(down & KEY_B || ttlNav[3].getEvent() == BUTTON_RELEASED)
        {
            start = 0;
            data::selData = 0;
            selRectX = 64;
            selRectY = 90;
            mstate = USR_SEL;
            return;
        }
        else if(down & KEY_L)
        {
            data::selUser--;
            if(data::selUser < 0)
                data::selUser = data::users.size() -1;

            start = 0;
            data::selData = 0;
            selRectX = 64, selRectY = 90;
            data::curUser = data::users[data::selUser];

            ui::showPopup(data::curUser.getUsername(), POP_FRAME_DEFAULT);
        }
        else if(down & KEY_R)
        {
            data::selUser++;
            if(data::selUser > (int)data::users.size() - 1)
                data::selUser = 0;

            start = 0;
            data::selData = 0;
            selRectX = 64, selRectY = 90;
            data::curUser = data::users[data::selUser];

            ui::showPopup(data::curUser.getUsername(), POP_FRAME_DEFAULT);
        }
    }
}
