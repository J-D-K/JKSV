#pragma once
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include "graphics/graphics.hpp"

namespace ui
{
    class menu
    {
        public:
            // First is a blank menu, second is initialized using array of strings
            menu(const int &x, const int &y, const int &rectWidth, const int &fontSize, const int &maxScroll);
            menu(const int &x, const int &y, const int &rectWidth, const int &fontSize, const int &maxScroll, const std::string *menuOpts, const int &menuOptCount);
            ~menu();

            // Updates menu
            void update(void);
            // Renders menu to target
            void render(SDL_Texture *target);
            // Adds option
            void addOpt(const std::string &newOpt);
            // Updates the color pulsing variables
            void updateColorPulse(void);
            // Returns currently selected option
            int getSelected(void) const;
            // Sets selected option to newSelected
            void setSelected(const int &newSelected);
            // Clears all options from menu
            void clearMenu(void);

        protected:
            // X and Y coordinates
            int m_X, m_Y;
            // These are used for scrolling effect
            int m_OriginalY, m_TargetY;
            // Font size to use
            int m_FontSize;
            // Selection rectangle width and height
            int m_RectWidth, m_RectHeight;
            // Selected option
            int m_Selected;
            // Maximum options for scrolling occurs
            int m_MaxScroll;
            // Color shifting. True = add, false = subtract
            bool m_ColorShift = true;
            // Amount to add/subtract from color to pulse
            int m_ColorMod = 0;

        private:
            // Vector of options
            std::vector<std::string> m_MenuOptions;
    };
}