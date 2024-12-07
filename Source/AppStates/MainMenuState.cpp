#include "AppStates/MainMenuState.hpp"
#include "Colors.hpp"
#include "SDL.hpp"
#include "Strings.hpp"

MainMenuState::MainMenuState(void)
    : m_RenderTarget(SDL::TextureManager::CreateLoadTexture("MainMenuTarget", 200, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_Background(SDL::TextureManager::CreateLoadTexture("MainMenuBackground", "romfs:/Textures/MenuBackground.png")),
      m_ControlGuide(Strings::GetByName(Strings::Names::ControlGuides, 0)), m_ControlGuideX(1220 - SDL::Text::GetWidth(20, m_ControlGuide)) {};

void MainMenuState::Update(void)
{
}

void MainMenuState::Render(void)
{
    // Clear render target by rendering background to it.
    m_Background->Render(m_RenderTarget->Get(), 0, 0);

    // Render target to screen.
    m_RenderTarget->Render(NULL, 0, 91);

    // Control Guide.
    if (AppState::HasFocus())
    {
        SDL::Text::Render(NULL, m_ControlGuideX, 673, 20, SDL::Text::NO_TEXT_WRAP, Colors::White, m_ControlGuide);
    }
}
