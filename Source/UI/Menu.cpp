#include "UI/Menu.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Input.hpp"
#include "UI/RenderFunctions.hpp"
#include <cmath>

UI::Menu::Menu(int X, int Y, int Width, int FontSize, int RenderTargetHeight)
    : m_X(X), m_Y(Y), m_OptionHeight(std::ceil(static_cast<double>(FontSize) * 1.8f)), m_OriginalY(Y), m_TargetY(Y), m_Width(Width),
      m_FontSize(FontSize), m_RenderTargetHeight(RenderTargetHeight)
{
    // Create render target for options
    static int MenuID = 0;
    std::string MenuTargetName = "Menu_" + std::to_string(MenuID++);
    m_OptionTarget =
        SDL::TextureManager::CreateLoadTexture(MenuTargetName, m_Width, m_OptionHeight, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

    // Calculate around how many options can be shown on the render target at once.
    m_MaxDisplayOptions = (RenderTargetHeight - m_OriginalY) / m_OptionHeight;
    m_ScrollLength = std::floor(static_cast<double>(m_MaxDisplayOptions) / 2.0f);
}

void UI::Menu::Update(bool HasFocus)
{
    // Bail if there's nothing to update.
    if (m_Options.empty())
    {
        return;
    }

    int OptionsSize = m_Options.size();
    if (Input::ButtonPressed(HidNpadButton_AnyUp) && --m_Selected < 0)
    {
        m_Selected = OptionsSize - 1;
    }
    else if (Input::ButtonPressed(HidNpadButton_AnyDown) && ++m_Selected >= OptionsSize)
    {
        m_Selected = 0;
    }
    else if (Input::ButtonPressed(HidNpadButton_AnyLeft) && (m_Selected -= m_ScrollLength) < 0)
    {
        m_Selected = 0;
    }
    else if (Input::ButtonPressed(HidNpadButton_AnyRight) && (m_Selected += m_ScrollLength) >= OptionsSize)
    {
        m_Selected = OptionsSize - 1;
    }
    else if (Input::ButtonPressed(HidNpadButton_L) && (m_Selected -= m_ScrollLength * 3) < 0)
    {
        m_Selected = 0;
    }
    else if (Input::ButtonPressed(HidNpadButton_R) && (m_Selected += m_ScrollLength * 3) >= OptionsSize)
    {
        m_Selected = OptionsSize - 1;
    }

    // Don't bother continuing further if there's no reason to scroll.
    if (static_cast<int>(m_Options.size()) <= m_MaxDisplayOptions)
    {
        return;
    }

    if (m_Selected < m_ScrollLength)
    {
        m_TargetY = m_OriginalY;
    }
    else if (m_Selected >= static_cast<int>(m_Options.size()) - (m_MaxDisplayOptions - m_ScrollLength))
    {
        m_TargetY = m_OriginalY - ((m_Options.size() - m_MaxDisplayOptions) * m_OptionHeight);
    }
    else if (m_Selected >= m_ScrollLength)
    {
        m_TargetY = m_OriginalY - ((m_Selected - m_ScrollLength) * m_OptionHeight);
    }

    if (m_Y != m_TargetY)
    {
        m_Y += std::ceil((m_TargetY - m_Y) / Config::GetAnimationScaling());
    }
}

void UI::Menu::Render(SDL_Texture *Target, bool HasFocus)
{
    if (m_Options.empty())
    {
        return;
    }

    m_ColorMod.Update();

    // I hate doing this.
    int TargetHeight = 0;
    SDL_QueryTexture(Target, NULL, NULL, NULL, &TargetHeight);
    for (int i = 0, TempY = m_Y; i < static_cast<int>(m_Options.size()); i++, TempY += m_OptionHeight)
    {
        if (TempY < -m_FontSize)
        {
            continue;
        }

        // Clear target texture.
        m_OptionTarget->Clear(Colors::Transparent);

        if (i == m_Selected)
        {
            if (HasFocus)
            {
                // Render the bounding box
                UI::RenderBoundingBox(Target, m_X - 4, TempY - 4, m_Width + 8, m_OptionHeight + 8, m_ColorMod);
            }
            // Render the little rectangle.
            SDL::RenderRectFill(m_OptionTarget->Get(), 8, 8, 4, m_OptionHeight - 16, Colors::BlueGreen);
        }
        // Render text to target.
        SDL::Text::Render(m_OptionTarget->Get(),
                          24,
                          (m_OptionHeight / 2) - (m_FontSize / 2),
                          m_FontSize,
                          SDL::Text::NO_TEXT_WRAP,
                          i == m_Selected && HasFocus ? Colors::BlueGreen : Colors::White,
                          m_Options.at(i).c_str());
        // Render target to target
        m_OptionTarget->Render(Target, m_X, TempY);
    }
}

void UI::Menu::AddOption(std::string_view NewOption)
{
    m_Options.push_back(NewOption.data());
}

int UI::Menu::GetSelected(void) const
{
    return m_Selected;
}

void UI::Menu::SetSelected(int Selected)
{
    m_Selected = Selected;
}

void UI::Menu::Reset(void)
{
    m_Options.clear();
}
