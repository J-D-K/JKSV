#include <stdio.h>
#include <map>
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define VA_SIZE 1024

#include "gfx.h"
#include "file.h"

static SDL_Window *wind;
SDL_Renderer *gfx::render;
gfx::textureMgr *gfx::texMgr;

static FT_Library lib;
static FT_Face face[6];
static int totalFonts = 0;
static bool loaded = false;

static const SDL_Color *textcol;
static SDL_Color red    = {0xFF, 0x00, 0x00, 0xFF};
static SDL_Color green  = {0x00, 0xFF, 0x00, 0xFF};
static SDL_Color blue   = {0x00, 0x99, 0xEE, 0xFF};
static SDL_Color yellow = {0xF8, 0xFC, 0x00, 0xFF};

static const uint32_t redMask   = 0xFF000000;
static const uint32_t greenMask = 0x00FF0000;
static const uint32_t blueMask  = 0x0000FF00;
static const uint32_t alphaMask = 0x000000FF;
static const uint32_t breakPoints[7] = {' ', L'　', '/', '_', '-', L'。', L'、'};

static inline bool compClr(const SDL_Color *c1, const SDL_Color *c2)
{
    return (c1->r == c2->r) && (c1->b == c2->b) && (c1->g == c2->g) && (c1->a == c2->a);
}

//Cache glyph textures
typedef struct
{
    uint16_t w, h;
    int advX, top, left;
    SDL_Texture *tex;
} glyphData;

//<Char, font size>, tex
std::map<std::pair<uint32_t, int>, glyphData> glyphCache;

static bool loadSystemFont()
{
    PlFontData shared[6];
    uint64_t langCode = 0;

    if(R_FAILED(plInitialize(PlServiceType_User)))
        return false;

    if(FT_Init_FreeType(&lib))
        return false;

    if(R_FAILED(setGetLanguageCode(&langCode)))
        return false;


    if(R_FAILED(plGetSharedFont(langCode, shared, 6, &totalFonts)))
        return false;

    for(int i = 0; i < totalFonts; i++)
    {
        if(FT_New_Memory_Face(lib, (FT_Byte *)shared[i].address, shared[i].size, 0, &face[i]))
            return false;
    }

    loaded = true;

    return true;
}

static void freeSystemFont()
{
    if(loaded)
    {
        for(int i = 0; i < totalFonts; i++)
            FT_Done_Face(face[i]);

        FT_Done_FreeType(lib);
    }

    plExit();
}

void gfx::init()
{
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    wind = SDL_CreateWindow("JKSV", 0, 0, 1280, 720, SDL_WINDOW_SHOWN);
    render = SDL_CreateRenderer(wind, -1, SDL_RENDERER_ACCELERATED);

    SDL_SetRenderDrawBlendMode(render, SDL_BLENDMODE_BLEND);

    gfx::texMgr = new gfx::textureMgr;

    loadSystemFont();

    //This is to avoid blank, black glyphs
    for(unsigned i = 0x20; i < 0x7E; i++)
        gfx::drawTextf(NULL, 18, 32, 32, &ui::txtCont, "%c", i);
}

void gfx::exit()
{
    delete gfx::texMgr;
    SDL_DestroyRenderer(gfx::render);
    SDL_DestroyWindow(wind);
    IMG_Quit();
    SDL_Quit();
    freeSystemFont();
}

void gfx::present()
{
    SDL_RenderPresent(render);
}

static inline void resizeFont(int sz)
{
    for(int i = 0; i < totalFonts; i++)
        FT_Set_Char_Size(face[i], 0, sz * 64, 90, 90);
}

static inline FT_GlyphSlot loadGlyph(const uint32_t c, FT_Int32 flags)
{
    for(int i = 0; i < totalFonts; i++)
    {
        FT_UInt cInd = 0;
        if( (cInd = FT_Get_Char_Index(face[i], c)) != 0 && FT_Load_Glyph(face[i], cInd, flags) == 0)
            return face[i]->glyph;
    }
    return NULL;
}

