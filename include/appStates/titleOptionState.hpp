#pragma once
#include <memory>

#include "appStates/appState.hpp"

#include "data/data.hpp"

#include "ui/ui.hpp"

class titleOptionState : public appState
{
    public:
        titleOptionState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo,  data::titleInfo *currentTitleInfo);
        ~titleOptionState();

        void update(void);
        void render(void);

    private:
        // Pointer to current user
        data::user *m_CurrentUser;
        // Pointer to current user's save info
        data::userSaveInfo *m_CurrentUserSaveInfo;
        // Pointer to current title
        data::titleInfo *m_CurrentTitleInfo;
        // Slide out panel
        std::unique_ptr<ui::slidePanel> m_SlidePanel;
        // Menu
        std::unique_ptr<ui::menu> m_OptionsMenu;
};

inline std::shared_ptr<titleOptionState> createTitleOptionState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo)
{
    return std::make_shared<titleOptionState>(currentUser, currentUserSaveInfo, currentTitleInfo);
}
