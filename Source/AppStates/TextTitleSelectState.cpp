#include "AppStates/TextTitleSelectState.hpp"
#include "AppStates/MainMenuState.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Input.hpp"
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

    if (Input::ButtonPressed(HidNpadButton_Y))
    {
        Config::AddRemoveFavorite(m_User->GetApplicationIDAt(m_TitleSelectMenu.GetSelected()));
        MainMenuState::RefreshViewStates();
    }
    else if (Input::ButtonPressed(HidNpadButton_B))
    {
        AppState::Deactivate();
    }
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
        std::string Option;
        uint64_t ApplicationID = m_User->GetApplicationIDAt(i);
        const char *Title = Data::GetTitleInfoByID(ApplicationID)->GetTitle();
        if (Config::IsFavorite(ApplicationID))
        {
            Option = std::string("^\uE017^ ") + Title;
        }
        else
        {
            Option = Title;
        }
        m_TitleSelectMenu.AddOption(Option.c_str());
    }
}
