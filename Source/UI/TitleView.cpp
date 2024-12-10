#include "UI/TitleView.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Input.hpp"
#include "Logger.hpp"
#include "UI/RenderFunctions.hpp"
#include <cmath>

namespace
{
    constexpr int ICON_ROW_SIZE = 7;
}

UI::TitleView::TitleView(Data::User *User) : m_User(User)
{
    TitleView::Refresh();
}

void UI::TitleView::Update(bool HasFocus)
{
    if (m_TitleTiles.empty())
    {
        return;
    }

    // Update pulse
    if (HasFocus)
    {
        m_ColorMod.Update();
    }

    // Input.
    int TotalTiles = m_TitleTiles.size() - 1;
    if (Input::ButtonPressed(HidNpadButton_AnyUp) && (m_Selected -= ICON_ROW_SIZE) < 0)
    {
        m_Selected = 0;
    }
    else if (Input::ButtonPressed(HidNpadButton_AnyDown) && (m_Selected += ICON_ROW_SIZE) > TotalTiles)
    {
        m_Selected = TotalTiles;
    }
    else if (Input::ButtonPressed(HidNpadButton_AnyLeft) && m_Selected > 0)
    {
        --m_Selected;
    }
    else if (Input::ButtonPressed(HidNpadButton_AnyRight) && m_Selected < TotalTiles)
    {
        ++m_Selected;
    }
    else if (Input::ButtonPressed(HidNpadButton_L) && (m_Selected -= 21) < 0)
    {
        m_Selected = 0;
    }
    else if (Input::ButtonPressed(HidNpadButton_R) && (m_Selected += 21) > TotalTiles)
    {
        m_Selected = TotalTiles;
    }

    double Scaling = Config::GetAnimationScaling();
    if (m_SelectedY > 388.0f)
    {
        m_Y += std::ceil((388.0f - m_SelectedY) / Scaling);
    }
    else if (m_SelectedY < 28.0f)
    {
        m_Y += std::ceil((28.0f - m_SelectedY) / Scaling);
    }

    for (size_t i = 0; i < m_TitleTiles.size(); i++)
    {
        m_TitleTiles.at(i).Update(m_Selected == static_cast<int>(i) ? true : false);
    }
}

void UI::TitleView::Render(SDL_Texture *Target, bool HasFocus)
{
    if (m_TitleTiles.empty())
    {
        return;
    }

    for (int i = 0, TempY = m_Y; i < static_cast<int>(m_TitleTiles.size()); TempY += 144)
    {
        int EndRow = i + 7;
        for (int j = i, TempX = 32; j < EndRow; j++, i++, TempX += 144)
        {
            if (i >= static_cast<int>(m_TitleTiles.size()))
            {
                break;
            }

            // Save the X and Y to render the selected tile over the rest.
            if (i == m_Selected)
            {
                m_SelectedX = TempX;
                m_SelectedY = TempY;
                continue;
            }
            // Just render
            m_TitleTiles.at(i).Render(Target, TempX, TempY);
        }
    }
    // Now render the selected title.
    if (HasFocus)
    {
        SDL::RenderRectFill(Target, m_SelectedX - 23, m_SelectedY - 23, 174, 174, Colors::ClearColor);
        UI::RenderBoundingBox(Target, m_SelectedX - 24, m_SelectedY - 24, 176, 176, m_ColorMod);
    }
    m_TitleTiles.at(m_Selected).Render(Target, m_SelectedX, m_SelectedY);
}

int UI::TitleView::GetSelected(void) const
{
    return m_Selected;
}

void UI::TitleView::Refresh(void)
{
    m_TitleTiles.clear();
    for (size_t i = 0; i < m_User->GetTotalDataEntries(); i++)
    {
        // Get pointer to data from user save index I.
        Data::TitleInfo *CurrentTitleInfo = Data::GetTitleInfoByID(m_User->GetApplicationIDAt(i));
        // Emplace is faster than push
        m_TitleTiles.emplace_back(Config::IsFavorite(m_User->GetApplicationIDAt(i)), CurrentTitleInfo->GetIcon());
    }
}

void UI::TitleView::Reset(void)
{
    for (UI::TitleTile &CurrentTile : m_TitleTiles)
    {
        CurrentTile.Reset();
    }
}
