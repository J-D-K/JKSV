#include <vector>
#include <algorithm>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "gfx.h"

gfx::textureMgr::~textureMgr()
{
    for(auto tex : textures)
        SDL_DestroyTexture(tex);
}

void gfx::textureMgr::textureAdd(SDL_Texture *_tex)
{
    textures.push_back(_tex);
}

SDL_Texture *gfx::textureMgr::textureCreate(int _w, int _h)
{
    SDL_Texture *ret = NULL;
    ret = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET, _w, _h);
    if(ret)
    {
        SDL_SetTextureBlendMode(ret, SDL_BLENDMODE_BLEND);
        textures.push_back(ret);
    }
    return ret;
}

SDL_Texture *gfx::textureMgr::textureLoadFromFile(const char *_path)
{
    SDL_Texture *ret = NULL;
    SDL_Surface *tmp = IMG_Load(_path);
    if(tmp)
    {
        ret = SDL_CreateTextureFromSurface(gfx::render, tmp);
        textures.push_back(ret);
        SDL_FreeSurface(tmp);
        SDL_SetTextureBlendMode(ret, SDL_BLENDMODE_BLEND);
    }
    return ret;
}

static SDL_Texture *loadPNGMem(const void *_dat, size_t _datSize)
{
    SDL_Texture *ret = NULL;
    SDL_RWops *pngData = SDL_RWFromConstMem(_dat, _datSize);
    SDL_Surface *tmp = IMG_LoadPNG_RW(pngData);
    if(tmp)
    {
        ret = SDL_CreateTextureFromSurface(gfx::render, tmp);
        SDL_FreeSurface(tmp);
        SDL_SetTextureBlendMode(ret, SDL_BLENDMODE_BLEND);
    }
    SDL_RWclose(pngData);
    return ret;
}

static SDL_Texture *loadJPEGMem(const void *_dat, size_t _datSize)
{
    SDL_Texture *ret = NULL;
    SDL_RWops *jpegData = SDL_RWFromConstMem(_dat, _datSize);
    SDL_Surface *tmp = IMG_LoadJPG_RW(jpegData);
    if(tmp)
    {
        ret = SDL_CreateTextureFromSurface(gfx::render, tmp);
        SDL_FreeSurface(tmp);
        SDL_SetTextureBlendMode(ret, SDL_BLENDMODE_BLEND);
    }
    SDL_RWclose(jpegData);
    return ret;
}

static SDL_Texture *loadBMPMem(const void *_dat, size_t _datSize)
{
    SDL_Texture *ret = NULL;
    SDL_RWops *bmpData = SDL_RWFromConstMem(_dat, _datSize);
    SDL_Surface *tmp = IMG_LoadBMP_RW(bmpData);
    if(tmp)
    {
        ret = SDL_CreateTextureFromSurface(gfx::render, tmp);
        SDL_FreeSurface(tmp);
        SDL_SetTextureBlendMode(ret, SDL_BLENDMODE_BLEND);
    }
    SDL_RWclose(bmpData);
    return ret;
}

SDL_Texture *gfx::textureMgr::textureLoadFromMem(imgTypes _type, const void *_dat, size_t _datSize)
{
    SDL_Texture *ret = NULL;
    switch (_type)
    {
        case IMG_FMT_PNG:
            ret = loadPNGMem(_dat, _datSize);
            break;

        case IMG_FMT_JPG:
            ret = loadJPEGMem(_dat, _datSize);
            break;

        case IMG_FMT_BMP:
            ret = loadBMPMem(_dat, _datSize);
            break;
    }

    if(ret)
        textures.push_back(ret);

    return ret;
}

void gfx::textureMgr::textureResize(SDL_Texture **_tex, int _w, int _h)
{
    auto texIt = std::find(textures.begin(), textures.end(), *_tex);
    if(texIt != textures.end())
    {
        SDL_DestroyTexture(*texIt);
        *_tex = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET, _w, _h);
        SDL_SetTextureBlendMode(*_tex, SDL_BLENDMODE_BLEND);
        *texIt = *_tex;
    }
}