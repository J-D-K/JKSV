#include "jksv.hpp"
#include "appStates/mainMenuState.hpp"
#include "appStates/titleSelectionState.hpp"
#include "appStates/taskState.hpp"
#include "graphics/graphics.hpp"
#include "data/data.hpp"
#include "system/input.hpp"
#include "ui/ui.hpp"
#include "system/task.hpp"
#include "log.hpp"

// Tasks
static void backupAllUserSaves(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    
}

mainMenuState::mainMenuState(void)
{
    // Setup control guide
    m_MainControlGuide = ui::strings::getString(LANG_USER_GUIDE, 0);
    m_MainControlGuideX = 1220 - graphics::systemFont::getTextWidth(m_MainControlGuide, 18);

    // Render target
    m_RenderTarget = graphics::textureCreate("mainMenuRenderTarget", 200, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

    // Load gradient for behind menu
    m_MenuBackgroundTexture = graphics::textureLoadFromFile(TEXTURE_MENU_BACKGROUND, "romfs:/img/menu/backgroundDark.png");

    // Setup menu
    m_MainMenu = std::make_unique<ui::iconMenu>(50, 16, 1);
    m_UserEnd = data::getTotalUsers();
    for (int i = 0; i < m_UserEnd; i++)
    {
        data::user *currentUser = data::getUserAtPosition(i);
        m_MainMenu->addOpt(currentUser->getUserIcon());
    }

    // Settings & extras
    std::string settingsString = ui::strings::getString(LANG_MAIN_MENU_SETTINGS, 0);
    std::string extrasString = ui::strings::getString(LANG_MAIN_MENU_EXTRAS, 0);
    m_MainMenu->addOpt(graphics::createIcon("mainMenuSettings", settingsString, 42));
    m_MainMenu->addOpt(graphics::createIcon("mainMenuExtras", extrasString, 42));
}

mainMenuState::~mainMenuState() {}

void mainMenuState::update(void)
{
    m_MainMenu->update();

    int selected = m_MainMenu->getSelected();
    if(sys::input::buttonDown(HidNpadButton_A) && selected < m_UserEnd)
    {
        data::user *selectedUser = data::getUserAtPosition(selected);
        // Normally wouldn't do this
        if(selectedUser->getTotalUserSaveInfo() > 0)
        {
            std::unique_ptr<appState> titleSelection = std::make_unique<titleSelectionState>(selectedUser);
            jksv::pushNewState(titleSelection);
        }
        else
        {
            logger::log("No titles found for user %s", selectedUser->getUsername().c_str());
        }
    }
}

void mainMenuState::render(void)
{
    graphics::textureClear(m_RenderTarget, COLOR_DEFAULT_CLEAR);
    graphics::textureRender(m_MenuBackgroundTexture, m_RenderTarget, 0, 0);
    m_MainMenu->render(m_RenderTarget);

    // Render render target to framebuffer
    graphics::textureRender(m_RenderTarget, NULL, 0, 91);

    if(appState::hasFocus())
    {
        graphics::systemFont::renderText(m_MainControlGuide, NULL, m_MainControlGuideX, 673, 18, COLOR_WHITE);
    }
}