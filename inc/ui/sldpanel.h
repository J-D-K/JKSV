#pragma once

namespace ui
{
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
}
