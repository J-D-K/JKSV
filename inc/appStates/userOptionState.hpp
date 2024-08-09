#pragma once
#include <memory>

#include "appStates/appState.hpp"

#include "data/data.hpp"

#include "ui/ui.hpp"

class userOptionState : public appState
{
    public:
        userOptionState(data::user *currentUser);
        ~userOptionState();
        
        void update(void);
        void render(void);

    private:
        // Pointer to current userk
        data::user *m_CurrentUser;
        // Panel
        std::unique_ptr<ui::slidePanel> m_UserOptionPanel;
        // Menu
        std::unique_ptr<ui::menu> m_OptionsMenu;
};