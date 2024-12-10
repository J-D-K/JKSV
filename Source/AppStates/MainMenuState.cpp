#include "AppStates/MainMenuState.hpp"
#include "AppStates/ExtrasMenuState.hpp"
#include "AppStates/SettingsState.hpp"
#include "AppStates/TextTitleSelectState.hpp"
#include "AppStates/TitleSelectCommon.hpp"
#include "AppStates/TitleSelectState.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Input.hpp"
#include "JKSV.hpp"
#include "Logger.hpp"
#include "SDL.hpp"
#include "Strings.hpp"

MainMenuState::MainMenuState(void)
    : m_RenderTarget(SDL::TextureManager::CreateLoadTexture("MainMenuTarget", 200, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET)),
      m_Background(SDL::TextureManager::CreateLoadTexture("MainMenuBackground", "romfs:/Textures/MenuBackground.png")), m_MainMenu(50, 15, 555),
      m_ControlGuide(Strings::GetByName(Strings::Names::ControlGuides, 0)), m_ControlGuideX(1220 - SDL::Text::GetWidth(22, m_ControlGuide))
{
    // Fetch user list.
    Data::GetUsers(m_Users);

    // Loop through add user's icon to menu and create states.
    for (size_t i = 0; i < m_Users.size(); i++)
    {
        m_MainMenu.AddOption(m_Users.at(i)->GetSharedIcon());

        if (Config::GetByKey(Config::Keys::JKSMTextMode))
        {
            m_States.push_back(std::make_shared<TextTitleSelectState>(m_Users.at(i)));
        }
        else
        {
            m_States.push_back(std::make_shared<TitleSelectState>(m_Users.at(i)));
        }
    }
    // Add the settings and extras.
    m_States.push_back(std::make_shared<SettingsState>());
    m_States.push_back(std::make_shared<ExtrasMenuState>());

    // Create icons for the other two.
    m_SettingsIcon = SDL::TextureManager::CreateLoadTexture("SettingsIcon", "romfs:/Textures/SettingsIcon.png");
    m_ExtrasIcon = SDL::TextureManager::CreateLoadTexture("ExtrasIcon", "romfs:/Textures/ExtrasIcon.png");

    // Finally add them to the end.
    m_MainMenu.AddOption(m_SettingsIcon);
    m_MainMenu.AddOption(m_ExtrasIcon);
}

void MainMenuState::Update(void)
{
    m_MainMenu.Update(AppState::HasFocus());

    if (Input::ButtonPressed(HidNpadButton_A) && m_MainMenu.GetSelected() < static_cast<int>(m_Users.size()) &&
        m_Users.at(m_MainMenu.GetSelected())->GetTotalDataEntries() > 0)
    {
        m_States.at(m_MainMenu.GetSelected())->Reactivate();
        JKSV::PushState(m_States.at(m_MainMenu.GetSelected()));
    }
    else if (Input::ButtonPressed(HidNpadButton_A) && m_MainMenu.GetSelected() >= static_cast<int>(m_Users.size()))
    {
        m_States.at(m_MainMenu.GetSelected())->Reactivate();
        JKSV::PushState(m_States.at(m_MainMenu.GetSelected()));
    }
}

void MainMenuState::Render(void)
{
    // Clear render target by rendering background to it.
    m_Background->Render(m_RenderTarget->Get(), 0, 0);
    // Render menu.
    m_MainMenu.Render(m_RenderTarget->Get(), AppState::HasFocus());
    // Render target to screen.
    m_RenderTarget->Render(NULL, 0, 91);

    // Render next state for current user and control guide if this state has focus.
    if (AppState::HasFocus())
    {
        m_States.at(m_MainMenu.GetSelected())->Render();
        SDL::Text::Render(NULL, m_ControlGuideX, 673, 22, SDL::Text::NO_TEXT_WRAP, Colors::White, m_ControlGuide);
    }
}

void MainMenuState::RefreshViewStates(void)
{
    for (size_t i = 0; i < m_Users.size(); i++)
    {
        m_Users.at(i)->SortData();
        std::static_pointer_cast<TitleSelectCommon>(m_States.at(i))->Refresh();
    }
}
