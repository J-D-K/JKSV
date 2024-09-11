#pragma once
#include <vector>
#include <SDL2/SDL.h>
#include "graphics/graphics.hpp"
#include "menu.hpp"

// This is used only for the main/user selection menu and is kind of hacky, but it works
namespace ui
{
    class iconMenu : public menu
    {
        public:
            iconMenu(int x, int y, int maxScroll);

            void addOpt(graphics::sdlTexture newOption);
            void render(SDL_Texture *target);

        private:
            std::vector<graphics::sdlTexture> m_MenuOptions;
    };
}
