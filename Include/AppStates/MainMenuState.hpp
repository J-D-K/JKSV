#pragma once
#include "AppStates/AppState.hpp"
#include "SDL.hpp"

class MainMenuState : public AppState
{
    public:
        MainMenuState(void);
        ~MainMenuState() {};

        void Update(void);
        void Render(void);

    private:
        // The render target.
        SDL::SharedTexture m_RenderTarget = nullptr;
        // Background of the menu.
        SDL::SharedTexture m_Background = nullptr;
        // Pointer to control guide string so I don't need to call and fetch it every loop.
        const char *m_ControlGuide = nullptr;
        // X coordinate to render controls at.
        int m_ControlGuideX;
};
