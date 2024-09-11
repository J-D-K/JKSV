#include <algorithm>
#include <array>
#include <string>
#include <cstring>
#include <cstdio>
#include <vector>
#include <map>

#include <switch.h>
#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphics/graphics.hpp"
#include "graphics/systemFont.hpp"

#include "log.hpp"

//This struct is for caching glyph data needed
typedef struct
{
    uint16_t width, height;
    int advanceX, top, left;
    graphics::sdlTexture glyph;
} glyphData;

namespace
{
    //Freetype lib and faces
    FT_Library s_FTLib;
    FT_Face    s_FTFaces[6];
    //Number of fonts loaded
    int s_TotalFonts;
    // Color used for rendering. Added to prevent duplicating code.
    uint32_t s_RenderingColor;
    // This is the glyph texture cache map
    std::map<std::pair<uint32_t, int>, glyphData> s_GlyphMap;
    // How large the word buffer is for line wrapping.
    const int WORD_WRAP_BUFFER_LENGTH = 0x1000;
    // Codepoints to break lines at
    const std::array<uint32_t, 7> s_BreakPoints = {L' ', L'　', L'/', L'_', L'-', L'。', L'、'};
}

//Resizes faces
static void resizeFont(int fontSize)
{
    for(int i = 0; i < s_TotalFonts; i++)
    {
        FT_Set_Char_Size(s_FTFaces[i], 0, fontSize * 64, 90, 90);
    }
}

//This is for loading a glyph at the current size if it hasn't been already
static FT_GlyphSlot loadGlyph(uint32_t codepoint, FT_Int32 flags)
{
    for(int i = 0; i < s_TotalFonts; i++)
    {
        // Get codepoint index and try to load it.
        FT_UInt codepointIndex = FT_Get_Char_Index(s_FTFaces[i], codepoint);
        FT_Error loadGlyphError = FT_Load_Glyph(s_FTFaces[i], codepointIndex, flags);

        if (codepointIndex != 0 && loadGlyphError == 0)
        {
            return s_FTFaces[i]->glyph;
        }
    }
    return NULL;
}

