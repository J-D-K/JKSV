#include <array>
#include <memory>
#include "ui/ui.hpp"
#include "graphics/graphics.hpp"
#include "system/system.hpp"

// Oh boy, here we go~
namespace
{
    // Selection bounding box names
    const std::string SELECTION_TOP_LEFT_NAME = "s_SelectionTopLeft";
    const std::string SELECTION_TOP_RIGHT_NAME = "s_SelectionTopRight";
    const std::string SELECTION_BOTTOM_LEFT_NAME = "s_SelectionBottomLeft";
    const std::string SELECTION_BOTTOM_RIGHT_NAME = "s_SelectionBottomRight";

    // Dialog box paths
    const std::string DIALOG_TOP_LEFT_NAME = "s_DialogTopLeft";
    const std::string DIALOG_TOP_RIGHT_NAME = "s_DialogTopRight";
    const std::string DIALOG_BOTTOM_LEFT_NAME = "s_DialogBottomLeft";
    const std::string DIALOG_BOTTOM_RIGHT_NAME = "s_DialogBottomRight";

    // Selection paths
    const std::string SELECTION_TOP_LEFT_PATH = "romfs:/img/menu/selectionTopLeft.png";
    const std::string SELECTION_TOP_RIGHT_PATH = "romfs:/img/menu/selectionTopRight.png";
    const std::string SELECTION_BOTTOM_LEFT_PATH = "romfs:/img/menu/selectionBottomLeft.png";
    const std::string SELECTION_BOTTOM_RIGHT_PATH = "romfs:/img/menu/selectionBottomRight.png";

    // Dialog paths
    const std::string DIALOG_TOP_LEFT_PATH = "romfs:/img/dialogDark/dialogTopLeft.png";
    const std::string DIALOG_TOP_RIGHT_PATH = "romfs:/img/dialogDark/dialogTopRight.png";
    const std::string DIALOG_BOTTOM_LEFT_PATH = "romfs:/img/dialogDark/dialogBottomLeft.png";
    const std::string DIALOG_BOTTOM_RIGHT_PATH = "romfs:/img/dialogDark/dialogBottomRight.png";

    // Ticks for loading glyph changing
    const int LOADING_GLYPH_CHANGE_TICKS = 50;

    // Loading Glyph render coordinates
    const int LOADING_GLYPH_RENDER_X = 56;
    const int LOADING_GLYPH_RENDER_Y = 673;
    const int LOADING_GLYPH_RENDER_SIZE = 32;

    // Nintendo Wii or 3DS? loading glyph array
    const std::array<std::string, 8> s_LoadGlyphArray = 
    {
        "\ue020", "\ue021", "\ue022", "\ue023",
        "\ue024", "\ue025", "\ue026", "\ue027"
    };

    // Timer for changing glyph. Might wanna rethink this some time
    std::unique_ptr<sys::timer> s_LoadGlyphChangeTimer;
    // Frame the animation is on
    int s_LoadGlyphFrame = 0;
    // This is just a bool for whether we're adding or substracting from colors
    bool s_GlyphColorModifier = true;
    // This is the how much to add to the base color for the pulse
    uint8_t s_GlyphColorModification = 0x00;

    // These are all corner textures for rendering various things on screen
    SDL_Texture *s_SelectionTopLeft, *s_SelectionTopRight, *s_SelectionBottomLeft, *s_SelectionBottomRight;
    SDL_Texture *s_DialogTopLeft, *s_DialogTopRight, *s_DialogBottomLeft, *s_DialogBottomRight;
}

void ui::init(void)
{
    // Init string system
    ui::strings::init();

    // Init timer for loading glyph
    s_LoadGlyphChangeTimer = std::make_unique<sys::timer>(LOADING_GLYPH_CHANGE_TICKS);

    // Corners for selection box
    s_SelectionTopLeft = graphics::textureLoadFromFile(SELECTION_TOP_LEFT_NAME, SELECTION_TOP_LEFT_PATH);
    s_SelectionTopRight = graphics::textureLoadFromFile(SELECTION_TOP_RIGHT_NAME, SELECTION_TOP_RIGHT_PATH);
    s_SelectionBottomLeft = graphics::textureLoadFromFile(SELECTION_BOTTOM_LEFT_NAME, SELECTION_BOTTOM_LEFT_PATH);
    s_SelectionBottomRight = graphics::textureLoadFromFile(SELECTION_BOTTOM_RIGHT_NAME, SELECTION_BOTTOM_RIGHT_PATH);

    // Corners for dialog
    s_DialogTopLeft = graphics::textureLoadFromFile(DIALOG_TOP_LEFT_NAME, DIALOG_TOP_LEFT_PATH);
    s_DialogTopRight = graphics::textureLoadFromFile(DIALOG_TOP_RIGHT_NAME, DIALOG_TOP_RIGHT_PATH);
    s_DialogBottomLeft = graphics::textureLoadFromFile(DIALOG_BOTTOM_LEFT_NAME, DIALOG_BOTTOM_LEFT_PATH);
    s_DialogBottomRight = graphics::textureLoadFromFile(DIALOG_BOTTOM_RIGHT_NAME, DIALOG_BOTTOM_RIGHT_PATH);
}

void ui::renderSelectionBox(SDL_Texture *target, int x, int y, int width, int height, uint8_t colorMod)
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

void ui::renderDialogBox(SDL_Texture *target, int x, int y, int width, int height)
{
    graphics::textureRender(s_DialogTopLeft, target, x, y);
    graphics::renderRect(target, x + 32, y, width - 64, 32, COLOR_DIALOG_BOX);
    graphics::textureRender(s_DialogTopRight, target, (x + width) - 32, y);
    graphics::renderRect(target, x, y + 32, width, height - 64, COLOR_DIALOG_BOX);
    graphics::textureRender(s_DialogBottomLeft, target, x, (y + height) - 32);
    graphics::renderRect(target, x + 32, (y + height) - 32, width - 64, 32, COLOR_DIALOG_BOX);
    graphics::textureRender(s_DialogBottomRight, target, (x + width) - 32, (y + height) - 32);
}

void ui::renderLoadingGlyph(void)
{
    if(s_LoadGlyphChangeTimer->triggered() && ++s_LoadGlyphFrame == 8)
    {
        s_LoadGlyphFrame = 0;
    }

    // Check if we've reached the higher target of the color
    if(s_GlyphColorModifier && (s_GlyphColorModification += 6) >= 0x72)
    {
        s_GlyphColorModifier = false;
    }
    else if((s_GlyphColorModification -= 3) <= 0x00)
    {
        s_GlyphColorModifier = true;
    }

    // Create color. For semi-readability
    uint32_t loadingGlyphColor = createColor(0x00, (0x88 + s_GlyphColorModification), (0xC5 + (s_GlyphColorModification / 2)), 0xFF);
    // Finally render it to screen
    graphics::systemFont::renderText(s_LoadGlyphArray.at(s_LoadGlyphFrame), NULL, LOADING_GLYPH_RENDER_X, LOADING_GLYPH_RENDER_Y, LOADING_GLYPH_RENDER_SIZE, loadingGlyphColor);
}