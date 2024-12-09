#pragma once
#include "UI/Menu.hpp"

namespace UI
{
    // This is a really hard-coded, hacky way of using the Menu's code with icons as options.
    class IconMenu : public UI::Menu
    {
        public:
            IconMenu(int X, int Y, int RenderTargetHeight);
            ~IconMenu() {};

            void Update(bool HasFocus);
            void Render(SDL_Texture *Target, bool HasFocus);

            // Adds icon to menu.
            void AddOption(SDL::SharedTexture NewOption);

        private:
            std::vector<SDL::SharedTexture> m_Options;
    };
} // namespace UI