//Checks map for glyph, if it's not found, loads and converts
static glyphData *getGlyph(uint32_t codepoint, int fontSize)
{
    if(s_GlyphMap.find(std::make_pair(codepoint, fontSize)) != s_GlyphMap.end())
    {
        return &s_GlyphMap[std::make_pair(codepoint, fontSize)];
    }

    //Load glyph and make sure we got the alpha basically
    FT_GlyphSlot glyphSlot = loadGlyph(codepoint, FT_LOAD_RENDER);
    FT_Bitmap bitmap = glyphSlot->bitmap;
    if(bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
    {
        return NULL;
    }

    // This is the size of the glyph bitmap
    int bitmapSize = bitmap.rows * bitmap.width;
    // Pointer to actual pixel buffer
    uint8_t *bitmapPointer = bitmap.buffer;
    // Vector of pixels to fill. I tried writing directly to SDL_Surface->pixels but it was all corrupted.
    std::vector<uint32_t> glyphPixelData(bitmapSize);
    //Loop through and make full pixels in white
    for(int i = 0; i < bitmapSize; i++)
    {
        glyphPixelData.at(i) = (0xFFFFFF00 | *bitmapPointer++);
    }

    // Create Temporary SDL_Surface to convert to texture
    SDL_Surface *tempGlyphSurface = SDL_CreateRGBSurfaceFrom(glyphPixelData.data(), bitmap.width, bitmap.rows, 32, sizeof(uint32_t) * bitmap.width, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    // Texture manager needs name for texture.
    std::string glyphName = std::to_string(codepoint) + "_" + std::to_string(fontSize);

    //Add it to glyph map
    s_GlyphMap[std::make_pair(codepoint, fontSize)] = 
    {
        .width = static_cast<uint16_t>(bitmap.width),
        .height = static_cast<uint16_t>(bitmap.rows),
        .advanceX = static_cast<int>(glyphSlot->advance.x >> 6),
        .top = glyphSlot->bitmap_top,
        .left = glyphSlot->bitmap_left,
        .glyph = graphics::textureManager::createTextureFromSurface(glyphName, tempGlyphSurface)
    };

    //Free surface, job is done
    SDL_FreeSurface(tempGlyphSurface);

    //Return it
    return &s_GlyphMap[std::make_pair(codepoint, fontSize)];
}

//Returns if line breakable character
static bool isBreakableCharacter(uint32_t codepoint)
{
    if(std::find(s_BreakPoints.begin(), s_BreakPoints.end(), codepoint) != s_BreakPoints.end())
    {
        return true;
    }
    return false;
}

//Finds length until next line breakable character
static size_t findNextBreakPoint(const char *str)
{
    uint32_t codepoint = 0;
    int stringLength = strlen(str);
    for(int i = 0; i < stringLength; )
    {
        ssize_t unitCount = decode_utf8(&codepoint, reinterpret_cast<const uint8_t *>(&str[i]));
        i += unitCount;
        if(unitCount <= 0)
        {
            return stringLength;
        }
        else if(isBreakableCharacter(codepoint))
        {
            return i;
        }
    }
    return stringLength;
}

//Processes characters set to change text colors. Returns true if character is special
static bool processSpecialCharacters(uint32_t codepoint, uint32_t originalColor)
{
    // This is set to true and on default, set to false.
    bool isSpecial = true;
    switch (codepoint)
    {
        case L'#':
        {
            s_RenderingColor = s_RenderingColor == originalColor ? COLOR_BLUE : originalColor;
        }
        break;

        case L'*':
        {
            s_RenderingColor = s_RenderingColor == originalColor ? COLOR_RED : originalColor;
        }
        break;

        case L'<':
        {
            s_RenderingColor = s_RenderingColor == originalColor ? COLOR_YELLOW : originalColor;
        }
        break;

        case L'>':
        {
            s_RenderingColor = s_RenderingColor == originalColor ? COLOR_GREEN : originalColor;
        }
        break;

        default:
        {
            isSpecial = false;
        }
        break;
    }

    return isSpecial;
}

bool graphics::systemFont::init(void)
{
    //Need language code
    uint64_t languageCode = 0;

    //To get font addresses and data from switch
    PlFontData sharedFont[6];

    if(R_FAILED(setGetLanguageCode(&languageCode)))
    {
        logger::log("Error getting language code.");
        return false;
    }

    FT_Error freeTypeInit = FT_Init_FreeType(&s_FTLib);
    if(freeTypeInit != 0)
    {
        logger::log("Error initializing FreeType: %i.", freeTypeInit);
        return false;
    }

    if(R_FAILED(plGetSharedFont(languageCode, sharedFont, 6, &s_TotalFonts)))
    {
        logger::log("plGetSharedFont failed.");
        return false;
    }

    for(int i = 0; i < s_TotalFonts; i++)
    {
        FT_Error newFace = FT_New_Memory_Face(s_FTLib, reinterpret_cast<FT_Byte *>(sharedFont[i].address), sharedFont[i].size, 0, &s_FTFaces[i]);
        if(newFace != 0)
        {
            logger::log("Error initializing Freetype face %i: %i.", i, newFace);
            return false;
        }
    }
    logger::log("systemFont::init(): Succeeded.");
    return true;
}

void graphics::systemFont::exit(void)
{
    for(int i = 0; i < s_TotalFonts; i++)
    {
        FT_Done_Face(s_FTFaces[i]);
    }
    FT_Done_FreeType(s_FTLib);

    logger::log("systemFont::exit(): Succeeded.");
}

void graphics::systemFont::renderText(const std::string &text, SDL_Texture *target, int x, int y, int fontSize, uint32_t color)
{
    //X and Y we're working with
    int workingX = x;
    int workingY = y;

    //Color we're working with
    s_RenderingColor = color;

    //Current character/codepoint
    uint32_t codepoint = 0;

    //Count returned by decode_utf8
    ssize_t unitCount = 0;

    //Length of string
    int stringLength = text.length();

    //Resize font
    resizeFont(fontSize);

    for(int i = 0; i < stringLength; )
    {
        unitCount = decode_utf8(&codepoint, reinterpret_cast<const uint8_t *>(&text.c_str()[i]));
        if(unitCount <= 0)
        {
            break;
        }

        i += unitCount;
        if(processSpecialCharacters(codepoint, color))
        {
            continue;
        }
        else if(codepoint == '\n')
        {
            // Break line, reset X, increase Y
            workingX = x;
            workingY += fontSize + 8;
            continue;
        }

        glyphData *glyph = getGlyph(codepoint, fontSize);
        if(glyph)
        {
            // Set color just to be sure
            SDL_SetTextureColorMod(glyph->glyph.get(), getRed(s_RenderingColor), getGreen(s_RenderingColor), getBlue(s_RenderingColor));
            // Render glyph texture
            graphics::textureRender(glyph->glyph.get(), target, workingX + glyph->left, workingY + (fontSize - glyph->top));
            // X update
            workingX += glyph->advanceX;
        }
    }
}

void graphics::systemFont::renderTextWrap(const std::string &text, SDL_Texture *target, int x, int y, int fontSize, int maxWidth, uint32_t color)
{
    // These are the same as above
    int workingX = x;
    int workingY = y;
    int stringLength = text.length();

    // This buffer is used to calculate if the next word should create a line break
    std::array<char, WORD_WRAP_BUFFER_LENGTH> wordBuffer = { 0 };

    resizeFont(fontSize);

    for(int i = 0; i < stringLength; )
    {
        // Find the next char we can break a line at
        size_t nextBreakpoint = findNextBreakPoint(&text.c_str()[i]);

        // Copy string up until that point to wordBuffer
        std::snprintf(wordBuffer.data(), WORD_WRAP_BUFFER_LENGTH, "%s", text.substr(i, nextBreakpoint).c_str());

        // Go to next line if maxWidth is exceeded
        int wordWidth = graphics::systemFont::getTextWidth(wordBuffer.data(), fontSize);
        if(workingX + wordWidth >= x + maxWidth)
        {
            workingX = x;
            workingY += fontSize + 8;
        }
        // Render word
        graphics::systemFont::renderText(wordBuffer.data(), target, workingX, workingY, fontSize, color);

        // Increase workingX  by word width
        workingX += wordWidth;

        // Add length to i
        i += std::strlen(wordBuffer.data());
    }
}

int graphics::systemFont::getTextWidth(const std::string &text, int fontSize)
{
    int textWidth = 0;
    uint32_t codepoint = 0;
    ssize_t unitCount = 0;

    resizeFont(fontSize);

    for(unsigned int i = 0; i < text.length(); )
    {
        unitCount = decode_utf8(&codepoint, reinterpret_cast<const uint8_t *>(&text.c_str()[i]));
        if(unitCount <= 0)
        {
            break;
        }

        i += unitCount;
        if(processSpecialCharacters(codepoint, COLOR_WHITE))
        {
            continue;
        }

        glyphData *glyph = getGlyph(codepoint, fontSize);
        if(glyph)
        {
            textWidth += glyph->advanceX;
        }
    }
    return textWidth;
}