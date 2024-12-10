#pragma once
#include "AppStates/AppState.hpp"
#include "Data/Data.hpp"
#include "SDL.hpp"
#include "UI/IconMenu.hpp"

class MainMenuState : public AppState
{
    public:
        MainMenuState(void);
        ~MainMenuState() {};

        void Update(void);
        void Render(void);

        // This allows other parts of the program to signal to this one to refresh the view states on the next Update() call.
        static void RefreshViewStates(void);

    private:
        // The render target.
        SDL::SharedTexture m_RenderTarget = nullptr;
        // Background of the menu.
        SDL::SharedTexture m_Background = nullptr;
        // Icons for the last two.
        SDL::SharedTexture m_SettingsIcon = nullptr;
        SDL::SharedTexture m_ExtrasIcon = nullptr;
        // Icon menu for users
        UI::IconMenu m_MainMenu;
        // Vector of pointers to users.
        static inline std::vector<Data::User *> m_Users;
        // Vector of view states for each user, settings, and extras
        static inline std::vector<std::shared_ptr<AppState>> m_States;
        // Pointer to control guide string so I don't need to call and fetch it every loop.
        const char *m_ControlGuide = nullptr;
        // X coordinate to render controls at.
        int m_ControlGuideX;
        // Variable that holds refresh signal.
        static inline bool m_RefreshNeeded = false;
};
