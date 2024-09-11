#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "graphics/graphics.hpp"
#include "log.hpp"

namespace graphics
{
    // For loadFromMem
    typedef enum
    {
        IMAGE_TYPE_PNG,
        IMAGE_TYPE_JPEG
    } imageType;

    // Less typing, slightly easier to read
    using sdlTexture = std::shared_ptr<SDL_Texture>;

    class textureManager
    {
        public:
            // No copying.
            textureManager(const textureManager&) = delete;
            bool operator=(const textureManager&) = delete;
            // Gets the instance of textureManager. Not really used outside of this class.
            static textureManager &getInstance(void);
            // Uses SDL2_image to load from texturePath.
            static sdlTexture loadTextureFromFile(const std::string& texturePath);
            // Uses SDL2_image to create a texture using data.
            static sdlTexture createTextureFromMem(const std::string& textureName, const void *data, size_t dataSize, imageType type);
            // Creates sdlTexture from imageSurface
            static sdlTexture createTextureFromSurface(const std::string &textureName, SDL_Surface *imageSurface);
            // Just creates a texture. Mostly used for render targets.
            static sdlTexture createTexture(const std::string &textureName, int width, int height, int sdlTextureFlags);

        private:
            // No contruction
            textureManager(void) { };
            // Texture map
            std::unordered_map<std::string, std::weak_ptr<SDL_Texture>> m_TextureMap;
    };
}
