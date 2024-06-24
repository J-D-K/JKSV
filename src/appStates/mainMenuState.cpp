#include <chrono>
#include "jksv.hpp"
#include "appStates/mainMenuState.hpp"
#include "appStates/titleSelectionState.hpp"
#include "appStates/taskState.hpp"
#include "filesystem/filesystem.hpp"
#include "graphics/graphics.hpp"
#include "system/system.hpp"
#include "data/data.hpp"
#include "ui/ui.hpp"
#include "config.hpp"
#include "stringUtil.hpp"
#include "log.hpp"

namespace
{
    // Main menu coordinates and dimensions
    const int MAIN_MENU_X = 50;
    const int MAIN_MENU_Y = 16;
    const int MAIN_MENU_SCROLL_LENGTH = 1;
    // Render target
    const int RENDER_TARGET_WIDTH = 200;
    const int RENDER_TARGET_HEIGHT = 555;
    // This is the font size used to render 'Settings' and 'Extras' to their icons
    const int EXTRA_OPTIONS_FONT_SIZE = 42;
    // Coordinates and font size of control guide
    const int CONTROL_GUIDE_Y_POSITION = 673;
    const int CONTROL_GUIDE_FONT_SIZE = 18;

    // Texture names
    const char *MAIN_MENU_RENDER_TARGET = "mainMenuRenderTarget";
    const char *MAIN_MENU_SETTINGS = "mainMenuSettings";
    const char *MAIN_MENU_EXTRAS = "mainMenuExtras";
    const char *MAIN_MENU_TEXTURE_NAME = "mainMenuBackgroundTexture";

    // Path to texture needed
    const char *MAIN_MENU_BACKGROUND_TEXTURE_PATH = "romfs:/img/menu/backgroundDark.png";
}

// Tasks
static void backupAllUserSaves(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    // Cast to progress
    sys::progressTask *progress = reinterpret_cast<sys::progressTask *>(task);

    // Loop through users
    int totalUserCount = data::getTotalUsers();
    for(int i = 0; i < totalUserCount; i++)
    {
        // Get pointer to user
        data::user *currentUser = data::getUserAtPosition(i);

        // Loop through titles
        int totalUserGameCount = currentUser->getTotalUserSaveInfo();
        for(int j = 0; j < totalUserGameCount; j++)
        {
            // Get pointer to userSaveInfo
            data::userSaveInfo *currentUserSaveInfo = currentUser->getUserSaveInfoAt(i);

            // Try to mount save
            bool saveIsMounted = fs::mountSaveData(currentUserSaveInfo->getSaveDataInfo());
            // Make sure full path for title exists
            fs::createTitleDirectoryByTID(currentUserSaveInfo->getTitleID());
            if(saveIsMounted && fs::directoryContainsFiles(fs::DEFAULT_SAVE_MOUNT_DEVICE) && config::getByKey(CONFIG_USE_ZIP))
            {

            }
        }
    }
}

mainMenuState::mainMenuState(void) :
m_MainControlGuide(ui::strings::getString(LANG_USER_GUIDE, 0)),
m_MainControlGuideX(1220 - graphics::systemFont::getTextWidth(m_MainControlGuide, CONTROL_GUIDE_FONT_SIZE)),
m_MainMenu(std::make_unique<ui::iconMenu>(MAIN_MENU_X, MAIN_MENU_Y, MAIN_MENU_SCROLL_LENGTH))
{
    // Render target
    m_RenderTarget = graphics::textureCreate(MAIN_MENU_RENDER_TARGET, RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

    // Load gradient for behind menu
    m_MenuBackgroundTexture = graphics::textureLoadFromFile(MAIN_MENU_TEXTURE_NAME, MAIN_MENU_BACKGROUND_TEXTURE_PATH);

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
    m_MainMenu->addOpt(graphics::createIcon(MAIN_MENU_SETTINGS, settingsString, EXTRA_OPTIONS_FONT_SIZE));
    m_MainMenu->addOpt(graphics::createIcon(MAIN_MENU_EXTRAS, extrasString, EXTRA_OPTIONS_FONT_SIZE));
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
        graphics::systemFont::renderText(m_MainControlGuide, NULL, m_MainControlGuideX, CONTROL_GUIDE_Y_POSITION, CONTROL_GUIDE_FONT_SIZE, COLOR_WHITE);
    }
}