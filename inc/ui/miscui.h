#pragma once

#include <vector>
#include <SDL2/SDL.h>

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
    MENU_RECT_HEIGHT,
    MENU_FONT_SIZE,
    MENU_MAX_SCROLL
} menuParams;

//For smaller classes that aren't easy to get lost in and general functions
namespace ui
{
    typedef struct
    {
        std::string text;
        bool hold;
        funcPtr confFunc, cancelFunc;
        void *args;
        unsigned lgFrame = 0, frameCount = 0;//To count frames cause I don't have time and am lazy
    } confirmArgs;

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
        int txtWidth;
        std::vector<menuOptEvent> events;
    } menuOpt;

    class menu
    {
        public:
            menu() = default;
            menu(const int& _x, const int& _y, const int& _rW, const int& _fS, const int& _mL);
            void editParam(int _param, int newVal);

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

            //Returns menu option count
            int getCount() { return opt.size(); }

            //Draws the menu at x and y. rectWidth is the width of the rectangle drawn under the selected
            void draw(SDL_Texture *target, const SDL_Color *textClr, bool drawText);

            //Clears and resets menu
            void reset();

            //Resets selected + start
            void resetSel() { selected = 0; }

            //Enables control/disables drawing select box
            void setActive(bool _set);
            bool getActive() { return isActive; }

        private:
            //drawing x and y + rectangle width/height. Height is calc'd with font size
            int x = 0, mY = 0, tY = 0, y = 0, rW = 0, rY = 0, fSize = 0, rH = 0, mL = 0;
            //Options vector
            std::vector<ui::menuOpt> opt;
            //Selected + frame counting for auto-scroll. Hover count is to not break autoscroll
            int selected = 0, fc = 0, hoverCount = 0, spcWidth = 0;
            //How much we shift the color of the rectangle
            uint8_t clrSh = 0;
            bool clrAdd = true, isActive = true, hover = false;
            //Option buffer. Basically, text is draw to this so it can't overlap. Also allows scrolling
            SDL_Texture *optTex;
            funcPtr onChange = NULL, callback = NULL;
            void *callbackArgs, *funcArgs;
    };

    //Progress bar for showing loading. Mostly so people know it didn't freeze
    class progBar
    {
        public:
            //Constructor. _max is the maximum value
            progBar() = default;
            progBar(const uint64_t& _max) { max = _max; }

            void setMax(const uint64_t& _max) { max = _max; };

            //Updates progress
            void update(const uint64_t& _prog);

            //Draws with text at top
            void draw(const std::string& text);

        private:
            uint64_t max = 0, prog = 0;
            float width = 0;
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

    //General use
    ui::confirmArgs *confirmArgsCreate(bool _hold, funcPtr _confFunc, funcPtr _cancelFunc, void *_funcArgs, const char *fmt, ...);
    void confirm(void *a);
    void showMessage(const char *fmt, ...);
    bool confirmTransfer(const std::string& f, const std::string& t);
    bool confirmDelete(const std::string& p);
    void drawBoundBox(SDL_Texture *target, int x, int y, int w, int h, uint8_t clrSh);
    void drawTextbox(SDL_Texture *target, int x, int y, int w, int h);
}
