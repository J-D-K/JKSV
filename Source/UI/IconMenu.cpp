#include "UI/IconMenu.hpp"
#include "Colors.hpp"
#include "UI/RenderFunctions.hpp"

UI::IconMenu::IconMenu(int X, int Y, int ScrollLength, int RenderTargetHeight) : Menu(X, Y, 152, 88, ScrollLength, RenderTargetHeight) {};

void UI::IconMenu::Update(bool HasFocus)
{
    Menu::Update(HasFocus);
}

void UI::IconMenu::Render(SDL_Texture *Target, bool HasFocus)
{
    if (HasFocus)
    {
        m_ColorMod.Update();
    }

    for (int i = 0, TempY = m_Y; i <= m_OptionsLength; i++, TempY += m_OptionHeight)
    {
        // Clear target.
        m_OptionTarget->Clear(Colors::Transparent);
        if (i == m_Selected)
        {
            if (HasFocus)
            {
                UI::RenderBoundingBox(Target, m_X - 8, TempY - 8, 152, 146, m_ColorMod);
            }
            SDL::RenderRectFill(m_OptionTarget->Get(), 0, 0, 6, 130, {0x00FFC5FF});
        }
        //m_Options.at(i)->Render(m_OptionTarget->Get(), 0, 0);
        m_Options.at(i)->RenderStretched(m_OptionTarget->Get(), 8, 1, 128, 128);
        m_OptionTarget->Render(Target, m_X, TempY);
    }
}

void UI::IconMenu::AddOption(SDL::SharedTexture NewOption)
{
    // Parent needs a text option to work correctly.
    Menu::AddOption("ICON");
    m_Options.push_back(NewOption);
}
