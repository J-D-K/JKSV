#include <cmath>

#include "ui/menu.hpp"
#include "ui/ui.hpp"

#include "system/input.hpp"

#include "graphics/systemFont.hpp"

#include "config.hpp"

namespace
{
    int menuID = 0;
    // This is how much extra vertical space is needed for bounding/selection box
    const int BOUNDING_EXTRA_HEIGHT = 32;
}

ui::menu::menu(int x, int y, int rectWidth, int fontSize, int maxScroll) : m_X(x),
                                                                           m_Y(y),
                                                                           m_OriginalY(y),
                                                                           m_FontSize(fontSize),
                                                                           m_RectWidth(rectWidth),
                                                                           m_RectHeight(m_FontSize + BOUNDING_EXTRA_HEIGHT),
                                                                           m_Selected(0),
                                                                           m_MaxScroll(maxScroll)
{
    // String for rendertarget
    std::string menuTargetName = "menuTarget_" + std::to_string(menuID++);
    // This is for rendering the options to. So text can't be rendered outside bounding box.
    m_OptionRenderTarget = graphics::textureManager::createTexture(menuTargetName, m_RectWidth, m_RectHeight, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
}

ui::menu::menu(int x, int y, int rectWidth, int fontSize, int maxScroll, const std::string *menuOptions, int menuOptCount) : menu(x, y, rectWidth, fontSize, maxScroll)
{
    for (int i = 0; i < menuOptCount; i++)
    {
        m_MenuOptions.push_back(std::string(menuOptions[i]));
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
    else if (sys::input::buttonDown(HidNpadButton_AnyLeft) && (m_Selected -= m_MaxScroll) < 0)
    {
        m_Selected = 0;
    }
    else if (sys::input::buttonDown(HidNpadButton_AnyRight) && (m_Selected += m_MaxScroll) > menuSize)
    {
        m_Selected = menuSize;
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
        double addToY = static_cast<double>(static_cast<double>(m_TargetY) - static_cast<double>(m_Y)) / config::getAnimationScaling();
        m_Y += std::ceil(addToY);
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
        // Clear option target
        graphics::textureClear(m_OptionRenderTarget.get(), COLOR_TRANSPARENT);
        // Render option to it.
        graphics::systemFont::renderText(m_MenuOptions.at(i), m_OptionRenderTarget.get(), 16, (m_RectHeight / 2) - (m_FontSize /2), m_FontSize, COLOR_WHITE);
        // Render option target to target
        graphics::textureRender(m_OptionRenderTarget.get(), target, m_X, m_Y + (i * m_RectHeight));
        // If it's the selected option, render the selection box over top.
        if (m_Selected == i)
        {
            ui::renderSelectionBox(target, m_X, (m_Y + (i * m_RectHeight)), m_RectWidth, m_RectHeight, m_ColorMod);
        }
    }
}

void ui::menu::addOption(const std::string &newOption)
{
    m_MenuOptions.push_back(newOption);
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

void ui::menu::setSelected(int newSelected)
{
    m_Selected = newSelected;
}

void ui::menu::clearMenu(void)
{
    m_MenuOptions.clear();
}