#pragma once
#include <memory>
#include <string>
#include <vector>

#include "appStates/appState.hpp"

#include "data/data.hpp"

#include "ui/ui.hpp"

class saveCreateState : public appState
{
    public:
        saveCreateState(data::user *currentUser, FsSaveDataType saveDataType);
        ~saveCreateState();

        void update(void);
        void render(void);

    private:
        // Pointer to currentUser
        data::user *m_CurrentUser;
        // Slide panel
        std::unique_ptr<ui::slidePanel> m_SaveCreatePanel;
        // Menu
        std::unique_ptr<ui::menu> m_SaveCreateMenu;
        // Vector containing pair of title id and and game title
        std::vector<std::pair<u128, std::string>> m_TitleList;
        // Type of save data we're working with.
        FsSaveDataType m_SaveDataType;
};