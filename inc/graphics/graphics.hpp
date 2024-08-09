#pragma once
#include <memory>
#include <string>

#include <SDL2/SDL.h>

#include "graphics/colors.hpp"
#include "graphics/systemFont.hpp"
#include "graphics/textureManager.hpp"

namespace graphics
{
    // Creates window and renderer
    bool init(const std::string &windowTitle, int windowWidth, int windowHeight, uint32_t windowFlags);
    void exit(void);

    // Returns pointer to renderer. Only used for textureManager.
    SDL_Renderer *getRenderer(void);

    // Clears the framebuffer to clearColor.
    void beginFrame(uint32_t clearColor);
    //Calls SDL_RenderPresent
    void endFrame(void);

    // Texture rendering functions
    // To do: I'd really like to make this work without using get().
    void textureClear(SDL_Texture *texture, uint32_t clearColor);
    void textureRender(SDL_Texture *texture, SDL_Texture *target, int x, int y);
    void textureRenderPart(SDL_Texture *texture, SDL_Texture *target, int x, int y, int sourceX, int sourceY, int sourceWidth, int sourceHeight);
    void textureRenderStretched(SDL_Texture *texture, SDL_Texture *target, int x, int y, int width, int height);

    // Misc
    std::shared_ptr<SDL_Texture> createIcon(const std::string &iconName, const std::string &text, int fontSize);
    void renderLine(SDL_Texture *target, int x1, int y1, int x2, int y2, uint32_t color);
    void renderRect(SDL_Texture *target, int x, int y, int width, int height, uint32_t color);
}
