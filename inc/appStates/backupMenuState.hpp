#pragma once
#include <string>
#include "data/userSaveInfo.hpp"
#include "data/titleInfo.hpp"
#include "filesystem/filesystem.hpp"
#include "ui/slidePanel.hpp"
#include "ui/menu.hpp"
#include "appStates/appState.hpp"
#include "system/task.hpp"

class backupMenuState : public appState
{
    public:
        backupMenuState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo);
        ~backupMenuState();

        void update(void);
        void render(void);

        // Loads directory listing and copies it to menu
        void loadDirectoryList(void);

    private:
        // Pointer to current user
        data::user *m_CurrentUser;
        // Pointer to current user's save
        data::userSaveInfo *m_CurrentUserSaveInfo;
        // Current title pointer
        data::titleInfo *m_CurrentTitleInfo;
        // String for control guide
        std::string m_BackupMenuControlGuide;
        // Base path for output
        std::string m_OutputBasePath;
        // Slide in/out panel
        std::unique_ptr<ui::slidePanel> m_BackupPanel;
        // Folder listing
        std::unique_ptr<fs::directoryListing> m_BackupListing;
        // Folder menu
        std::unique_ptr<ui::menu> m_BackupMenu;
};
