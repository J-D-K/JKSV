#pragma once

#include <vector>
#include <SDL2/SDL.h>

/*Texture manager class
Keeps pointers to ALL textures so it is not possible to forget to free them. Also keeps code a little easier to read. A little.*/

typedef enum
{
    IMG_FMT_PNG,
    IMG_FMT_JPG,
    IMG_FMT_BMP
} imgTypes;

namespace gfx
{
    class textureMgr
    {
        public:
            textureMgr() = default;
            ~textureMgr();

            void textureAdd(SDL_Texture *_tex);
            SDL_Texture *textureCreate(int _w, int _h);
            SDL_Texture *textureLoadFromFile(const char *_path);
            SDL_Texture *textureLoadFromMem(imgTypes _type, const void *_dat, size_t _datSize);
            void textureResize(SDL_Texture **_tex, int _w, int _h);
        
        private:
            std::vector<SDL_Texture *> textures;
    };   
}
