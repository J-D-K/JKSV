#ifndef KB_H
#define KB_H

#include <string>
#include <vector>

#include "miscui.h"

namespace ui
{
    //The way it should have been. Inheritance drives me insane sometimes, but they're basically the same thing.
    class key : button
    {
        public:
            key(const std::string& _txt, const char& _l, unsigned _x, unsigned _y, unsigned _w, unsigned _h);

            //Returns char assigned to key
            char getLet() { return let; }

            //Updates displayed text
            void updateText(const std::string& txt);

            //toUpper
            void toCaps();

            //toLower
            void toLower();

            void draw() { button::draw();}
            void update(const touchPosition& p) { button::update(p);}
            int getEvent() { return button::getEvent(); }
            int getX() { return button::getX(); }
            int getY() { return button::getY(); }
            int getW() { return w; }
            int getH() { return h; }

        private:
            char let;
    };

    class keyboard
    {
        public:
            //Builds keyboard
            keyboard();
            ~keyboard();

            void draw();
            //returns string
            std::string getString(const std::string& def);

        private:
            std::vector<key> keys;
            std::string str;
            int selKey = 0;
            uint8_t clrSh = 0;
            bool clrAdd = true;
            tex *backTemp = NULL;
    };
}

#endif // KB_H
