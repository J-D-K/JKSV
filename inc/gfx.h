#ifndef GFX_H
#define GFX_H

#include <stdint.h>
#include <stdbool.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef __cplusplus
extern "C"
{
#endif

//Structs
typedef struct
{
    uint8_t r, g, b, a;
} color;

typedef struct
{
    FT_Library lib;
    FT_Face    face;
    FT_Error libRet, faceRet;
    //Loads to buffer for speed for TTF
    uint8_t *fntData;
} font;

typedef struct
{
    size_t size;
    unsigned width, height;
    uint32_t *data;
} tex;

//Inits needed graphics stuff
bool graphicsInit(int windowWidth, int windowHeight);

//Exits needed services
bool graphicsExit();

//Flush, swap buffers
void gfxHandleBuffs();

//Creates color from uint32_t
inline color colorCreateU32(uint32_t clr)
{
    color ret;
    ret.a = clr >> 24 & 0xFF;
    ret.b = clr >> 16 & 0xFF;
    ret.g = clr >>  8 & 0xFF;
    ret.r = clr & 0xFF;
    return ret;
}

//Sets color to [r], [g], [b], [a]
inline color colorCreateRGBA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
{
    color ret;
    ret.r = _r;
    ret.g = _g;
    ret.b = _b;
    ret.a = _a;
    return ret;
}

//Inverts color
inline void colorInvert(color *c)
{
    c->r = (0xFF - c->r);
    c->g = (0xFF - c->g);
    c->b = (0xFF - c->b);
}

//Returns uint32_t color
inline uint32_t colorGetColor(const color c)
{
    return (c.a << 24 | c.b << 16 | c.g << 8 | c.r);
}

//Draws text using f
void drawText(const char *str, tex *target, const font *f, int x, int y, int sz, color c);

//Returns text width
size_t textGetWidth(const char *str, const font *f, int sz);

//Clears framebuffer to c
void clearBufferColor(const color c);

//Draws rectangle at x, y with w, h
void drawRect(tex *target, int x, int y, int w, int h, const color c);

/*
TEX BEGIN
*/
//Inits empty tex
tex *texCreate(int w, int h);

//Loads PNG from path
tex *texLoadPNGFile(const char *path);

//Loads JPEG from path
tex *texLoadJPEGFile(const char *path);

//Loads jpeg from memory
tex *texLoadJPEGMem(const uint8_t *jpegData, size_t jpegSize);

//Frees memory used by t
void texDestroy(tex *t);

//Clears tex completely with c
void texClearColor(tex *in, const color c);

//Draws t at x, y
void texDraw(const tex *t, tex *target, int x, int y);

//Draws without alpha blending, faster
void texDrawNoAlpha(const tex *t, tex *target, int x, int y);

//Draws skipping every other pixel + row
void texDrawSkip(const tex *t, tex *target, int x, int y);

//Same as above, no alpha
void texDrawSkipNoAlpha(const tex *t, tex *target, int x, int y);

//Draw t inverted at x, y
void texDrawInvert(const tex *t, tex *target, int x, int y);

//Replaces old with newColor
void texSwapColors(tex *t, const color old, const color newColor);

//Scales tex * scale and writes to out. Can only multiply for now
void texScaleToTex(const tex *in, tex *out, int scale);

//Draws directly to Switch's framebuffer
void texDrawDirect(const tex *in, int x, int y);

//Loads and returns font with Switch shared font loaded
font *fontLoadSharedFont(PlSharedFontType fontType);

//Loads and returns TTF font
font *fontLoadTTF(const char *path);

//Frees font
void fontDestroy(font *f);

/*
TEX END
*/

//returns framebuffer tex pointer
tex *texGetFramebuffer();
#ifdef __cplusplus
}
#endif

#endif // GFX_H
