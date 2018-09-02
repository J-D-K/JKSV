#ifndef MISCUI_H
#define MISCUI_H

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

//For smaller classes that aren't easy to get lost in and general functions
namespace ui
{
    //Progress bar for showing loading. Mostly so people know it didn't freeze
    class progBar
    {
        public:
            //Constructor. _max is the maximum value
            progBar(const uint64_t& _max);

            //Updates progress
            void update(const uint64_t& _prog);

            //Draws with text at top
            void draw(const std::string& text);

        private:
            uint64_t max, prog;
            float width;
    };

    class button
    {
        public:
            button(const std::string& _txt, unsigned _x, unsigned _y, unsigned _w, unsigned _h);
            void update(const touchPosition& p);
            bool isOver();
            bool wasOver();
            int getEvent() { return retEvent; }

            void draw();

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
    void showMessage(const std::string& mess);
    void showError(const std::string& mess, const Result& r);
    bool confirm(const std::string& q);
    bool confirmTransfer(const std::string& f, const std::string& t);
    bool confirmDelete(const std::string& p);
    void drawTextbox(int x, int y, int w, int h);
    void drawTextboxInvert(int x, int y, int w, int h);
}

#endif // MISCUI_H
