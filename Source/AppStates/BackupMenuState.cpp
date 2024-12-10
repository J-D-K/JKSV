#include "AppStates/BackupMenuState.hpp"
#include "Config.hpp"
#include "SDL.hpp"
#include "Strings.hpp"

BackupMenuState::BackupMenuState(Data::User *User, Data::TitleInfo *TitleInfo)
    : m_User(User), m_TitleInfo(TitleInfo), m_DirectoryPath(Config::GetWorkingDirectory() / m_TitleInfo->GetPathSafeTitle())
{
    // Calculate these if they weren't already by a previous instance.
    if (m_PanelWidth == 0 || m_CurrentBackupsCoordinate == 0)
    {
        m_PanelWidth = SDL::Text::GetWidth(22, Strings::GetByName(Strings::Names::ControlGuides, 2)) + 64;
        m_CurrentBackupsCoordinate = (m_PanelWidth / 2) - (SDL::Text::GetWidth(28, Strings::GetByName(Strings::Names::BackupMenu, 0)) / 2);
    }

    BackupMenuState::RefreshListing();
}

void BackupMenuState::Update(void)
{
}

void BackupMenuState::Render(void)
{
}

void BackupMenuState::RefreshListing(void)
{
    m_DirectoryListing.Open(m_DirectoryPath);
    if (!m_DirectoryListing.IsOpen())
    {
        return false;
    }

    m_BackupMenu.Reset();
    for (int64_t i = 0; i < m_DirectoryListing.GetEntryCount(); i++)
    {
        m_BackupMenu.AddOption(m_DirectoryListing[i]);
    }
}
