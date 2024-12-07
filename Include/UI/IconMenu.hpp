#pragma once
#include "UI/Menu.hpp"

namespace UI
{
    class IconMenu : public UI::Menu
    {
        public:
            IconMenu(int X, int Y, int ScrollLength);

        private:
            std::vector<SDL::SharedTexture> m_Options;
    };
} // namespace UI
