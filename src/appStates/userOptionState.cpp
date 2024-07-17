#include <string_view>
#include "appStates/userOptionState.hpp"

namespace
{
    // Slide panel name
    const std::string SLIDE_PANEL_NAME = "userOptionPanel";
    // Slide dimensions
    const int SLIDE_PANEL_WIDTH = 410;
    // Menu coordinates and dimensions
    const int OPTION_MENU_X = 8;
    const int OPTION_MENU_Y = 32;
    const int OPTION_MENU_BOUNDING_WIDTH = 390;
    const int OPTION_MENU_FONT_SIZE = 20;
    const int OPTION_MENU_MAX_SCROLL = 6;
    // This is how many options are in the map for this menu
    const int OPTION_MENU_COUNT = 4;
    // UI Strings this state uses
    const std::string OPTION_MENU_STRING_NAME = "userOptions";
}

userOptionState::userOptionState(data::user *currentUser) :
m_CurrentUser(currentUser),
m_SlidePanel(std::make_unique<ui::slidePanel>(SLIDE_PANEL_NAME, SLIDE_PANEL_WIDTH, ui::slidePanelSide::PANEL_SIDE_RIGHT)),
m_OptionsMenu(std::make_unique<ui::menu>(OPTION_MENU_X, OPTION_MENU_Y, OPTION_MENU_BOUNDING_WIDTH, OPTION_MENU_FONT_SIZE, OPTION_MENU_MAX_SCROLL))
{
    // Add options to menu
    for(int i = 0; i < OPTION_MENU_COUNT; i++)
    {
        m_OptionsMenu->addOption(ui::strings::getString(OPTION_MENU_STRING_NAME, i));
    }
}

userOptionState::~userOptionState() { }

void userOptionState::update(void)
{

}

void userOptionState::render(void)
{
    
}