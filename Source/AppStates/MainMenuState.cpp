#include "AppStates/MainMenuState.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include "SDL.hpp"
#include "Strings.hpp"

namespace
{
    // Size of the font to use to make tweaking this easier.
    static int ICON_FONT_SIZE = 50;
} // namespace

MainMenuState::MainMenuState(void)
    : m_RenderTarget(SDL::TextureManager::CreateLoadTexture("MainMenuTarget", 200, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_Background(SDL::TextureManager::CreateLoadTexture("MainMenuBackground", "romfs:/Textures/MenuBackground.png")),
      m_MainMenu(50, 15, 1, 555), m_ControlGuide(Strings::GetByName(Strings::Names::ControlGuides, 0)),
      m_ControlGuideX(1220 - SDL::Text::GetWidth(20, m_ControlGuide))
{
    // Fetch user list.
    Data::GetUsers(m_Users);

    // Loop through and add icons to menu
    for (size_t i = 0; i < m_Users.size(); i++)
    {
        m_MainMenu.AddOption(m_Users.at(i)->GetSharedIcon());
    }

    // Create icons for the other two.
    m_SettingsIcon = SDL::TextureManager::CreateLoadTexture("SettingsIcon", "romfs:/Textures/SettingsIcon.png");

    // Finally add them to the end.
    m_MainMenu.AddOption(m_SettingsIcon);
    m_MainMenu.AddOption(m_ExtrasIcon);
}

void MainMenuState::Update(void)
{
    m_MainMenu.Update();
}

void MainMenuState::Render(void)
{
    // Clear render target by rendering background to it.
    m_Background->Render(m_RenderTarget->Get(), 0, 0);
    // Render menu.
    m_MainMenu.Render(m_RenderTarget->Get(), AppState::HasFocus());
    // Render target to screen.
    m_RenderTarget->Render(NULL, 0, 91);

    // Control Guide.
    if (AppState::HasFocus())
    {
        SDL::Text::Render(NULL, m_ControlGuideX, 673, 20, SDL::Text::NO_TEXT_WRAP, Colors::White, m_ControlGuide);
    }
}
