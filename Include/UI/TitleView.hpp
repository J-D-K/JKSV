#pragma once
#include "Data/Data.hpp"
#include "SDL.hpp"
#include "UI/ColorMod.hpp"
#include "UI/Element.hpp"
#include "UI/TitleTile.hpp"
#include <vector>

namespace UI
{
    class TitleView : public UI::Element
    {
        public:
            TitleView(Data::User *User);
            void Update(bool HasFocus);
            void Render(SDL_Texture *Target, bool HasFocus);
            // Returns index of selected title.
            int GetSelected(void) const;
            // Refreshes the view using m_User
            void Refresh(void);
            // Resets all tiles to 128x128
            void Reset(void);

        private:
            // Saves pointer to the user passed.
            Data::User *m_User = nullptr;
            // Y coordinate.
            double m_Y = 28.0f;
            // Currently highlighted/selected title.
            int m_Selected = 0;
            // This is to save the X and Y coordinates so the selected icon can be drawn last.
            double m_SelectedX, m_SelectedY = 28.0f;
            // Color mod for pulse.
            UI::ColorMod m_ColorMod;
            // Vector of tiles.
            std::vector<UI::TitleTile> m_TitleTiles;
    };
} // namespace UI
