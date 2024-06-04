#include <chrono>
#include "jksv.hpp"
#include "appStates/mainMenuState.hpp"
#include "appStates/titleSelectionState.hpp"
#include "appStates/taskState.hpp"
#include "graphics/graphics.hpp"
#include "data/data.hpp"
#include "system/input.hpp"
#include "ui/ui.hpp"
#include "system/task.hpp"
#include "stringUtil.hpp"
#include "log.hpp"

// Main menu coordinates and dimensions
static const int MAIN_MENU_X = 50;
static const int MAIN_MENU_Y = 16;
static const int MAIN_MENU_SCROLL_LENGTH = 1;

// Texture names
static const char *MAIN_MENU_RENDER_TARGET = "mainMenuRenderTarget";
static const char *MAIN_MENU_SETTINGS = "mainMenuSettings";
static const char *MAIN_MENU_EXTRAS = "mainMenuExtras";

// Tasks
static void backupAllUserSaves(void *in)
{
    
}

mainMenuState::mainMenuState(void) :
m_MainControlGuide(ui::strings::getString(LANG_USER_GUIDE, 0)),
m_MainControlGuideX(1220 - graphics::systemFont::getTextWidth(m_MainControlGuide, 18)),
m_MainMenu(std::make_unique<ui::iconMenu>(MAIN_MENU_X, MAIN_MENU_Y, MAIN_MENU_SCROLL_LENGTH))
{
    // Render target
    m_RenderTarget = graphics::textureCreate(MAIN_MENU_RENDER_TARGET, 200, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

    // Load gradient for behind menu
    m_MenuBackgroundTexture = graphics::textureLoadFromFile(TEXTURE_MENU_BACKGROUND, "romfs:/img/menu/backgroundDark.png");

    // Setup menu
    m_UserEnd = data::getTotalUsers();
    for (int i = 0; i < m_UserEnd; i++)
    {
        data::user *currentUser = data::getUserAtPosition(i);
        m_MainMenu->addOpt(currentUser->getUserIcon());
    }

    // Settings & extras
    std::string settingsString = ui::strings::getString(LANG_MAIN_MENU_SETTINGS, 0);
    std::string extrasString = ui::strings::getString(LANG_MAIN_MENU_EXTRAS, 0);
    m_MainMenu->addOpt(graphics::createIcon(MAIN_MENU_SETTINGS, settingsString, 42));
    m_MainMenu->addOpt(graphics::createIcon(MAIN_MENU_EXTRAS, extrasString, 42));
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
            std::string noSavesMessage = stringUtil::getFormattedString(ui::strings::getCString(LANG_SAVEDATA_NONE_FOUND, 0), selectedUser->getUsername().c_str());
            ui::popMessage::newMessage(noSavesMessage, ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
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