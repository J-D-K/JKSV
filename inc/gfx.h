#pragma once

#include <SDL.h>

namespace gfx
{
    extern SDL_Renderer *render;

    void init();
    void exit();
    void present();

    SDL_Texture *loadJPEGMem(const void *jpegData, size_t jpegSize);
    SDL_Texture *loadImageFile(const char *file);

    void drawTextf(SDL_Texture *target, int fontSize, int x, int y, const SDL_Color *c, const char *fmt, ...);
    void drawTextfWrap(SDL_Texture *target, int fontSize, int x, int y, int maxWidth, const SDL_Color* c, const char *fmt, ...);
    size_t getTextWidth(const char *str, int fontSize);

    //Shortcuts for drawing
    void texDraw(SDL_Texture *target, SDL_Texture *tex, int x, int y);
    void texDrawStretch(SDL_Texture *target, SDL_Texture *tex, int x, int y, int w, int h);
    void texDrawPart(SDL_Texture *target, SDL_Texture *tex, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY);
    void drawLine(SDL_Texture *target, const SDL_Color *c, int x1, int y1, int x2, int y2);
    void drawRect(SDL_Texture *target, const SDL_Color *c, int x, int y, int w, int h);
    void clearTarget(SDL_Texture *target, const SDL_Color *clear);
}
