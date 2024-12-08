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

            // Returns index of selected title.
            int GetSelected(void) const;

            void Update(void);
            void Render(SDL_Texture *Target);

        private:
            // Saves pointer to the user passed.
            Data::User *m_User = nullptr;
            // Y coordinate.
            double m_Y = 38;
            // Target Y for scrolling effect.
            double m_TargetY = 38;
            // Currently highlighted/selected title.
            int m_Selected = 0;
            // This is to save the X and Y coordinates so the selected icon can be drawn last.
            int m_SelectedX, m_SelectedY;
            // Color mod for pulse.
            UI::ColorMod m_ColorMod;
            // Vector of tiles.
            std::vector<UI::TitleTile> m_TitleTiles;
    };
} // namespace UI
