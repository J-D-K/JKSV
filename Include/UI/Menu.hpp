#pragma once
#include "SDL.hpp"
#include "UI/ColorMod.hpp"
#include "UI/Element.hpp"
#include <string>
#include <vector>

namespace UI
{
    class Menu : public UI::Element
    {
        public:
            Menu(int X, int Y, int Width, int FontSize, int RenderTargetHeight);
            ~Menu() {};

            void Update(bool HasFocus);
            void Render(SDL_Texture *Target, bool HasFocus);

            void AddOption(std::string_view NewOption);
            int GetSelected(void) const;
            void SetSelected(int Selected);
            void Reset(void);

        protected:
            // X and Y coordinates.
            double m_X, m_Y;
            // Selected option.
            int m_Selected = 0;
            // Color pulse
            UI::ColorMod m_ColorMod;
            // Calculated height of the options.
            int m_OptionHeight;
            // Render target for options so they can't render outside the bounding area.
            SDL::SharedTexture m_OptionTarget = nullptr;

        private:
            // Need to preserve the original Y.
            double m_OriginalY;
            // This is the TargetY the menu catches up to so-to-speak.
            double m_TargetY;
            // This is calculated according to the render target's length.
            int m_ScrollLength;
            // Maximum display length of options.
            int m_Width;
            // Font size.
            int m_FontSize;
            // Vertical size of the destination render target in pixels.
            int m_RenderTargetHeight;
            // Maximum number of display options render target can show.
            int m_MaxDisplayOptions;
            // Vector of options.
            std::vector<std::string> m_Options;
    };
} // namespace UI
