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

    //General use
    void showMessage(const char *head, const char *fmt, ...);
    bool confirm(bool hold, const char *fmt, ...);
    bool confirmTransfer(const std::string& f, const std::string& t);
    bool confirmDelete(const std::string& p);
    void drawTextbox(int x, int y, int w, int h);
    void drawTextboxInvert(int x, int y, int w, int h);

    //Popup from freebird
    void showPopup(const std::string& mess, unsigned frames);
    void drawPopup(const uint64_t& down);
}

#endif // MISCUI_H
