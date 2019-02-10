#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>

#include "gfx.h"
#include "miscui.h"

enum menuTouch
{
    MENU_NOTHING,
    MENU_DOUBLE_REL
};

namespace ui
{
    void menuPrepGfx();
    void menuDestGfx();

    class menu
    {
        public:
            void setParams(const unsigned& _x, const unsigned& _y, const unsigned& _rW);
            //Adds option
            void addOpt(const std::string& add);
            //Changes opt text
            void editOpt(int ind, const std::string& ch);
            //Clears menu stuff
            ~menu();

            //Handles controller input
            void handleInput(const uint64_t& down, const uint64_t& held, const touchPosition& p);

            //Returns selected option
            int getSelected() { return selected; }

            //Returns touch event from buttons
            int getTouchEvent() { return retEvent; }

            //Draws the menu at x and y. rectWidth is the width of the rectangle drawn under the selected
            void draw(const clr& textClr);

            //Clears and resets menu
            void reset();

            //Resets selected + start
            void resetSel(){ selected = 0; start = 0; }

            //Adjusts things after changes are made
            void adjust();

        private:
            //drawing x and y + rectangle width
            unsigned x = 0, y = 0, rW = 0, rY = 0;
            //Options vector
            std::vector<std::string> opt;
            //Selected + frame counting for auto-scroll
            int selected = 0, fc = 0, start = 0, retEvent = MENU_NOTHING;
            //How much we shift the color of the rectangle
            uint8_t clrSh = 0;
            bool clrAdd = true;
            bool separate = true;

            ui::touchTrack track;
            std::vector<ui::button> optButtons;
    };
}

#endif
