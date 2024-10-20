#pragma once

#include <SDL2/SDL.h>

#include "type.h"
#include "data.h"

namespace ui
{
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
            bool getActive() { return active; }
            void setSelected(int _set) { selected = _set; }
            int getSelected() { return selected; }
            void draw(SDL_Texture *target);

        private:
            const data::user *u;//Might not be safe. Users *shouldn't* be touched after initial load
            bool active = false, showSel = false, clrAdd = true;
            uint8_t clrShft = 0;
            funcPtr callback = NULL;
            int x = 200, y = 62, selected = 0, selRectX = 10, selRectY = 45;
            int iconW, iconH, horGap, vertGap, rowCount;
            std::vector<ui::titleTile *> tiles;
    };
}
