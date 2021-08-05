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
    inline void texDraw(SDL_Texture *target, SDL_Texture *tex, int x, int y)
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

    inline void texDrawStretch(SDL_Texture *target, SDL_Texture *tex, int x, int y, int w, int h)
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

    inline void texDrawPart(SDL_Texture *target, SDL_Texture *tex, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY)
    {
        SDL_Rect src = {srcX, srcY, srcW, srcH};
        SDL_Rect dst = {dstX, dstY, srcW, srcH};
        SDL_SetRenderTarget(gfx::render, target);
        SDL_RenderCopy(gfx::render, tex, &src, &dst);
    }

    inline void drawLine(SDL_Texture *target, const SDL_Color *c, int x1, int y1, int x2, int y2)
    {
        SDL_SetRenderTarget(gfx::render, target);
        SDL_SetRenderDrawColor(gfx::render, c->r, c->g, c->b, c->a);
        SDL_RenderDrawLine(gfx::render, x1, y1, x2, y2);
    }

    inline void drawRect(SDL_Texture *target, const SDL_Color *c, int x, int y, int w, int h)
    {
        SDL_SetRenderTarget(gfx::render, target);
        SDL_SetRenderDrawColor(gfx::render, c->r, c->g, c->b, c->a);
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderFillRect(gfx::render, &rect);
    }

    inline void clearTarget(SDL_Texture *target, const SDL_Color *clear)
    {
        SDL_SetRenderTarget(gfx::render, target);
        SDL_SetRenderDrawColor(gfx::render, clear->r, clear->g, clear->b, clear->a);
        SDL_RenderClear(gfx::render);
    }

    inline void resetRender()
    {
        SDL_SetRenderDrawColor(gfx::render, 0xFF, 0xFF, 0xFF, 0xFF);
    }
}
