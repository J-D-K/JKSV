#include "AppStates/TitleSelectState.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Input.hpp"
#include "SDL.hpp"
#include "Strings.hpp"
#include <string_view>

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

TitleSelectState::TitleSelectState(Data::User *User)
    : m_User(User),
      m_RenderTarget(SDL::TextureManager::CreateLoadTexture(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_TitleView(m_User)
{
    if (m_TitleControlsX == 0)
    {
        m_TitleControlsX = 1220 - SDL::Text::GetWidth(22, Strings::GetByName(Strings::Names::ControlGuides, 1));
    }
}

void TitleSelectState::Update(void)
{
    m_TitleView.Update(AppState::HasFocus());

    if (Input::ButtonPressed(HidNpadButton_B))
    {
        // This will reset all the tiles so they're 128x128.
        m_TitleView.Reset();
        AppState::Deactivate();
    }
    else if (Input::ButtonPressed(HidNpadButton_Y))
    {
        uint64_t ApplicationID = m_User->GetApplicationIDAt(m_TitleView.GetSelected());
        Config::AddRemoveFavorite(ApplicationID);
    }
}

void TitleSelectState::Render(void)
{
    m_RenderTarget->Clear(Colors::Transparent);
    m_TitleView.Render(m_RenderTarget->Get(), AppState::HasFocus());

    if (AppState::HasFocus())
    {
        SDL::Text::Render(NULL,
                          m_TitleControlsX,
                          673,
                          22,
                          SDL::Text::NO_TEXT_WRAP,
                          Colors::White,
                          Strings::GetByName(Strings::Names::ControlGuides, 1));
    }
    m_RenderTarget->Render(NULL, 201, 91);
}
