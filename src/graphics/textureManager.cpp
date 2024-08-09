#include "graphics/textureManager.hpp"

struct textureDestroyer
{
    void operator()(SDL_Texture *t)
    {
        SDL_DestroyTexture(t);
    }
};

graphics::textureManager &graphics::textureManager::getInstance(void)
{
    static graphics::textureManager manager;
    return manager;
}

graphics::sdlTexture graphics::textureManager::loadTextureFromFile(const std::string &texturePath)
{
    // This is the shared pointer being returned
    graphics::sdlTexture returnTexture;
    // Get the instance of this manager
    graphics::textureManager &manager = graphics::textureManager::getInstance();
    // Search if texturePath already exists in map.
    auto findTexture = manager.m_TextureMap.find(texturePath);
    // If it does and the pointer is still good, lock it and return it.
    if (findTexture != manager.m_TextureMap.end() && findTexture->second.expired() == false)
    {
        returnTexture = findTexture->second.lock();
    }
    else // Load/reload it.
    {
        // Surface from SDL_image
        SDL_Surface *imageSurface = IMG_Load(texturePath.c_str());
        if (imageSurface == NULL)
        {
            returnTexture = nullptr;
        }
        else
        {
            // Allocate new shared_ptr with deleter so I don't have to care about destroying it later.
            returnTexture = graphics::sdlTexture(SDL_CreateTextureFromSurface(graphics::getRenderer(), imageSurface), textureDestroyer());
            // Free surface. It's useless now and should feel bad.
            SDL_FreeSurface(imageSurface);
            // Add to map
            manager.m_TextureMap[texturePath] = returnTexture;
        }
    }
    // Return it.
    return returnTexture;
}

graphics::sdlTexture graphics::textureManager::createTextureFromMem(const std::string &textureName, const void *data, size_t dataSize, graphics::imageType type)
{
    // Shared pointer being returned.
    graphics::sdlTexture returnTexture;
    // Get instance.
    graphics::textureManager &manager = graphics::textureManager::getInstance();
    // Search for it
    auto findTexture = manager.m_TextureMap.find(textureName);
    // If it's still good, lock it.
    if (findTexture != manager.m_TextureMap.end() && findTexture->second.expired() == false)
    {
        returnTexture = findTexture->second.lock();
    }
    else
    {
        // SDL Read write operations. Const since we're not changing anything
        SDL_RWops *imageMemory = SDL_RWFromConstMem(data, dataSize);
        // Surface pointer we're using.
        SDL_Surface *imageSurface = NULL;
        // Load according to type provided.
        switch (type)
        {
        case IMAGE_TYPE_PNG:
        {
            imageSurface = IMG_LoadPNG_RW(imageMemory);
        }
        break;

        case IMAGE_TYPE_JPEG:
        {
            imageSurface = IMG_LoadJPG_RW(imageMemory);
        }
        break;
        }
        // Check just in case
        if (imageSurface == NULL)
        {
            returnTexture = nullptr;
        }
        else
        {
            // Create shared pointer to return
            returnTexture = graphics::sdlTexture(SDL_CreateTextureFromSurface(graphics::getRenderer(), imageSurface), textureDestroyer());
            // Free surface
            SDL_FreeSurface(imageSurface);
            // Add to map
            manager.m_TextureMap[textureName] = returnTexture;
        }
    }
    return returnTexture;
}

graphics::sdlTexture graphics::textureManager::createTextureFromSurface(const std::string &textureName, SDL_Surface *imageSurface)
{
    // Shared pointer to return
    graphics::sdlTexture returnTexture;
    // instance
    graphics::textureManager &manager = graphics::textureManager::getInstance();
    // Try to convert
    SDL_Texture *imageTexture = SDL_CreateTextureFromSurface(graphics::getRenderer(), imageSurface);
    if(imageTexture == NULL)
    {
        returnTexture = nullptr;
    }
    else
    {
        returnTexture = sdlTexture(imageTexture, textureDestroyer());
        // Add it to map. Collisions should result in old one being freed.
        manager.m_TextureMap[textureName] = returnTexture;
    }
    return returnTexture;
}

graphics::sdlTexture graphics::textureManager::createTexture(const std::string &textureName, int width, int height, int sdlTextureFlags)
{
    // Shared pointer being returned
    graphics::sdlTexture returnTexture;
    // Manager instance
    graphics::textureManager &manager = graphics::textureManager::getInstance();
    // This is different. Don't bother searching. Just create it. If it exists, the shared_ptr reference count should hit 0 and it will free itself.
    SDL_Texture *createdTexture = SDL_CreateTexture(graphics::getRenderer(), SDL_PIXELFORMAT_RGBA8888, sdlTextureFlags, width, height);
    // Just check so it looks like I care more.
    if (createdTexture == NULL)
    {
        returnTexture = nullptr;
    }
    else
    {
        // Create sdlTexture
        returnTexture = graphics::sdlTexture(createdTexture, textureDestroyer());
        // Set blendmode for alpha
        SDL_SetTextureBlendMode(returnTexture.get(), SDL_BLENDMODE_BLEND);
        // Add it to map
        manager.m_TextureMap[textureName] = returnTexture;
    }
    // Return
    return returnTexture;
}