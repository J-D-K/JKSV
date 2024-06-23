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

// This is how large the buffer is for words for line wrapping
static const int WORD_WRAP_BUFFER_LENGTH = 0x1000;

//Masks for converting glyphs to sdl surface
static const uint32_t s_RedMask   = 0xFF000000;
static const uint32_t s_GreenMask = 0x00FF0000;
static const uint32_t s_BlueMask  = 0x0000FF00;
static const uint32_t s_AlphaMask = 0x000000FF;

//Codepoints to break a line at
static const uint32_t s_BreakPoints[7] = {' ', L'　', '/', '_', '-', L'。', L'、'};

//This struct is for caching glyph data needed
typedef struct
{
    uint16_t width, height;
    int advanceX, top, left;
    SDL_Texture *glyph;
} glyphData;

namespace
{
    //Freetype lib and faces
    FT_Library s_FTLib;
    FT_Face    s_FTFaces[6];
    //Number of fonts loaded
    int s_TotalFonts;
    std::map<std::pair<uint32_t, int>, glyphData> s_GlyphMap;
}

//Resizes faces
static void resizeFont(const int &fontSize)
{
    for(int i = 0; i < s_TotalFonts; i++)
    {
        FT_Set_Char_Size(s_FTFaces[i], 0, fontSize * 64, 90, 90);
    }
}

//This is for loading a glyph at the current size if it hasn't been already
static FT_GlyphSlot loadGlyph(const uint32_t &c, const FT_Int32 &flags)
{
    for(int i = 0; i < s_TotalFonts; i++)
    {
        FT_UInt index = 0;
        if ( (index = FT_Get_Char_Index(s_FTFaces[i], c)) != 0 && FT_Load_Glyph(s_FTFaces[i], index, flags) == 0 )
        {
            return s_FTFaces[i]->glyph;
        }
    }
    return NULL;
}

//Checks map for glyph, if it's not found, loads and converts
static glyphData *getGlyph(const uint32_t& c, const int &fontSize)
{
    if(s_GlyphMap.find(std::make_pair(c, fontSize)) != s_GlyphMap.end())
    {
        return &s_GlyphMap[std::make_pair(c, fontSize)];
    }

    //Load glyph and make sure we got the alpha basically
    FT_GlyphSlot glyphSlot = loadGlyph(c, FT_LOAD_RENDER);
    FT_Bitmap bitmap = glyphSlot->bitmap;
    if(bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
    {
        return NULL;
    }

    //Vector to put together full glyph
    int bitmapSize = bitmap.rows * bitmap.width;
    uint8_t *bitmapPointer = bitmap.buffer;
    std::vector<uint32_t> glyphPixelData(bitmapSize);

    //Loop through and make full pixels in white
    for(int i = 0; i < bitmapSize; i++)
    {
        glyphPixelData.at(i) = (0xFFFFFF00 | *bitmapPointer++);
    }

    //Create SDL_Surface from pixel data
    SDL_Surface *glyphSurface = SDL_CreateRGBSurfaceFrom(glyphPixelData.data(), bitmap.width, bitmap.rows, 32, sizeof(uint32_t) * bitmap.width, s_RedMask, s_GreenMask, s_BlueMask, s_AlphaMask);

    //Convert to texture, graphics needs name for texture
    std::string glyphName = std::to_string(c) + "_" + std::to_string(fontSize);
    SDL_Texture *glyphTexture = graphics::textureCreateFromSurface(glyphName, glyphSurface);

    //Free surface, job is done
    SDL_FreeSurface(glyphSurface);

    //Add it to glyph map
    s_GlyphMap[std::make_pair(c, fontSize)] = {static_cast<uint16_t>(bitmap.width), static_cast<uint16_t>(bitmap.rows), static_cast<int>(glyphSlot->advance.x >> 6), glyphSlot->bitmap_top, glyphSlot->bitmap_left, glyphTexture };

    //Return it
    return &s_GlyphMap[std::make_pair(c, fontSize)];
}

//Returns if line breakable character
static bool isBreakableCharacter(const uint32_t &codepoint)
{
    for(int i = 0; i < 7; i++)
    {
        if(codepoint == s_BreakPoints[i])
        {
            return true;
        }
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
static bool processSpecialCharacters(const uint32_t &codepoint, const uint32_t &originalColor, uint32_t &color)
{
    bool isSpecial = true;
    switch (codepoint)
    {
        case L'#':
        {
            color = color == COLOR_BLUE ? originalColor : COLOR_BLUE;
        }
        break;

        case L'*':
        {
            color = color == COLOR_RED ? originalColor : COLOR_RED;
        }
        break;

        case L'<':
        {
            color = color == COLOR_YELLOW ? originalColor : COLOR_YELLOW;
        }
        break;

        case L'>':
        {
            color = color == COLOR_GREEN ? originalColor : COLOR_GREEN;
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

void graphics::systemFont::renderText(const std::string &text, SDL_Texture *target, const int &x, const int &y, const int &fontSize, const uint32_t &color)
{
    //X and Y we're working with
    int workingX = x;
    int workingY = y;

    //Color we're working with
    uint32_t workingColor = color;

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
        if(processSpecialCharacters(codepoint, color, workingColor))
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

        glyphData *gd = getGlyph(codepoint, fontSize);
        if(gd)
        {
            // Set color just to be sure
            SDL_SetTextureColorMod(gd->glyph, getRed(workingColor), getGreen(workingColor), getBlue(workingColor));
            // Render glyph texture
            graphics::textureRender(gd->glyph, target, workingX + gd->left, workingY + (fontSize - gd->top));
            // X update
            workingX += gd->advanceX;
        }
    }
}

void graphics::systemFont::renderTextWrap(const std::string &text, SDL_Texture *target, const int &x, const int &y, const int &fontSize, const int &maxWidth, const uint32_t &color)
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

int graphics::systemFont::getTextWidth(const std::string &text, const int &fontSize)
{
    int textWidth = 0;
    uint32_t codepoint = 0;
    uint32_t colorMod = 0; // This is just needed so we can properly count and skip special chars
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
        if(processSpecialCharacters(codepoint, COLOR_WHITE, colorMod))
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