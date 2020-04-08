#ifndef MISCUI_H
#define MISCUI_H

#include "gfx.h"

enum buttonEvents
{
    BUTTON_NOTHING,
    BUTTON_PRESSED,
    BUTTON_RELEASED
};

enum trackEvents
{
    TRACK_NOTHING,
    TRACK_SWIPE_UP,
    TRACK_SWIPE_DOWN,
    TRACK_SWIPE_LEFT,
    TRACK_SWIPE_RIGHT
};

#define POP_FRAME_DEFAULT 130

//For smaller classes that aren't easy to get lost in and general functions
namespace ui
{
    //Progress bar for showing loading. Mostly so people know it didn't freeze
    class progBar
    {
        public:
            //Constructor. _max is the maximum value
            progBar(const uint64_t& _max) { max = _max; }

            //Updates progress
            void update(const uint64_t& _prog);

            //Draws with text at top
            void draw(const std::string& text, const std::string& head);

        private:
            uint64_t max, prog;
            float width;
            tex *bg;
    };

    class button
    {
        public:
            button(const std::string& _txt, unsigned _x, unsigned _y, unsigned _w, unsigned _h);
            void setText(const std::string& _txt);
            void update(const touchPosition& p);
            bool isOver();
            bool wasOver();
            int getEvent() { return retEvent; }

            void draw();
            void draw(const clr& _txt);

            unsigned getX() { return x; }
            unsigned getY() { return y; }
            unsigned getTx() { return tx; }
            unsigned getTy() { return ty; }

        protected:
            bool pressed = false, first = false;
            int retEvent = BUTTON_NOTHING;
            unsigned x, y, w, h;
            unsigned tx, ty;
            std::string text;
            touchPosition prev, cur;
    };

    class touchTrack
    {
        public:
            void update(const touchPosition& p);

            int getEvent() { return retTrack; }
            int getOriginX() { return originX; }
            int getOriginY() { return originY; }

        private:
            touchPosition pos[5];
            int retTrack = TRACK_NOTHING;
            int curPos = 0, avX = 0, avY = 0;
            int originX = 0, originY = 0;
    };

    //General use
    void showMessage(const std::string& mess, const std::string& head);
    bool confirm(const std::string& q, bool hold);
    bool confirmTransfer(const std::string& f, const std::string& t);
    bool confirmDelete(const std::string& p);
    void drawTextbox(int x, int y, int w, int h);
    void drawTextboxInvert(int x, int y, int w, int h);

    //Popup from freebird
    void showPopup(const std::string& mess, unsigned frames);
    void drawPopup(const uint64_t& down);
}

#endif // MISCUI_H
