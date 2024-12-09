#pragma once
#include "AppStates/TitleSelectCommon.hpp"
#include "Data/Data.hpp"
#include "SDL.hpp"
#include "UI/Menu.hpp"

class TextTitleSelectState : public TitleSelectCommon
{
    public:
        TextTitleSelectState(Data::User *User);
        ~TextTitleSelectState() {};

        void Update(void);
        void Render(void);

        void Refresh(void);

    private:
        // Pointer to user to save.
        Data::User *m_User;
        // Title select menu
        UI::Menu m_TitleSelectMenu;
        // Render target
        SDL::SharedTexture m_RenderTarget;
};
