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
        void renderText(const std::string &text, SDL_Texture *target, int x, int y, int fontSize, uint32_t color);
        // Renders text, but wraps to new line if maxWidth is hit or exeeded
        void renderTextWrap(const std::string &text, SDL_Texture *target, int x, int y, int fontSize, int maxWidth, uint32_t color);
        // Returns width of string
        int getTextWidth(const std::string &text, int fontSize);
    }
}