static glyphData *getGlyph(uint32_t chr, int size)
{
    //If it's already been loaded and rendered, grab the texture
    if(glyphCache.find(std::make_pair(chr, size)) != glyphCache.end())
        return &glyphCache[std::make_pair(chr, size)];

    //Load glyph with Freetype
    FT_GlyphSlot glyph = loadGlyph(chr, FT_LOAD_RENDER);
    FT_Bitmap bmp = glyph->bitmap;
    if(bmp.pixel_mode != FT_PIXEL_MODE_GRAY)
        return NULL;

    //Convert to SDL_Surface -> Texture
    SDL_Texture *tex;
    size_t glyphSize = bmp.rows * bmp.width;
    uint8_t *bmpPtr = bmp.buffer;
    uint32_t basePixel = 0xFFFFFF00;
    uint32_t *tmpBuff = (uint32_t *)malloc(sizeof(uint32_t) * glyphSize);

    //Loop through and fill out buffer
    for(size_t i = 0; i < glyphSize; i++)
        tmpBuff[i] = basePixel | *bmpPtr++;

    SDL_Surface *tmpSurf = SDL_CreateRGBSurfaceFrom(tmpBuff, bmp.width, bmp.rows, 32, 4 * bmp.width, redMask, greenMask, blueMask, alphaMask);
    tex = SDL_CreateTextureFromSurface(gfx::render, tmpSurf);

    SDL_FreeSurface(tmpSurf);
    free(tmpBuff);

    //Add it to texture manager so textures are freed on exit
    gfx::texMgr->textureAdd(tex);

    //Add it to cache map
    glyphCache[std::make_pair(chr, size)] = {(uint16_t)bmp.width, (uint16_t)bmp.rows, (int)glyph->advance.x >> 6, glyph->bitmap_top, glyph->bitmap_left, tex};

    return &glyphCache[std::make_pair(chr, size)];
}

//Takes care of special characters/color switching
static inline bool specialChar(const uint32_t *p, const int *fontSize, const SDL_Color *c, const int *baseX, int *modX, int *modY)
{
    //set to false on default
    bool ret = true;

    switch(*p)
    {
        case '\n':
            *modX = *baseX;
            *modY += *fontSize + 8;
            break;

        case '#':
            if(compClr(textcol, &blue))
                textcol = c;
            else
                textcol = &blue;
            break;

        case '*':
            if(compClr(textcol, &red))
                textcol = c;
            else
                textcol = &red;
            break;

        case '<':
            if(compClr(textcol, &yellow))
                textcol = c;
            else
                textcol = &yellow;
            break;

        case '>':
            if(compClr(textcol, &green))
                textcol = c;
            else
                textcol = &green;
            break;

        default:
            ret = false;//no special char
            break;
    }
    return ret;
}

void gfx::drawTextf(SDL_Texture *target, int fontSize, int x, int y, const SDL_Color *c, const char *fmt, ...)
{
    SDL_SetRenderTarget(gfx::render, target);
    char tmp[VA_SIZE];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    int tmpX = x;
    uint32_t point = 0;
    ssize_t unitCnt = 0;
    size_t textLength = strlen(tmp);

    resizeFont(fontSize);

    textcol = c;

    for(unsigned i = 0; i < textLength; )
    {
        unitCnt = decode_utf8(&point, (const uint8_t *)&tmp[i]);
        if(unitCnt <= 0)
            break;

        i += unitCnt;
        if(specialChar(&point, &fontSize, c, &x, &tmpX, &y))
            continue;

        glyphData *g = getGlyph(point, fontSize);
        if(g != NULL)
        {
            SDL_Rect src = {0, 0, g->w, g->h};
            SDL_Rect dst = {tmpX + g->left, y + (fontSize - g->top), g->w, g->h};
            SDL_SetTextureColorMod(g->tex, textcol->r, textcol->g, textcol->b);
            SDL_RenderCopy(render, g->tex, &src, &dst);

            tmpX += g->advX;
        }
    }
    SDL_SetRenderTarget(gfx::render, NULL);
}

inline bool isBreakChar(uint32_t point)
{
    for(int i = 0; i < 7; i++)
    {
        if(breakPoints[i] == point)
            return true;
    }
    return false;
}

inline size_t findNextBreak(const char *str)
{
    size_t length = strlen(str);
    for(size_t i = 0; i < length; )
    {
        uint32_t nextPoint = 0;
        ssize_t unitCnt = decode_utf8(&nextPoint, (const uint8_t *)&str[i]);
        i += unitCnt;
        if(unitCnt <= 0)
            return length;

        if(isBreakChar(nextPoint))
            return i;
    }
    return length;
}

