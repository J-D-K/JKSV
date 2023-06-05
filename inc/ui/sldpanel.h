#pragma once

namespace ui
{
    //Maybe more later *if* needed, but not now.
    typedef enum
    {
        SLD_LEFT,
        SLD_RIGHT
    } slidePanelOrientation;

    //_draw is called and passed the panel texture/target when this.draw() is called.
    class slideOutPanel
    {
        public:
            slideOutPanel(int _w, int _h, int _y, slidePanelOrientation _side, funcPtr _draw);

            void resizePanel(int _w, int _h, int _y);
            void update();
            void setCallback(funcPtr _cb, void *_args) { callback = _cb; cbArgs = _args; }
            void openPanel() { open = true; }
            void closePanel() { open = false; }
            void setX(int _nX){ x = _nX; };
            bool isOpen() { return open; }
            void draw(const SDL_Color *backCol);

        private:
            int w, h, x, y;
            uint8_t sldSide;
            bool open = false;
            SDL_Texture *panel;
            funcPtr drawFunc, callback = NULL;
            void *cbArgs = NULL;
    };
}
