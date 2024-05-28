#pragma once
#include <string>
#include <SDL2/SDL.h>
#include "data/data.hpp"
#include "filesystem/filesystem.hpp"
#include "appStates/appState.hpp"
#include "ui/ui.hpp"

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
        // Width of panel/text
        int m_PanelWidth;
        // Folder listing
        std::unique_ptr<fs::directoryListing> m_BackupListing;
        // Listing to make sure there is a save
        std::unique_ptr<fs::directoryListing> m_SaveListing;
        // Folder menu
        std::unique_ptr<ui::menu> m_BackupMenu;
        // Render target for menu
        SDL_Texture *m_BackupMenuRenderTarget = NULL;
};
