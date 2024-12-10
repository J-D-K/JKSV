#pragma once
#include "AppStates/AppState.hpp"
#include "Data/Data.hpp"
#include "FsLib.hpp"
#include "UI/Menu.hpp"
#include "UI/RenderTarget.hpp"
#include "UI/SlideOutPanel.hpp"

class BackupMenuState : public AppState
{
    public:
        BackupMenuState(Data::User *User, Data::TitleInfo *TitleInfo);
        ~BackupMenuState() {};

        void Update(void);
        void Render(void);
        // Refreshes/Updates menu and listing.
        void RefreshListing(void);

    private:
        // Pointer to user.
        Data::User *m_User;
        // Pointer to title info.
        Data::TitleInfo *m_TitleInfo;
        // Backup folder path
        FsLib::Path m_DirectoryPath;
        // Directory listing of that folder.
        FsLib::Directory m_DirectoryListing;
        // Menu
        UI::Menu m_BackupMenu;
        // Render target for menu.
        UI::RenderTarget m_MenuTarget;
        // Slide panel.
        UI::SlideOutPanel m_SlidePanel;
        // Width of panel.
        static inline int m_PanelWidth = 0;
        // X coordinate of text above menu.
        static inline int m_CurrentBackupsCoordinate = 0;
};
