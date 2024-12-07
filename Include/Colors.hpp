#pragma once
#include "SDL.hpp"

namespace Colors
{
    static constexpr SDL::Color White = {0xFFFFFFFF};
    static constexpr SDL::Color Black = {0x000000FF};
    static constexpr SDL::Color Red = {0xFF0000FF};
    static constexpr SDL::Color Green = {0x00FF00FF};
    static constexpr SDL::Color Blue = {0x0099EEFF};
    static constexpr SDL::Color Yellow = {0xF8FC00FF};
    static constexpr SDL::Color Pink = {0xFF4444FF};
    static constexpr SDL::Color ClearColor = {0x2D2D2DFF};
    static constexpr SDL::Color DialogBox = {0x505050FF};
    static constexpr SDL::Color BackgroundDim = {0x00000088};
    static constexpr SDL::Color Transparent = {0x00000000};
} // namespace Colors
