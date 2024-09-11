#pragma once
#include <memory>
#include <SDL2/SDL.h>
#include "appStates/appState.hpp"
#include "ui/titleSelection.hpp"
#include "data/user.hpp"

class titleSelectionState : public appState
{
    public:
        titleSelectionState(data::user *currentUser);
        ~titleSelectionState();

        void update(void);
        void render(void);

    private:
        // Pointer to current user
        data::user *m_CurrentUser;
        // Grid of icons
        std::unique_ptr<ui::titleSelection> m_TitleSelection;
        // Control guide for bottom right
        std::string m_TitleControlGuide;
        // X position of above
        int m_TitleControlGuideX;
        // Render target
        graphics::sdlTexture m_RenderTarget;
};

inline std::shared_ptr<titleSelectionState> createTitleSelectionState(data::user *currentUser)
{
    return std::make_shared<titleSelectionState>(currentUser);
}
