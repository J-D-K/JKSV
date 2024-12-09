#pragma once
#include "AppStates/AppState.hpp"
#include "SDL.hpp"
#include "UI/Menu.hpp"

class ExtrasMenuState : public AppState
{
    public:
        ExtrasMenuState(void);
        ~ExtrasMenuState() {};

        void Update(void);
        void Render(void);

    private:
        // Actual menu
        UI::Menu m_ExtrasMenu;
        // Render target
        SDL::SharedTexture m_RenderTarget;
};
