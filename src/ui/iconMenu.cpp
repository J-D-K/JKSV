#include "ui/iconMenu.hpp"
#include "ui/ui.hpp"

ui::iconMenu::iconMenu(const int& x, const int& y, const int& maxScroll) : menu(x, y, 128, 128, maxScroll) { }

void ui::iconMenu::addOpt(SDL_Texture *newOpt)
{
    //Parent class needs a text string to register newOpt. Might fix this later
    menu::addOpt("ICON");
    m_MenuOptions.push_back(newOpt);
}

void ui::iconMenu::render(SDL_Texture *target)
{
    if(m_MenuOptions.empty())
    {
        return;
    }

    menu::updateColorPulse(); 

    for(unsigned int i = 0; i < m_MenuOptions.size(); i++)
    {
        //This is special and hardcoded mostly
        if(m_Selected == static_cast<int>(i))
        {
            ui::renderSelectionBox(target, m_X - 8, (m_Y + (i * m_RectHeight)) - 8, 144, 144, m_ColorMod);
        }

        //iconMenu is hardcoded for 128x128 icons
        graphics::textureRenderStretched(m_MenuOptions[i], target, m_X, m_Y + (i * m_RectHeight), 128, 128);
    }
}