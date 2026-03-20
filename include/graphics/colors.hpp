#pragma once
#include "sdl.hpp"

namespace colors
{
    inline constexpr SDL_Color WHITE             = {0xFF, 0xFF, 0xFF, 0xFF};
    inline constexpr SDL_Color BLACK             = {0x00, 0x00, 0x00, 0xFF};
    inline constexpr SDL_Color RED               = {0xFF, 0x00, 0x00, 0xFF};
    inline constexpr SDL_Color DARK_RED          = {0xDD, 0x00, 0x00, 0xFF};
    inline constexpr SDL_Color GREEN             = {0x00, 0xFF, 0x00, 0xFF};
    inline constexpr SDL_Color BLUE              = {0x00, 0x99, 0xEE, 0xFF};
    inline constexpr SDL_Color YELLOW            = {0xF8, 0xFC, 0x00, 0xFF};
    inline constexpr SDL_Color PINK              = {0xFF, 0x44, 0x44, 0xFF};
    inline constexpr SDL_Color BLUE_GREEN        = {0x00, 0xFF, 0xC5, 0xFF};
    inline constexpr SDL_Color CLEAR_COLOR       = {0x2D, 0x2D, 0x2D, 0xFF};
    inline constexpr SDL_Color CLEAR_PANEL       = {0x0D, 0x0D, 0x0D, 0xFF};
    inline constexpr SDL_Color DIALOG_DARK       = {0x50, 0x50, 0x50, 0xFF};
    inline constexpr SDL_Color DIALOG_LIGHT      = {0xDC, 0xDC, 0xDC, 0xFF};
    inline constexpr SDL_Color DIM_BACKGROUND    = {0x00, 0x06, 0x11, 0x80};
    inline constexpr SDL_Color TRANSPARENT       = {0x00, 0x00, 0x00, 0x00};
    inline constexpr SDL_Color SLIDE_PANEL_CLEAR = {0x00, 0x00, 0x00, 0xCC};
    inline constexpr SDL_Color DIV_COLOR         = {0x70, 0x70, 0x70, 0xFF};
    inline constexpr SDL_Color GUIDE_COLOR       = {0x00, 0x00, 0x00, 0x7F};
    inline constexpr SDL_Color BAR_GREEN         = {0x11, 0xCC, 0x33, 0xFF};
    inline constexpr SDL_Color GOLD              = {0xFF, 0xD7, 0x00, 0xFF};

    inline constexpr uint8_t ALPHA_FADE_BEGIN = 0x00;
    inline constexpr uint8_t ALPHA_FADE_END   = 0x80;
} // namespace colors
