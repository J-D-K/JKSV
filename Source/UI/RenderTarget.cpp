#include "UI/RenderTarget.hpp"

UI::RenderTarget::RenderTarget(int X, int Y, int Width, int Height) : m_X(X), m_Y(Y)
{
    static int RenderTargetID = 0;
    std::string TargetName = "RenderTarget_" + std::to_string(RenderTargetID++);
    m_RenderTarget = SDL::TextureManager::CreateLoadTexture(TargetName, Width, Height, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
}

void UI::RenderTarget::Render(SDL_Texture *Target, bool HasFocus)
{
    m_RenderTarget->Render(Target, m_X, m_Y);
}

SDL_Texture *UI::RenderTarget::Get(void)
{
    return m_RenderTarget->Get();
}
