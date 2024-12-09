#include "AppStates/TextTitleSelectState.hpp"
#include "Colors.hpp"
#include "SDL.hpp"
#include <string_view>

namespace
{
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
}

TextTitleSelectState::TextTitleSelectState(Data::User *User)
    : TitleSelectCommon(), m_User(User), m_TitleSelectMenu(32, 8, 1000, 20, 555),
      m_RenderTarget(SDL::TextureManager::CreateLoadTexture(SECONDARY_TARGET, 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
{
    TextTitleSelectState::Refresh();
}

void TextTitleSelectState::Update(void)
{
    m_TitleSelectMenu.Update(AppState::HasFocus());
}

void TextTitleSelectState::Render(void)
{
    m_RenderTarget->Clear(Colors::Transparent);
    m_TitleSelectMenu.Render(m_RenderTarget->Get(), AppState::HasFocus());
    TitleSelectCommon::RenderControlGuide();
    m_RenderTarget->Render(NULL, 201, 91);
}

void TextTitleSelectState::Refresh(void)
{
    m_TitleSelectMenu.Reset();
    for (size_t i = 0; i < m_User->GetTotalDataEntries(); i++)
    {
        m_TitleSelectMenu.AddOption(Data::GetTitleInfoByID(m_User->GetApplicationIDAt(i))->GetTitle());
    }
}
