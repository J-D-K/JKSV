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
    bool init(const std::string &windowTitle, int windowWidth, int windowHeight, uint32_t windowFlags);
    void exit(void);

    // Clears the framebuffer to clearColor.
    void beginFrame(uint32_t clearColor);
    //Calls SDL_RenderPresent
    void endFrame(void);

    // Texture loading functions. Textures are freed automatically at app exit
    SDL_Texture *textureCreate(const std::string &textureName, int width, int height, int accessFlags);
    SDL_Texture *textureCreateFromSurface(const std::string &textureName, SDL_Surface *surface);
    SDL_Texture *textureLoadFromFile(const std::string &textureName, const std::string &texturePath);
    SDL_Texture *textureLoadFromMem(const std::string &textureName, graphics::imageTypes imageType, const void *data, size_t dataSize);

    // Texture rendering functions
    void textureClear(SDL_Texture *texture, uint32_t clearColor);
    void textureRender(SDL_Texture *texture, SDL_Texture *target, int x, int y);
    void textureRenderPart(SDL_Texture *texture, SDL_Texture *target, int x, int y, int sourceX, int sourceY, int sourceWidth, int sourceHeight);
    void textureRenderStretched(SDL_Texture *texture, SDL_Texture *target, int x, int y, int width, int height);

    // Misc
    SDL_Texture *createIcon(const std::string &iconName, const std::string &text, int fontSize);
    void renderLine(SDL_Texture *target, int x1, int y1, int x2, int y2, uint32_t color);
    void renderRect(SDL_Texture *target, int x, int y, int width, int height, uint32_t color);
}
