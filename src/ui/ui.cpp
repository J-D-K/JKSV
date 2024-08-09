#include <array>
#include <memory>

#include "ui/ui.hpp"

#include "graphics/graphics.hpp"

#include "system/system.hpp"

// Oh boy, here we go~... Nevermind.
namespace
{
    // Paths to UI textures.
    const std::string menuBoundingBoxPath = "romfs:/img/menuBounding.png";
    const std::string dialogBoxPath = "romfs:/img/dialogBox.png";

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
    graphics::sdlTexture s_MenuBoundingBox;
    graphics::sdlTexture s_DialogBox;
}

void ui::init(void)
{
    // Init string system
    ui::strings::init();

    // Init timer for loading glyph
    s_LoadGlyphChangeTimer = std::make_unique<sys::timer>(LOADING_GLYPH_CHANGE_TICKS);
    
    // Load textures
    s_MenuBoundingBox = graphics::textureManager::loadTextureFromFile(menuBoundingBoxPath);
    s_DialogBox = graphics::textureManager::loadTextureFromFile(dialogBoxPath);
}

void ui::renderSelectionBox(SDL_Texture *target, int x, int y, int width, int height, uint8_t colorMod)
{
    // Setup color mod for pulse
    uint8_t red = 0x00;
    uint8_t green = 0x88 + colorMod;
    uint8_t blue =  0xC5 + colorMod / 2;
    uint32_t fullColor = createColor(red, green, blue, 0xFF);

    // Makes stuff easier to type AND READ!!!!!!!!!!!!!!!!!!!!!
    SDL_Texture *boundingBox = s_MenuBoundingBox.get();

    // Mod colors for pulse.
    SDL_SetTextureColorMod(boundingBox, red, green, blue);

    // Top
    graphics::textureRenderPart(boundingBox, target, x, y, 0, 0, 8, 8);
    graphics::renderRect(target, x + 4, y, width - 8, 4, fullColor);
    graphics::textureRenderPart(boundingBox, target, (x + width) - 8, y, 8, 0, 8, 8);
    // Middle
    graphics::renderRect(target, x, y + 4, 4, height - 8, fullColor);
    graphics::renderRect(target, (x + width) - 4, y + 4, 4, height - 8, fullColor);
    // Bottom
    graphics::textureRenderPart(boundingBox, target, x, (y + height) - 8, 0, 8, 8, 8);
    graphics::renderRect(target, x + 4, (y + height) - 4, width - 8, 4, fullColor);
    graphics::textureRenderPart(boundingBox, target, (x + width) - 8, (y + height) - 8, 8, 8, 8, 8);
}

void ui::renderDialogBox(SDL_Texture *target, int x, int y, int width, int height)
{
    // Same here
    SDL_Texture *dialogBox = s_DialogBox.get();

    // Top
    graphics::textureRenderPart(dialogBox, target, x, y, 0, 0, 32, 32);
    graphics::renderRect(target, x + 32, y, width - 64, 32, COLOR_DIALOG_BOX);
    graphics::textureRenderPart(dialogBox, target, (x + width) - 32, y, 32, 0, 32, 32);
    // Middle
    graphics::renderRect(target, x, y + 32, width, height - 64, COLOR_DIALOG_BOX);
    // Bottom
    graphics::textureRenderPart(dialogBox, target, x, (y + height) - 32, 0, 32, 32, 32);
    graphics::renderRect(target, x + 32, (y + height) - 32, width - 64, 32, COLOR_DIALOG_BOX);
    graphics::textureRenderPart(dialogBox, target, (x + width) - 32, (y + height) - 32, 32, 32, 32, 32);
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