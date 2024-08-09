#pragma once
#include <memory>
#include <string>

#include "appStates/appState.hpp"

#include "ui/iconMenu.hpp"

class mainMenuState : public appState
{
    public:
        mainMenuState(void);
        ~mainMenuState();

        void update(void);
        void render(void);

    private:
        // Control guide string
        std::string m_MainControlGuide;
        // X coordinate of control guide
        int m_MainControlGuideX = 0;
        // Menu on left of screen
        std::unique_ptr<ui::iconMenu> m_MainMenu;
        // Texture behind menu
        graphics::sdlTexture m_MenuBackgroundTexture;
        // Render target
        graphics::sdlTexture m_RenderTarget;
        // End of users for settings and extras
        int m_UserEnd;
};