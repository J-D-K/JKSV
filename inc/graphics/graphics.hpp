#pragma once
#include <string>
#include <SDL2/SDL.h>
#include "graphics/colors.hpp"
#include "graphics/systemFont.hpp"

namespace graphics
{
    // This is for textureLoadFromMem
    typedef enum
    {
        IMG_TYPE_JPEG,
        IMG_TYPE_PNG
    } imageTypes;

    // Creates window and renderer
    bool init(const std::string &windowTitle, const int &windowWidth, const int &windowHeight);
    void exit(void);

    // Clears the framebuffer to clearColor.
    void beginFrame(const uint32_t &clearColor);
    //Calls SDL_RenderPresent
    void endFrame(void);

    // Texture loading functions. Textures are freed automatically at app exit
    SDL_Texture *textureCreate(const std::string &textureName, const int &width, const int &height, const int &accessFlags);
    SDL_Texture *textureCreateFromSurface(const std::string &textureName, SDL_Surface *SDL_Surface);
    SDL_Texture *textureLoadFromFile(const std::string &textureName, const std::string &texturePath);
    SDL_Texture *textureLoadFromMem(const std::string &textureName, const graphics::imageTypes &imgType, const void *data, const size_t &dataSize);

    // Texture rendering functions
    void textureClear(SDL_Texture *texture, const uint32_t &clearColor);
    void textureRender(SDL_Texture *texture, SDL_Texture *target, const int &x, const int &y);
    void textureRenderPart(SDL_Texture *texture, SDL_Texture *target, const int &x, const int &y, const int &sourceX, const int &sourceY, const int &sourceWidth, const int &sourceHeight);
    void textureRenderStretched(SDL_Texture *texture, SDL_Texture *target, const int &x, const int &y, const int &width, const int &height);

    // Misc
    SDL_Texture *createIcon(const std::string &iconName, const std::string &text, const int &fontSize);
    void renderLine(SDL_Texture *target, const int &x1, const int &y1, const int &x2, const int &y2, const uint32_t &color);
    void renderRect(SDL_Texture *target, const int &x, const int &y, const int &width, const int &height, const uint32_t &color);
}
