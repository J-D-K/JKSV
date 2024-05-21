#pragma once
#include <string>
#include <SDL2/SDL.h>

namespace graphics
{
    namespace systemFont
    {
        // Inits freetype2
        bool init(void);
        void exit(void);

        // Renders text with system font
        void renderText(const std::string &text, SDL_Texture *target, const int &x, const int &y, const int &fontSize, const uint32_t &color);
        // Renders text, but wraps to new line if maxWidth is hit or exeeded
        void renderTextWrap(const std::string &text, SDL_Texture *target, const int &x, const int &y, const int &fontSize, const int &maxWidth, const uint32_t &color);
        // Returns width of string
        int getTextWidth(const std::string &text, const int &fontSize);
    }
}
