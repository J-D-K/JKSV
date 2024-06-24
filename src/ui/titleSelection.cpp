#include <cmath>
#include "data/titleInfo.hpp"
#include "ui/titleSelection.hpp"
#include "ui/ui.hpp"
#include "system/input.hpp"
#include "config.hpp"

namespace
{
    // Starting coordinates for rendering loop
    const int ICON_STARTING_X = 32;
    const int ICON_STARTING_Y = 38;
    // Icon dimensions
    const int ICON_WIDTH = 128;
    const int ICON_HEIGHT = 128;
    // Gap size in between icons
    const int ICON_GAP_HORIZONTAL = 16;
    const int ICON_GAP_VERTICAL = 16;
    // How many icons per row
    const int ICON_ROW_SIZE = 7;
    // I really need to figure out what this is and where it came from. I don't remember.
    const int TARGET_Y_START = 280;
}

ui::titleSelection::titleSelection(data::user *currentUser) : 
m_X(ICON_STARTING_X),
m_Y(ICON_STARTING_Y),
m_CurrentUser(currentUser),
m_Selected(0),
m_ColorMod(0)
{
    for (int i = 0; i < m_CurrentUser->getTotalUserSaveInfo(); i++)
    {
        data::userSaveInfo *currentUserSaveInfo = m_CurrentUser->getUserSaveInfoAt(i);
        data::titleInfo *currentTitleInfo = data::getTitleInfoByTitleID(currentUserSaveInfo->getTitleID());
        m_TitleTiles.emplace_back(ICON_WIDTH, ICON_HEIGHT, config::titleIsFavorite(currentUserSaveInfo->getTitleID()), currentTitleInfo->getIcon());
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
    if (sys::input::buttonDown(HidNpadButton_AnyUp) && (m_Selected -= ICON_ROW_SIZE) < 0)
    {
        m_Selected = 0;
    }
    else if (sys::input::buttonDown(HidNpadButton_AnyDown) && (m_Selected += ICON_ROW_SIZE) > totalTiles)
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
    else if (sys::input::buttonDown(HidNpadButton_L) && (m_Selected -= ICON_ROW_SIZE * 3) < 0)
    {
        m_Selected = 0;
    }
    else if (sys::input::buttonDown(HidNpadButton_R) && (m_Selected += ICON_ROW_SIZE * 3) > totalTiles)
    {
        m_Selected = totalTiles;
    }

    // Calculate scrolling
    float animationScaling = config::getAnimationScaling();
    m_TargetY = TARGET_Y_START;
    if (m_SelectionY > m_TargetY)
    {
        double addToY = (static_cast<double>(m_TargetY) - (static_cast<double>(m_SelectionY))) / animationScaling;
        m_Y += std::ceil(addToY);
    }
    else if (m_SelectionY < ICON_STARTING_Y)
    {
        double addToY = (static_cast<double>(ICON_STARTING_Y) - static_cast<double>(m_SelectionY)) / animationScaling;
        m_Y += std::ceil(addToY);
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
    for (int i = 0; i < totalTiles; m_TargetY += (ICON_HEIGHT + ICON_GAP_VERTICAL))
    {
        int endOfRow = i + ICON_ROW_SIZE;
        m_TargetX = m_X;
        for (; i < endOfRow; m_TargetX += (ICON_WIDTH + ICON_GAP_HORIZONTAL), i++)
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