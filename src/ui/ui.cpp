#include "ui/ui.hpp"
#include "graphics/graphics.hpp"

namespace
{
    // These are all corner textures for rendering various things on screen
    SDL_Texture *s_SelectionTopLeft, *s_SelectionTopRight, *s_SelectionBottomLeft, *s_SelectionBottomRight;
    SDL_Texture *s_DialogTopLeft, *s_DialogTopRight, *s_DialogBottomLeft, *s_DialogBottomRight;
}


void ui::init(void)
{
    // Init string system
    ui::strings::init();

    // Corners for selection box
    s_SelectionTopLeft = graphics::textureLoadFromFile("s_SelectionTopLeft", "romfs:/img/menu/selectionTopLeft.png");
    s_SelectionTopRight = graphics::textureLoadFromFile("s_SelectionTopRight", "romfs:/img/menu/selectionTopRight.png");
    s_SelectionBottomLeft = graphics::textureLoadFromFile("s_SelectionBottomLeft", "romfs:/img/menu/selectionBottomLeft.png");
    s_SelectionBottomRight = graphics::textureLoadFromFile("s_SelectionBottomRight", "romfs:/img/menu/selectionBottomRight.png");

    // Corners for dialog
    s_DialogTopLeft = graphics::textureLoadFromFile("s_DialogTopLeft", "romfs:/img/dialogDark/dialogTopLeft.png");
    s_DialogTopRight = graphics::textureLoadFromFile("s_DialogTopRight", "romfs:/img/dialogDark/dialogTopRight.png");
    s_DialogBottomLeft = graphics::textureLoadFromFile("s_DialogBottomLeft", "romfs:/img/dialogDark/dialogBottomLeft.png");
    s_DialogBottomRight = graphics::textureLoadFromFile("s_DialogBottomRight", "romfs:/img/dialogDark/dialogBottomRight.png");
}

void ui::renderSelectionBox(SDL_Texture *target, const int &x, const int &y, const int &width, const int &height, const uint8_t &colorMod)
{
    // Setup color mod for pulse
    uint8_t red = 0x00;
    uint8_t green = 0x88 + colorMod;
    uint8_t blue =  0xC5 + colorMod / 2;
    uint32_t fullColor = createColor(red, green, blue, 0xFF);

    SDL_SetTextureColorMod(s_SelectionTopLeft, red, green, blue);
    SDL_SetTextureColorMod(s_SelectionTopRight, red, green, blue);
    SDL_SetTextureColorMod(s_SelectionBottomLeft, red, green, blue);
    SDL_SetTextureColorMod(s_SelectionBottomRight, red, green, blue);

    graphics::textureRender(s_SelectionTopLeft, target, x, y);
    graphics::renderRect(target, x + 4, y, width - 8, 4, fullColor);
    graphics::textureRender(s_SelectionTopRight, target, (x + width) - 4, y);
    graphics::renderRect(target, x, y + 4, 4, height - 8, fullColor);
    graphics::renderRect(target, (x + width) - 4, y + 4, 4, height - 8, fullColor);
    graphics::textureRender(s_SelectionBottomLeft, target, x, (y + height) - 4);
    graphics::renderRect(target, x + 4, (y + height) - 4, width - 8, 4, fullColor);
    graphics::textureRender(s_SelectionBottomRight, target, (x + width) - 4, (y + height) - 4);
}

void ui::renderDialogBox(SDL_Texture *target, const int &x, const int &y, const int &width, const int &height)
{
    graphics::textureRender(s_DialogTopLeft, target, x, y);
    graphics::renderRect(target, x + 32, y, width - 64, 32, COLOR_DIALOG_BOX);
    graphics::textureRender(s_DialogTopRight, target, (x + width) - 32, y);
    graphics::renderRect(target, x, y + 32, width, height - 64, COLOR_DIALOG_BOX);
    graphics::textureRender(s_DialogBottomLeft, target, x, (y + height) - 32);
    graphics::renderRect(target, x + 32, (y + height) - 32, width - 64, 32, COLOR_DIALOG_BOX);
    graphics::textureRender(s_DialogBottomRight, target, (x + width) - 32, (y + height) - 32);
} 