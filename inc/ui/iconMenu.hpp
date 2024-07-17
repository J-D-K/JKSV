#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include "menu.hpp"

// This is used only for the main/user selection menu and is kind of hacky, but it works
namespace ui
{
    class iconMenu : public menu
    {
        public:
            iconMenu(int x, int y, int maxScroll);

            void addOpt(SDL_Texture *newOption);
            void render(SDL_Texture *target);

        private:
            std::vector<SDL_Texture *> m_MenuOptions;
    };
}