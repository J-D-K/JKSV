#include "UI/TitleView.hpp"
#include "Config.hpp"

UI::TitleView::TitleView(Data::User *User) : m_User(User)
{
    for (size_t i = 0; i < m_User->GetTotalDataEntries(); i++)
    {
        // Get pointer to data from user save index I.
        Data::TitleInfo *CurrentTitleInfo = Data::GetTitleInfoByID(m_User->GetApplicationIDAt(i));
        // Emplace is faster than push
        m_TitleTiles.emplace_back(Config::IsFavorite(m_User->GetApplicationIDAt(i)), CurrentTitleInfo->GetIcon());
    }
}
