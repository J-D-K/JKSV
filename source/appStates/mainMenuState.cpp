#include <chrono>
#include <string>
#include "jksv.hpp"

#include "appStates/mainMenuState.hpp"
#include "appStates/titleSelectionState.hpp"
#include "appStates/taskState.hpp"
#include "appStates/progressState.hpp"
#include "appStates/userOptionState.hpp"
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
    // This is the font size used to render 'Settings' and 'Extras' to their icons
    constexpr int EXTRA_OPTIONS_FONT_SIZE = 42;
    // Texture names
    const std::string MAIN_MENU_RENDER_TARGET = "mainMenuRenderTarget";
    const std::string MAIN_MENU_SETTINGS = "mainMenuSettings";
    const std::string MAIN_MENU_EXTRAS = "mainMenuExtras";
    // Path to texture needed
    const std::string MAIN_MENU_BACKGROUND_TEXTURE_PATH = "romfs:/img/backgroundDark.png";
    // Strings used here
    const std::string MAIN_MENU_CONTROL_GUIDE = "helpUser";
    const std::string MAIN_MENU_SETTINGS_STRING = "mainMenuSettings";
    const std::string MAIN_MENU_EXTRAS_STRING = "mainMenuExtras";
    const std::string POP_NO_SAVES_FOUND = "saveDataNoneFound";
}

// Tasks
static void backupAllUserSaves(sys::task *task, sys::sharedTaskData sharedData)
{
    // Cast to progress
    sys::progressTask *progress = reinterpret_cast<sys::progressTask *>(task);

    // Loop through users
    int totalUserCount = data::getTotalUsers();
    for (int i = 0; i < totalUserCount; i++)
    {
        // Get pointer to user
        data::user *currentUser = data::getUserAtPosition(i);

        // Loop through titles
        int totalUserGameCount = currentUser->getTotalUserSaveInfo();
        for (int j = 0; j < totalUserGameCount; j++)
        {
            // Get pointer to userSaveInfo
            data::userSaveInfo *currentUserSaveInfo = currentUser->getUserSaveInfoAt(j);

            // Get pointer to title data for path later
            data::titleInfo *currentTitleInfo = data::getTitleInfoByTitleID(currentUserSaveInfo->getTitleID());

            // Try to mount save
            bool saveIsMounted = fs::mountSaveData(currentUserSaveInfo->getSaveDataInfo());
            // Make sure full path for title exists
            fs::createTitleDirectoryByTID(currentUserSaveInfo->getTitleID());
            if (saveIsMounted && fs::directoryContainsFiles(fs::DEFAULT_SAVE_MOUNT_DEVICE) && config::getByKey(CONFIG_USE_ZIP))
            {
                // Path to zip we're backing up. It's a biggin'
                std::string outputZipPath = config::getWorkingDirectory() + currentTitleInfo->getPathSafeTitle() + "/" + currentUser->getPathSafeUsername() +
                                            " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YMD) + ".zip";
                // Open zip
                zipFile newZip = zipOpen64(outputZipPath.c_str(), 0);
                // Copy to it
                fs::zip::copyDirectoryToZip(fs::DEFAULT_SAVE_MOUNT_DEVICE, newZip, progress);
            }
            else if (saveIsMounted && fs::directoryContainsFiles(fs::DEFAULT_SAVE_MOUNT_DEVICE) && config::getByKey(CONFIG_USE_ZIP) == false)
            {
                // Output path
                std::string newSaveBackupPath = config::getWorkingDirectory() + currentTitleInfo->getPathSafeTitle() + "/" + currentUser->getPathSafeUsername() +
                                                " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YMD) + "/";
                // Make sure it exists
                std::filesystem::create_directories(newSaveBackupPath);
                // Backup
                fs::io::copyDirectory(fs::DEFAULT_SAVE_MOUNT_DEVICE, newSaveBackupPath, progress);
            }
            fs::unmountSaveData();
        }
    }
    progress->finished();
}

mainMenuState::mainMenuState(void) : m_MainControlGuide(ui::strings::getString(MAIN_MENU_CONTROL_GUIDE, 0)),
                                     m_MainControlGuideX(1220 - graphics::systemFont::getTextWidth(m_MainControlGuide, 18)),
                                     m_MainMenu(std::make_unique<ui::iconMenu>(50, 16, 1))
{
    // Render target
    m_RenderTarget = graphics::textureManager::createTexture(MAIN_MENU_RENDER_TARGET, 200, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

    // Load gradient for behind menu
    m_MenuBackgroundTexture = graphics::textureManager::loadTextureFromFile(MAIN_MENU_BACKGROUND_TEXTURE_PATH);

    // Setup menu
    m_UserEnd = data::getTotalUsers();
    for (int i = 0; i < m_UserEnd; i++)
    {
        data::user *currentUser = data::getUserAtPosition(i);
        m_MainMenu->addOpt(currentUser->getUserIcon());
    }

    // Settings & extras
    std::string settingsString = ui::strings::getString(MAIN_MENU_SETTINGS_STRING, 0);
    std::string extrasString = ui::strings::getString(MAIN_MENU_EXTRAS_STRING, 0);
    m_MainMenu->addOpt(graphics::createIcon(MAIN_MENU_SETTINGS, settingsString, EXTRA_OPTIONS_FONT_SIZE));
    m_MainMenu->addOpt(graphics::createIcon(MAIN_MENU_EXTRAS, extrasString, EXTRA_OPTIONS_FONT_SIZE));
}

mainMenuState::~mainMenuState() {}

void mainMenuState::update(void)
{
    m_MainMenu->update();

    int selected = m_MainMenu->getSelected();
    if (sys::input::buttonDown(HidNpadButton_A) && selected < m_UserEnd)
    {
        data::user *selectedUser = data::getUserAtPosition(selected);
        // Normally wouldn't do this
        if (selectedUser->getTotalUserSaveInfo() > 0)
        {
            sharedAppState newTitleSelection = createTitleSelectionState(selectedUser);
            jksv::pushState(newTitleSelection);
        }
        else
        {
            std::string noSavesMessage = stringUtil::getFormattedString(ui::strings::getCString(POP_NO_SAVES_FOUND, 0), selectedUser->getUsername().c_str());
            ui::popMessage::newMessage(noSavesMessage, ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
        }
    }
    else if (sys::input::buttonDown(HidNpadButton_Y))
    {
        // This doesn't need any input
        sharedAppState backupAllUserSaveState = createNewProgressTask(backupAllUserSaves, nullptr);
        jksv::pushState(backupAllUserSaveState);
    }
    else if (sys::input::buttonDown(HidNpadButton_X) && selected < m_UserEnd)
    {
        // Get selected user
        data::user *selectedUser = data::getUserAtPosition(selected);
        // Create new option state
        sharedAppState userOptionState = createUserOptionState(selectedUser);
        jksv::pushState(userOptionState);
    }
}

void mainMenuState::render(void)
{
    graphics::textureClear(m_RenderTarget.get(), COLOR_DEFAULT_CLEAR);
    graphics::textureRender(m_MenuBackgroundTexture.get(), m_RenderTarget.get(), 0, 0);
    m_MainMenu->render(m_RenderTarget.get());

    // Render render target to framebuffer
    graphics::textureRender(m_RenderTarget.get(), NULL, 0, 91);

    if (appState::hasFocus())
    {
        graphics::systemFont::renderText(m_MainControlGuide, NULL, m_MainControlGuideX, 673, 18, COLOR_WHITE);
    }
}
