#pragma once
#include "SDL.hpp"

namespace UI
{
    class TitleTile
    {
        public:
            TitleTile(bool IsFavorite, SDL::SharedTexture Icon);

            void Update(bool IsSelected);
            void Render(SDL_Texture *Target, int X, int Y);

        private:
            // Width and height so icon can "expand" when highlighted.
            int m_RenderWidth = 128, m_RenderHeight = 128;
            // Whether or not the title is a favorite
            bool m_IsFavorite = false;
            // Icon
            SDL::SharedTexture m_Icon = nullptr;
    };
} // namespace UI
