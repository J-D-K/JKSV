#pragma once
#include "SDL.hpp"

// These are just functions to render generic parts of the UI.
namespace UI
{
    void RenderDialogBox(SDL_Texture *Target, int X, int Y, int Width, int Height);
    void RenderBoundingBox(SDL_Texture *Target, int X, int Y, int Width, int Height, uint8_t ColorMod);
} // namespace UI
