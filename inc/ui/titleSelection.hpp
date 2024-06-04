#pragma once
#include <vector>
#include <SDL2/SDL.h>
#include "data/data.hpp"
#include "ui/titleTile.hpp"

namespace ui
{
    class titleSelection
    {
        public:
            titleSelection(data::user *currentUser);
            ~titleSelection();

            int getSelected(void);

            void update(void);
            void render(SDL_Texture *target);

        private:
            // Pointer to current user to load/reload
            data::user *m_CurrentUser;
            // X and Y to render tiles to
            int m_X, m_Y;
            // Target X and Y (???)
            int m_TargetX, m_TargetY;
            // Currently selected title
            int m_Selected;
            // X and Y to render selection box
            int m_SelectionX, m_SelectionY;
            // Width and height of icons
            int m_IconWidth, m_IconHeight;
            // Gaps between icons
            int m_HorizontalGap, m_VerticalGap;
            // How long a row is
            int m_RowSize;
            // Color shifting for selection box
            bool m_ColorShift;
            // How much color is modified
            int m_ColorMod = 0;
            // Vector of tiles
            std::vector<titleTile> m_TitleTiles;
    };
}