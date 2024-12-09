#pragma once
#include "AppStates/TitleSelectCommon.hpp"
#include "Data/Data.hpp"
#include "SDL.hpp"
#include "UI/TitleView.hpp"

class TitleSelectState : public TitleSelectCommon
{
    public:
        TitleSelectState(Data::User *User);
        ~TitleSelectState() {};

        void Update(void);
        void Render(void);

        void Refresh(void);

    private:
        // Save pointer to user
        Data::User *m_User = nullptr;
        // Render target.
        SDL::SharedTexture m_RenderTarget = nullptr;
        // Title select view
        UI::TitleView m_TitleView;
};
