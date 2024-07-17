#include "ui/iconMenu.hpp"
#include "ui/ui.hpp"

namespace
{
    // Icon size
    const int ICON_WIDTH = 128;
    const int ICON_HEIGHT = 128;
    // Bounding box width and height
    const int BOUNDING_WIDTH = 144;
    const int BOUNDING_HEIGHT = 144;
}

ui::iconMenu::iconMenu(int x, int y, int maxScroll) : 
menu(x, y, ICON_WIDTH, ICON_HEIGHT, maxScroll) { }

void ui::iconMenu::addOpt(SDL_Texture *newOption)
{
    //Parent class needs a text string to register newOpt. Might fix this later.
    menu::addOption("ICON");
    m_MenuOptions.push_back(newOption);
}

void ui::iconMenu::render(SDL_Texture *target)
{
    // Update the color pulsing for the selection box
    menu::updateColorPulse(); 

    for(unsigned int i = 0; i < m_MenuOptions.size(); i++)
    {
        //This is special and hardcoded mostly
        if(m_Selected == static_cast<int>(i))
        {
            ui::renderSelectionBox(target, m_X - 8, (m_Y + (i * m_RectHeight)) - 8, BOUNDING_WIDTH, BOUNDING_HEIGHT, m_ColorMod);
        }
        //iconMenu is hardcoded for 128x128 icons
        graphics::textureRenderStretched(m_MenuOptions[i], target, m_X, m_Y + (i * m_RectHeight), ICON_WIDTH, ICON_HEIGHT);
    }
}