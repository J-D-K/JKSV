#include "UI/RenderFunctions.hpp"
#include "Colors.hpp"

namespace
{
    SDL::SharedTexture s_DialogCorners = nullptr;
    SDL::SharedTexture s_MenuBoundingCorners = nullptr;
} // namespace

void UI::RenderDialogBox(SDL_Texture *Target, int X, int Y, int Width, int Height)
{
    if (!s_DialogCorners)
    {
        s_DialogCorners = SDL::TextureManager::CreateLoadTexture("DialogCorners", "romfs:/Textures/DialogCorners.png");
    }

    // Top
    s_DialogCorners->RenderPart(Target, X, Y, 0, 0, 16, 16);
    SDL::RenderRectFill(Target, X + 16, Y, Width - 32, 16, Colors::DialogBox);
    s_DialogCorners->RenderPart(Target, (X + Width) - 16, Y, 16, 0, 16, 16);
    // Middle
    SDL::RenderRectFill(NULL, X, Y + 16, Width, Height - 32, Colors::DialogBox);
    // Bottom
    s_DialogCorners->RenderPart(Target, X, (Y + Height) - 16, 0, 16, 16, 16);
    SDL::RenderRectFill(NULL, X + 16, (Y + Height) - 16, Width - 32, 16, Colors::DialogBox);
    s_DialogCorners->RenderPart(NULL, (X + Width) - 16, (Y + Height) - 16, 16, 16, 16, 16);
}

void UI::RenderBoundingBox(SDL_Texture *Target, int X, int Y, int Width, int Height, uint8_t ColorMod)
{
    if (!s_MenuBoundingCorners)
    {
        s_MenuBoundingCorners = SDL::TextureManager::CreateLoadTexture("MenuBoundingCorners", "romfs:/Textures/MenuBounding.png");
    }

    // Setup color.
    SDL::Color RenderMod = {(0x88 + ColorMod) << 16 | (0xC5 + (ColorMod / 2) << 8) | 0xFF};

    // This shouldn't fail, but I don't really care if it does.
    SDL_SetTextureColorMod(s_MenuBoundingCorners->Get(), RenderMod.RGBA[3], RenderMod.RGBA[2], RenderMod.RGBA[1]);

    // Top
    s_MenuBoundingCorners->RenderPart(Target, X, Y, 0, 0, 8, 8);
    SDL::RenderRectFill(Target, X + 8, Y, Width - 16, 4, RenderMod);
    s_MenuBoundingCorners->RenderPart(Target, (X + Width) - 8, Y, 8, 0, 8, 8);
    // Middle
    SDL::RenderRectFill(Target, X, Y + 8, 4, Height - 16, RenderMod);
    SDL::RenderRectFill(Target, (X + Width) - 4, Y + 8, 4, Height - 16, RenderMod);
    // Bottom
    s_MenuBoundingCorners->RenderPart(Target, X, (Y + Height) - 8, 0, 8, 8, 8);
    SDL::RenderRectFill(Target, X + 8, (Y + Height) - 4, Width - 16, 4, RenderMod);
    s_MenuBoundingCorners->RenderPart(Target, (X + Width) - 8, (Y + Height) - 8, 8, 8, 8, 8);
}
