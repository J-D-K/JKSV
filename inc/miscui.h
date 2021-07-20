#pragma once

#include <vector>
#include <SDL.h>

#include "type.h"
#include "gfx.h"

#define POP_FRAME_DEFAULT 130

#define MENU_FONT_SIZE_DEFAULT 18
#define MENU_MAX_SCROLL_DEFAULT 15

typedef enum
{
    MENU_X,
    MENU_Y,
    MENU_RECT_WIDTH,
    MENU_FONT_SIZE,
    MENU_MAX_SCROLL
} menuParams;

//For smaller classes that aren't easy to get lost in and general functions
namespace ui
{
    typedef struct
    {
        funcPtr func = NULL;
        void *args   = NULL;
        HidNpadButton button = (HidNpadButton)0;
    } menuOptEvent;

    typedef struct
    {
        SDL_Texture *icn = NULL;
        std::string txt;
        std::vector<menuOptEvent> events;
    } menuOpt;

    class menu
    {
        public:
            menu() = default;

            //X, Y, Rect width/Max length, Font size, max to show
            void setParams(const unsigned& _x, const unsigned& _y, const unsigned& _rW, const unsigned& _fS, const unsigned& _mL);
            void editParam(int _param, unsigned newVal);

            //Gets executed when menu changes at all
            void setOnChangeFunc(funcPtr func) { onChange = func; }

            //executed when .update() is called.
            void setCallback(funcPtr _callback, void *args) { callback = _callback; callbackArgs = args; }

            //Adds option.
            int addOpt(SDL_Texture *_icn, const std::string& add);

            //Adds an function to be executed on pressing button specified
            void optAddButtonEvent(unsigned _ind, HidNpadButton _button, funcPtr _func, void *args);
            //Changes opt text
            void editOpt(int ind, SDL_Texture *_icn, const std::string& ch);
            size_t getOptCount() { return opt.size(); }
            int getOptPos(const std::string& txt);

            //Clears menu stuff
            ~menu();

            //Handles controller input and executes functions for buttons if they're set
            void update();

            //Returns selected option
            int getSelected() { return selected; }

            //Draws the menu at x and y. rectWidth is the width of the rectangle drawn under the selected
            void draw(SDL_Texture *target, const SDL_Color *textClr, bool drawText);

            //Clears and resets menu
            void reset();

            //Resets selected + start
            void resetSel() { selected = 0; start = 0; }

            //Enables control/disables drawing select box
            void setActive(bool _set);
            bool getActive() { return isActive; }

        private:
            //drawing x and y + rectangle width/height. Height is calc'd with font size
            int x = 0, y = 0, rW = 0, rY = 0, fSize = 0, rH = 0, mL = 0;
            //Options vector
            std::vector<ui::menuOpt> opt;
            //Selected + frame counting for auto-scroll
            int selected = 0, fc = 0, start = 0;
            //How much we shift the color of the rectangle
            uint8_t clrSh = 0;
            bool clrAdd = true, isActive = true;
            funcPtr onChange = NULL, callback = NULL;
            void *callbackArgs, *funcArgs;
    };

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
    };

    //_draw is called and passed the panel texture/target when this.draw() is called.
    class slideOutPanel
    {
        public:
            slideOutPanel(int _w, int _h, int _y, funcPtr _draw);
            ~slideOutPanel();

            void update();
            void setCallback(funcPtr _cb, void *_args) { callback = _cb; cbArgs = _args; }
            void openPanel() { open = true; }
            void closePanel() { open = false; }
            bool isOpen() { return open; }
            void draw(const SDL_Color *backCol);

        private:
            int w, h, x = 1280, y;
            bool open = false;
            SDL_Texture *panel;
            funcPtr drawFunc, callback = NULL;
            void *cbArgs = NULL;
    };

    class titleTile
    {
        public:
            titleTile(unsigned _w, unsigned _h, bool _fav, SDL_Texture *_icon)
            {
                w = _w;
                h = _h;
                wS = _w;
                hS = _h;
                fav = _fav;
                icon = _icon;
            }

            void draw(SDL_Texture *target, int x, int y, bool sel);

        private:
            unsigned w, h, wS, hS;
            bool fav = false;
            SDL_Texture *icon;
    };

    //Todo less hardcode etc
    class titleview
    {
        public:
            titleview(const data::user& _u, int _iconW, int _iconH, int _horGap, int _vertGap, int _rowCount, funcPtr _callback);
            ~titleview();

            void update();
            void refresh();

            void setActive(bool _set, bool _showSel) { active = _set; showSel = _showSel; }
            bool getActive(){ return active; }
            void setSelected(int _set){ selected = _set; }
            int getSelected(){ return selected; }
            void draw(SDL_Texture *target);

        private:
            const data::user *u;//Might not be safe. Users *shouldn't* be touched after initial load
            bool active = false, showSel = false, clrAdd = true;
            uint8_t clrShft = 0;
            funcPtr callback = NULL;
            int x = 34, y = 69, selected = 0, selRectX = 10, selRectY = 45;
            int iconW, iconH, horGap, vertGap, rowCount;
            std::vector<ui::titleTile *> tiles;
    };

    typedef struct
    {
        std::string message;
        int rectWidth = 0, frames = 0, y = 720;
    } popMessage;

    class popMessageMngr
    {
        public:
            ~popMessageMngr();

            void update();

            void popMessageAdd(const std::string& mess, int frameTime);
            void draw();

        private:
            std::vector<popMessage> popQueue;//All graphics need to be on main thread. Directly adding will cause text issues
            std::vector<popMessage> message;
    };

    class threadProcMngr
    {
        public:
            ~threadProcMngr();
            void newThread(ThreadFunc func, void *args);
            void addThread(threadInfo *t);
            void update();
            void draw();
            bool empty(){ return threads.empty(); }

        private:
            std::vector<threadInfo *> threads;
            uint8_t lgFrame = 0;
            unsigned frameCount = 0;
    };

    //General use
    void showMessage(const char *head, const char *fmt, ...);
    bool confirm(bool hold, const char *fmt, ...);
    bool confirmTransfer(const std::string& f, const std::string& t);
    bool confirmDelete(const std::string& p);
    void drawBoundBox(SDL_Texture *target, int x, int y, int w, int h, uint8_t clrSh);
    void drawTextbox(SDL_Texture *target, int x, int y, int w, int h);
}
