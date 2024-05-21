#include <cmath>
#include "data/titleInfo.hpp"
#include "ui/titleSelection.hpp"
#include "ui/ui.hpp"
#include "system/input.hpp"
#include "config.hpp"

ui::titleSelection::titleSelection(data::user *currentUser) : m_CurrentUser(currentUser)
{
    // To do: Not this. Originally were passed as variables
    m_X = 32;
    m_Y = 38;
    m_IconWidth = 128;
    m_IconHeight = 128;
    m_HorizontalGap = 16;
    m_VerticalGap = 16;
    m_RowSize = 7;
    m_Selected = 0;
    m_ColorMod = 0;

    for (int i = 0; i < m_CurrentUser->getTotalUserSaveInfo(); i++)
    {
        data::userSaveInfo *currentUserSaveInfo = m_CurrentUser->getUserSaveInfoAt(i);
        data::titleInfo *currentTitleInfo = data::getTitleInfoByTitleID(currentUserSaveInfo->getTitleID());
        m_TitleTiles.emplace_back(128, 128, config::titleIsFavorite(currentUserSaveInfo->getTitleID()), currentTitleInfo->getIcon());
    }
}

ui::titleSelection::~titleSelection()
{
}

int ui::titleSelection::getSelected(void)
{
    return m_Selected;
}

void ui::titleSelection::update(void)
{
    int totalTiles = m_TitleTiles.size() - 1;
    if (m_Selected > totalTiles && m_Selected > 0)
    {
        m_Selected = m_TitleTiles.size() - 1;
    }

    // I chose this over switch case because the _AnyX can't work that way
    if (sys::input::buttonDown(HidNpadButton_AnyUp) && (m_Selected -= m_RowSize) < 0)
    {
        m_Selected = 0;
    }
    else if (sys::input::buttonDown(HidNpadButton_AnyDown) && (m_Selected += m_RowSize) > totalTiles)
    {
        m_Selected = totalTiles;
    }
    else if (sys::input::buttonDown(HidNpadButton_AnyLeft) && m_Selected > 0)
    {
        --m_Selected;
    }
    else if (sys::input::buttonDown(HidNpadButton_AnyRight) && m_Selected < totalTiles)
    {
        ++m_Selected;
    }
    else if (sys::input::buttonDown(HidNpadButton_L) && (m_Selected -= m_RowSize * 3) < 0)
    {
        m_Selected = 0;
    }
    else if (sys::input::buttonDown(HidNpadButton_R) && (m_Selected += m_RowSize * 3) > totalTiles)
    {
        m_Selected = totalTiles;
    }

    // Calculate scrolling
    float animationScaling = config::getAnimationScaling();
    m_TargetY = 280;
    if (m_SelectionY > m_TargetY)
    {
        float addToY = ((float)m_TargetY - (float)m_SelectionY) / animationScaling;
        m_Y += ceil(addToY);
    }
    else if (m_SelectionY < 38)
    {
        float addToY = (38.0f - (float)m_SelectionY) / animationScaling;
        m_Y += ceil(addToY);
    }

    // Update title titles
    for (unsigned int i = 0; i < m_TitleTiles.size(); i++)
    {
        if (m_Selected == static_cast<int>(i))
        {
            m_TitleTiles.at(i).update(true);
        }
        else
        {
            m_TitleTiles.at(i).update(false);
        }
    }
}

void ui::titleSelection::render(SDL_Texture *target)
{
    // Update pulse
    if (m_ColorShift && (m_ColorMod += 6) >= 0x72)
    {
        m_ColorShift = false;
    }
    else if (!m_ColorShift && (m_ColorMod -= 3) <= 0x00)
    {
        m_ColorShift = true;
    }

    int totalTiles = m_TitleTiles.size();
    m_TargetY = m_Y;
    for (int i = 0; i < totalTiles; m_TargetY += (m_IconHeight + m_VerticalGap))
    {
        int endOfRow = i + m_RowSize;
        m_TargetX = m_X;
        for (; i < endOfRow; m_TargetX += (m_IconWidth + m_HorizontalGap), i++)
        {
            if (i >= totalTiles)
            {
                break;
            }

            if (i == m_Selected)
            {
                // Selected needs to be drawn later so it's on top
                m_SelectionX = m_TargetX;
                m_SelectionY = m_TargetY;
            }
            else
            {
                m_TitleTiles[i].render(target, m_TargetX, m_TargetY);
            }
        }
    }

    // This renders the selected title icon so it's over top of the surrounding ones
    graphics::renderRect(target, m_SelectionX - 23, m_SelectionY - 23, 174, 174, COLOR_DEFAULT_CLEAR);
    m_TitleTiles[m_Selected].render(target, m_SelectionX, m_SelectionY);
    ui::renderSelectionBox(target, m_SelectionX - 24, m_SelectionY - 24, 176, 176, m_ColorMod);
}