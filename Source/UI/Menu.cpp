#include "UI/Menu.hpp"
#include "Colors.hpp"
#include "Config.hpp"
#include "Input.hpp"
#include "UI/RenderFunctions.hpp"

UI::Menu::Menu(int X, int Y, int Width, int FontSize, int ScrollLength)
    : m_X(X), m_Y(Y), m_OriginalY(Y), m_FontSize(FontSize), m_ScrollLength(ScrollLength), m_MenuRenderLength(ScrollLength * 2), m_Width(Width),
      m_Height(FontSize + 32)
{
    static int MenuID = 0;
    // Create target for menu option
    std::string MenuTargetName = "Menu_" + std::to_string(MenuID++);
    m_OptionTarget =
        SDL::TextureManager::CreateLoadTexture(MenuTargetName, m_Width, m_Height, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
}

void UI::Menu::Update(void)
{
    // Bail if there's nothing to update.
    if (m_Options.empty())
    {
        return;
    }

    if (Input::ButtonPressed(HidNpadButton_AnyUp) && --m_Selected < 0)
    {
        m_Selected = m_OptionsLength;
    }
    else if (Input::ButtonPressed(HidNpadButton_AnyDown) && ++m_Selected > m_OptionsLength)
    {
        m_Selected = 0;
    }
    else if (Input::ButtonPressed(HidNpadButton_Left) && (m_Selected -= m_ScrollLength) < 0)
    {
        m_Selected = 0;
    }
    else if (Input::ButtonPressed(HidNpadButton_AnyRight) && (m_Selected += m_ScrollLength) > m_OptionsLength)
    {
        m_Selected = m_OptionsLength;
    }

    // Calculate scrolling
    if (m_Selected < m_ScrollLength)
    {
        m_TargetY = m_OriginalY;
    }
    else if (m_Selected >= m_ScrollLength && m_OptionsLength > m_MenuRenderLength)
    {
        m_TargetY = m_OriginalY + -(m_Height * (m_OptionsLength * m_MenuRenderLength));
    }
    else if (m_Selected > m_MenuRenderLength && m_Selected < (m_OptionsLength - m_MenuRenderLength))
    {
        m_TargetY = -(m_Height * (m_Selected - m_MenuRenderLength));
    }

    if (m_Y != m_TargetY)
    {
        m_Y += std::ceil((m_TargetY - m_Y) / Config::GetAnimationScaling());
    }
}

void UI::Menu::Render(SDL_Texture *Target)
{
    if (m_Options.empty())
    {
        return;
    }

    m_ColorMod.Update();

    // I hate doing this.
    int TargetHeight = 0;
    SDL_QueryTexture(Target, NULL, NULL, NULL, &TargetHeight);
    for (int i = 0, TempY = m_Y; i < m_OptionsLength; i++, TempY += m_Height)
    {
        if (TempY < 0 || TempY > TargetHeight)
        {
            continue;
        }
        // Clear target texture.
        m_OptionTarget->Clear(Colors::Transparent);

        if (i == m_Selected)
        {
            // Render the bounding box
            UI::RenderBoundingBox(Target, m_X - 4, m_Y - 4, m_Width + 8, m_Height + 8, m_ColorMod);
            // Render the little rectangle.
            SDL::RenderRectFill(m_OptionTarget->Get(), 8, 2, 4, m_Height - 4, {0x00FFC5FF});
        }
        // Render text to target.
        SDL::Text::Render(m_OptionTarget->Get(),
                          14,
                          (m_Height / 2) - (m_FontSize / 2),
                          m_FontSize,
                          SDL::Text::NO_TEXT_WRAP,
                          i == m_Selected ? Colors::Blue : Colors::White,
                          m_Options.at(i).c_str());
        // Render target to target
        m_OptionTarget->Render(Target, m_X, TempY);
    }
}

void UI::Menu::AddOption(std::string_view NewOption)
{
    m_Options.push_back(NewOption.data());
    ++m_OptionsLength;
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
    m_OptionsLength = -1;
    m_Options.clear();
}
