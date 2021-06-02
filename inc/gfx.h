#ifndef GFX_H
#define GFX_H

#include <SDL.h>

namespace gfx
{
    extern SDL_Renderer *render;

    void init();
    void exit();
    void clear(const SDL_Color *c);
    void present();

    SDL_Texture *loadJPEGMem(const void *jpegData, size_t jpegSize);
    SDL_Texture *loadImageFile(const char *file);

    void drawTextf(int fontSize, int x, int y, const SDL_Color *c, const char *fmt, ...);
    void drawTextfWrap(int fontSize, int x, int y, int maxWidth, const SDL_Color* c, const char *fmt, ...);
    size_t getTextWidth(const char *str, int fontSize);

    //Shortcuts for drawing
    inline void texDraw(SDL_Texture *tex, int x, int y)
    {
        int tW = 0, tH = 0;
        if(SDL_QueryTexture(tex, NULL, NULL, &tW, &tH) == 0)
        {
            SDL_Rect src = {0, 0, tW, tH};
            SDL_Rect pos = {x, y, tW, tH};
            SDL_RenderCopy(render, tex, &src, &pos);
        }
    }

    inline void texDrawStretch(SDL_Texture *tex, int x, int y, int w, int h)
    {
        int tW = 0, tH = 0;
        if(SDL_QueryTexture(tex, NULL, NULL, &tW, &tH) == 0)
        {
            SDL_Rect src = {0, 0, tW, tH};
            SDL_Rect pos = {x, y, w, h};
            SDL_RenderCopy(render, tex, &src, &pos);
        }
    }

    inline void drawLine(const SDL_Color *c, int x1, int y1, int x2, int y2)
    {
        SDL_SetRenderDrawColor(render, c->r, c->g, c->b, c->a);
        SDL_RenderDrawLine(render, x1, y1, x2, y2);
    }

    inline void drawRect(const SDL_Color *c, int x, int y, int w, int h)
    {
        SDL_SetRenderDrawColor(render, c->r, c->g, c->b, c->a);
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderFillRect(render, &rect);
    }
}

#endif // GFX_H
