#pragma once
#include "AppStates/AppState.hpp"
#include "Data/Data.hpp"
#include "SDL.hpp"
#include "UI/TitleView.hpp"

class TitleSelectState : public AppState
{
    public:
        TitleSelectState(Data::User *User);
        ~TitleSelectState() {};

        void Update(void);
        void Render(void);

    private:
        // Save pointer to user
        Data::User *m_User = nullptr;
        // Render target.
        SDL::SharedTexture m_RenderTarget = nullptr;
        // Title select view
        UI::TitleView m_TitleView;
        // X coordinate for control guide. Shared between all instances. Only should be calculated once.
        static inline int m_TitleControlsX = 0;
};