void gfx::drawTextfWrap(SDL_Texture *target, int fontSize, int x, int y, int maxWidth, const SDL_Color *c, const char *fmt, ...)
{
    SDL_SetRenderTarget(gfx::render, target);
    char tmp[VA_SIZE], wordBuff[128];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    resizeFont(fontSize);
    size_t nextBreak = 0, strlength = strlen(tmp);
    int tmpX = x;

    textcol = c;

    for(unsigned i = 0; i < strlength; )
    {
        nextBreak = findNextBreak(&tmp[i]);
        memset(wordBuff, 0, 128);
        memcpy(wordBuff, &tmp[i], nextBreak);

        size_t width = gfx::getTextWidth(wordBuff, fontSize);

        if((int)(tmpX + width) >= (int)(x + maxWidth))
        {
            tmpX = x;
            y += fontSize + 8;
        }

        size_t wordLength = strlen(wordBuff);
        uint32_t point = 0;
        for(unsigned j = 0; j < wordLength; )
        {
            ssize_t unitCnt = decode_utf8(&point, (const uint8_t *)&wordBuff[j]);
            if(unitCnt <= 0)
                break;

            j += unitCnt;

            if(specialChar(&point, &fontSize, c, &x, &tmpX, &y))
                continue;

            glyphData *g = getGlyph(point, fontSize);
            if(g != NULL)
            {
                SDL_Rect src = {0, 0, g->w, g->h};
                SDL_Rect dst = {tmpX + g->left, y + (fontSize - g->top), g->w, g->h};
                SDL_SetTextureColorMod(g->tex, textcol->r, textcol->g, textcol->b);
                SDL_RenderCopy(render, g->tex, &src, &dst);

                tmpX += g->advX;
            }
        }
        i += wordLength;
    }
    SDL_SetRenderTarget(gfx::render, NULL);
}

size_t gfx::getTextWidth(const char *str, int fontSize)
{
    resizeFont(fontSize);
    size_t width = 0, strlength = strlen(str);
    uint32_t unitCnt = 0, point = 0;

    for(unsigned i = 0; i < strlength; )
    {
        unitCnt = decode_utf8(&point, (const uint8_t *)&str[i]);
        if(unitCnt <= 0)
            break;

        i += unitCnt;

        //Ignore these
        if(point == '\n' || point == '#' || point == '*' || point == '<' || point == '>')
            continue;

        glyphData *g = getGlyph(point, fontSize);
        if(g != NULL)
            width += g->advX;
    }
    return width;
}

void gfx::texDraw(SDL_Texture *target, SDL_Texture *tex, int x, int y)
{
    int tW = 0, tH = 0;
    if(SDL_QueryTexture(tex, NULL, NULL, &tW, &tH) == 0)
    {
        SDL_SetRenderTarget(gfx::render, target);
        SDL_Rect src = {0, 0, tW, tH};
        SDL_Rect pos = {x, y, tW, tH};
        SDL_RenderCopy(gfx::render, tex, &src, &pos);
    }
}

void gfx::texDrawStretch(SDL_Texture *target, SDL_Texture *tex, int x, int y, int w, int h)
{
    int tW = 0, tH = 0;
    if(SDL_QueryTexture(tex, NULL, NULL, &tW, &tH) == 0)
    {
        SDL_SetRenderTarget(gfx::render, target);
        SDL_Rect src = {0, 0, tW, tH};
        SDL_Rect pos = {x, y, w, h};
        SDL_RenderCopy(gfx::render, tex, &src, &pos);
    }
}

void gfx::texDrawPart(SDL_Texture *target, SDL_Texture *tex, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY)
{
    SDL_Rect src = {srcX, srcY, srcW, srcH};
    SDL_Rect dst = {dstX, dstY, srcW, srcH};
    SDL_SetRenderTarget(gfx::render, target);
    SDL_RenderCopy(gfx::render, tex, &src, &dst);
}

void gfx::drawLine(SDL_Texture *target, const SDL_Color *c, int x1, int y1, int x2, int y2)
{
    SDL_SetRenderTarget(gfx::render, target);
    SDL_SetRenderDrawColor(gfx::render, c->r, c->g, c->b, c->a);
    SDL_RenderDrawLine(gfx::render, x1, y1, x2, y2);
}

void gfx::drawRect(SDL_Texture *target, const SDL_Color *c, int x, int y, int w, int h)
{
    SDL_SetRenderTarget(gfx::render, target);
    SDL_SetRenderDrawColor(gfx::render, c->r, c->g, c->b, c->a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(gfx::render, &rect);
}

void gfx::clearTarget(SDL_Texture *target, const SDL_Color *clear)
{
    SDL_SetRenderTarget(gfx::render, target);
    SDL_SetRenderDrawColor(gfx::render, clear->r, clear->g, clear->b, clear->a);
    SDL_RenderClear(gfx::render);
}
