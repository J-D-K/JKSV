#include <memory>
#include <map>
#include <SDL2/SDL_image.h>
#include "graphics/graphics.hpp"
#include "graphics/systemFont.hpp"
#include "log.hpp"

namespace
{
    SDL_Window *s_Window;
    SDL_Renderer *s_Renderer;
    std::map<std::string, SDL_Texture *> s_TextureMap;
}

// Checks if texture exists in map already
static bool textureExists(const std::string &textureName)
{
    return s_TextureMap.find(textureName) != s_TextureMap.end();
}

bool graphics::init(const std::string &windowTitle, int windowWidth, int windowHeight, uint32_t windowFlags)
{
    int sdlInitError = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    if (sdlInitError != 0)
    {
        logger::log("Error starting SDL: %s.", SDL_GetError());
        return false;
    }

    s_Window = SDL_CreateWindow(windowTitle.c_str(), 0, 0, windowWidth, windowHeight, windowFlags);
    if (s_Window == NULL)
    {
        logger::log("Error creating SDL Window: %s.", SDL_GetError());
        return false;
    }

    s_Renderer = SDL_CreateRenderer(s_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (s_Renderer == NULL)
    {
        logger::log("Error creating SDL Renderer: %s.", SDL_GetError());
        return false;
    }

    // Need to read and remember exactly how this returns errors
    int imgError = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    if(imgError != (IMG_INIT_JPG | IMG_INIT_PNG))
    {
        logger::log("Error initializing SDL_Image.");
        return false;
    }

    // This is so icons don't look off when scaled
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_SetRenderDrawBlendMode(s_Renderer, SDL_BLENDMODE_BLEND);

    logger::log("graphics::init(): Succeeded.");

    return true;
}

void graphics::exit(void)
{
    // Clear texture map
    for (auto &t : s_TextureMap)
    {
        SDL_DestroyTexture(t.second);
    }
    SDL_DestroyRenderer(s_Renderer);
    SDL_DestroyWindow(s_Window);
    IMG_Quit();
    SDL_Quit();
    logger::log("graphics::exit(): Succeeded.");
}

void graphics::beginFrame(uint32_t clearColor)
{
    graphics::textureClear(NULL, clearColor);
}

void graphics::endFrame(void)
{
    SDL_RenderPresent(s_Renderer);
}

SDL_Texture *graphics::textureCreate(const std::string &textureName, int width, int height, int accessFlags)
{
    if (textureExists(textureName))
    {
        SDL_DestroyTexture(s_TextureMap[textureName]);
    }

    s_TextureMap[textureName] = SDL_CreateTexture(s_Renderer, SDL_PIXELFORMAT_RGBA8888, accessFlags, width, height);
    SDL_SetTextureBlendMode(s_TextureMap[textureName], SDL_BLENDMODE_BLEND);

    return s_TextureMap[textureName];
}

SDL_Texture *graphics::textureCreateFromSurface(const std::string &textureName, SDL_Surface *surface)
{
    if (textureExists(textureName))
    {
        SDL_DestroyTexture(s_TextureMap[textureName]);
    }

    s_TextureMap[textureName] = SDL_CreateTextureFromSurface(s_Renderer, surface);

    return s_TextureMap[textureName];
}

SDL_Texture *graphics::textureLoadFromFile(const std::string &textureName, const std::string &texturePath)
{
    if (textureExists(textureName))
    {
        return s_TextureMap[textureName];
    }

    SDL_Surface *surface = IMG_Load(texturePath.c_str());

    s_TextureMap[textureName] = SDL_CreateTextureFromSurface(s_Renderer, surface);

    SDL_FreeSurface(surface);

    return s_TextureMap[textureName];
}

SDL_Texture *graphics::textureLoadFromMem(const std::string &textureName, graphics::imageTypes imageType, const void *data, size_t dataSize)
{
    if (textureExists(textureName))
    {
        return s_TextureMap[textureName];
    }

    SDL_RWops *image = SDL_RWFromConstMem(data, dataSize);
    SDL_Surface *surface = NULL;
    switch (imageType)
    {
        case graphics::IMG_TYPE_JPEG:
        {
            surface = IMG_LoadJPG_RW(image);
        }
        break;

        case graphics::IMG_TYPE_PNG:
        {
            surface = IMG_LoadPNG_RW(image);
        }
        break;
    }

    s_TextureMap[textureName] = SDL_CreateTextureFromSurface(s_Renderer, surface);

    SDL_FreeSurface(surface);

    return s_TextureMap[textureName];
}

void graphics::textureClear(SDL_Texture *texture, uint32_t clearColor)
{
    SDL_SetRenderTarget(s_Renderer, texture);
    SDL_SetRenderDrawColor(s_Renderer, getRed(clearColor), getGreen(clearColor), getBlue(clearColor), getAlpha(clearColor));
    SDL_RenderClear(s_Renderer);
}

void graphics::textureRender(SDL_Texture *texture, SDL_Texture *target, int x, int y)
{
    // Get width and height of texture
    int textureWidth = 0, textureHeight = 0;
    SDL_QueryTexture(texture, NULL, NULL, &textureWidth, &textureHeight);

    SDL_Rect sourceRect = {0, 0, textureWidth, textureHeight};
    SDL_Rect destinationRect = {x, y, textureWidth, textureHeight};

    SDL_SetRenderTarget(s_Renderer, target);
    SDL_RenderCopy(s_Renderer, texture, &sourceRect, &destinationRect);
}

void graphics::textureRenderPart(SDL_Texture *texture, SDL_Texture *target, int x, int y, int sourceX, int sourceY, int sourceWidth, int sourceHeight)
{
    SDL_Rect sourceRect = {sourceX, sourceY, sourceWidth, sourceHeight};
    SDL_Rect destinationRect = {x, y, sourceWidth, sourceHeight};

    SDL_SetRenderTarget(s_Renderer, target);
    SDL_RenderCopy(s_Renderer, texture, &sourceRect, &destinationRect);
}

void graphics::textureRenderStretched(SDL_Texture *texture, SDL_Texture *target, int x, int y, int width, int height)
{
    int textureWidth = 0, textureHeight = 0;
    SDL_QueryTexture(texture, NULL, NULL, &textureWidth, &textureHeight);

    SDL_Rect sourceRect = {0, 0, textureWidth, textureHeight};
    SDL_Rect destinationRect = {x, y, width, height};

    SDL_SetRenderTarget(s_Renderer, target);
    SDL_RenderCopy(s_Renderer, texture, &sourceRect, &destinationRect);
}

SDL_Texture *graphics::createIcon(const std::string &iconName, const std::string &text, int fontSize)
{
    SDL_Texture *icon = graphics::textureCreate(iconName, 256, 256, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
    graphics::textureClear(icon, COLOR_DIALOG_BOX);
    int x = 128 - (systemFont::getTextWidth(text, fontSize) / 2);
    systemFont::renderText(text, icon, x, 107, fontSize, COLOR_WHITE);
    return icon;
}

void graphics::renderLine(SDL_Texture *target, int x1, int y1, int x2, int y2, uint32_t color)
{
    SDL_SetRenderTarget(s_Renderer, target);
    SDL_SetRenderDrawColor(s_Renderer, getRed(color), getGreen(color), getBlue(color), getAlpha(color));
    SDL_RenderDrawLine(s_Renderer, x1, y1, x2, y2);
}

void graphics::renderRect(SDL_Texture *target, int x, int y, int width, int height, uint32_t color)
{
    // Set target and color
    SDL_SetRenderTarget(s_Renderer, target);
    SDL_SetRenderDrawColor(s_Renderer, getRed(color), getGreen(color), getBlue(color), getAlpha(color));

    SDL_Rect rect = {x, y, width, height};
    SDL_RenderFillRect(s_Renderer, &rect);
}