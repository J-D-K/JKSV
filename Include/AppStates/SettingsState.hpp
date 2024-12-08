#pragma once
#include "AppStates/AppState.hpp"
#include "SDL.hpp"
#include "UI/Menu.hpp"

class SettingsState : public AppState
{
    public:
        SettingsState(void);
        ~SettingsState() {};

        void Update(void);
        void Render(void);

    private:
        // Menu containing options.
        UI::Menu m_SettingsMenu;
        // Render target.
        SDL::SharedTexture m_RenderTarget = nullptr;
};
