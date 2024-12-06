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
