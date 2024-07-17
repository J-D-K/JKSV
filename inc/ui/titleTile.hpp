#pragma once
#include <SDL2/SDL.h>
#include "graphics/graphics.hpp"

namespace ui
{
    class titleTile
    {
        public:
            titleTile(int width, int height, bool favorite, SDL_Texture *icon);

            void update(bool isSelected);
            void render(SDL_Texture *target, int x, int y);

        private:
            // Base width and height of tiles
            int m_Width, m_Height;
            // Render width and height to make icon expand
            int m_RenderWidth, m_RenderHeight;
            // Used to store what to expand to.
            int m_SelectedWidth, m_SelectedHeight;
            // The X and Y to use when rendered
            int m_RenderX, m_RenderY;
            // If it's a favorite and a heart is rendered in top left corner
            bool m_IsFavorite;
            // Point to icon texture
            SDL_Texture *m_Icon;
    };
}