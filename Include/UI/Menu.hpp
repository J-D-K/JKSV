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
            // Coordinates to render to and dimensions of menu.
            Menu(int X, int Y, int Width, int FontSize, int ScrollLength, int RenderTargetHeight);
            ~Menu() {};

            // Required functions from Element.
            void Update(void);
            void Render(SDL_Texture *Target, bool HasFocus);

            // Adds an option to the menu.
            void AddOption(std::string_view NewOption);

            // Returns the index of the selected item.
            int GetSelected(void) const;
            // Sets the selected option
            void SetSelected(int Selected);
            // Resets the menu.
            void Reset(void);

        protected:
            // X and Y coordinates.
            double m_X, m_Y;
            // Font size
            int m_FontSize;
            // Selected option.
            int m_Selected = 0;
            // Color mod for rendering bounding box.
            UI::ColorMod m_ColorMod;
            // Actual length of the menu.
            int m_OptionsLength = -1;
            // Height of options.
            int m_OptionHeight;
            // Total height of menu in pixels.
            int m_MenuHeight = 0;
            // A small target to render the option to so it can't draw text outside of the bounding area.
            SDL::SharedTexture m_OptionTarget = nullptr;

        private:
            // These are used for the scrolling effect.
            double m_OriginalY, m_TargetY;
            // How many options before scrolling starts happening.
            int m_ScrollLength, m_MenuRenderLength;
            // The height of the render target for scrolling calculations.
            int m_RenderTargetHeight = 0;
            // Width.
            int m_Width;
            // Vector of options.
            std::vector<std::string> m_Options;
    };
} // namespace UI
