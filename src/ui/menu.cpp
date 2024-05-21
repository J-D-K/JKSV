#include "ui/menu.hpp"
#include "ui/ui.hpp"
#include "system/input.hpp"
#include "graphics/systemFont.hpp"
#include "config.hpp"

ui::menu::menu(const int &x, const int &y, const int &rectWidth, const int &fontSize, const int &maxScroll) : m_X(x), m_Y(y), m_OriginalY(y), m_RectWidth(rectWidth), m_FontSize(fontSize), m_MaxScroll(maxScroll)
{
    m_RectHeight = m_FontSize + 30;
    m_Selected = 0;
}

ui::menu::menu(const int &x, const int &y, const int &rectWidth, const int &fontSize, const int &maxScroll, const std::string *menuOpts, const int &menuOptCount) : menu(x, y, rectWidth, fontSize, maxScroll)
{
    for (int i = 0; i < menuOptCount; i++)
    {
        m_MenuOptions.push_back(menuOpts[i]);
    }
}

ui::menu::~menu() {}

void ui::menu::update(void)
{
    if (m_MenuOptions.empty())
    {
        return;
    }

    // Input handling
    int menuSize = m_MenuOptions.size() - 1;
    if (sys::input::buttonDown(HidNpadButton_AnyUp) && --m_Selected < 0)
    {
        m_Selected = menuSize;
    }
    else if (sys::input::buttonDown(HidNpadButton_AnyDown) && ++m_Selected > menuSize)
    {
        m_Selected = 0;
    }

    // Calculate if scrolling needs to happen
    if (m_Selected <= m_MaxScroll)
    {
        m_TargetY = m_OriginalY;
    }
    else if (m_Selected >= (menuSize - m_MaxScroll) && menuSize > m_MaxScroll * 2)
    {
        m_TargetY = m_OriginalY + -(m_RectHeight * (menuSize - (m_MaxScroll * 2)));
    }
    else if (m_Selected > m_MaxScroll && m_Selected < (menuSize - m_MaxScroll))
    {
        m_TargetY = -(m_RectHeight * (m_Selected - m_MaxScroll));
    }

    // Calculate scroll amount
    if (m_Y != m_TargetY)
    {
        float addY = (float)((float)m_TargetY - (float)m_Y) / config::getAnimationScaling();
        m_Y += addY;
    } 
}

void ui::menu::render(SDL_Texture *target)
{
    if (m_MenuOptions.empty())
    {
        return;
    }

    updateColorPulse();

    int targetHeight;
    SDL_QueryTexture(target, NULL, NULL, NULL, &targetHeight);
    for (int i = 0; i < (int)m_MenuOptions.size(); i++)
    {
        if (m_Selected == i)
        {
            ui::renderSelectionBox(target, m_X - 4, (m_Y + (i * m_RectHeight)) - 4, m_RectWidth, m_RectHeight, m_ColorMod);
        }
        graphics::systemFont::renderText(m_MenuOptions.at(i), target, m_X + 8, m_Y + (i * m_RectHeight) + (m_RectHeight / 2 - m_FontSize / 2), m_FontSize, COLOR_WHITE);
    }
}

void ui::menu::addOpt(const std::string &newOpt)
{
    m_MenuOptions.push_back(newOpt);
}

void ui::menu::updateColorPulse(void)
{
     if (m_ColorShift && (m_ColorMod += 6) >= 0x72)
    {
        m_ColorShift = false;
    }
    else if (!m_ColorShift && (m_ColorMod -= 3) <= 0x00)
    {
        m_ColorShift = true;
    }
}

int ui::menu::getSelected(void) const
{
    return m_Selected;
}

void ui::menu::clearMenu(void)
{
    m_MenuOptions.clear();
}