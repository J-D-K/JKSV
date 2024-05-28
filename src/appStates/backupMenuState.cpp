#include <memory>
#include <filesystem>
#include <cstring>
#include "ui/ui.hpp"
#include "config.hpp"
#include "system/input.hpp"
#include "graphics/graphics.hpp"
#include "appStates/backupMenuState.hpp"
#include "appStates/confirmState.hpp"
#include "appStates/taskState.hpp"
#include "filesystem/filesystem.hpp"
#include "system/task.hpp"
#include "stringUtil.hpp"
#include "jksv.hpp"
#include "log.hpp"

// This is for backup task
struct backupArgs : sys::taskArgs
{
    data::user *currentUser;
    data::titleInfo *currentTitleInfo;
    std::string outputBasePath;
    // This is to be able to reload the list after a new backup is created
    backupMenuState *sendingState;
};

// This is shared by overwrite and restore
struct pathArgs : sys::taskArgs
{
    std::string source, destination;
    // For reloading folder list
    backupMenuState *sendingState = NULL;
};

// These are the functions used for tasks
// Creates a new backup when new is selected. args must be a backupArgs struct as a shared pointer
void createNewBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    // Make sure we weren't passed nullptr
    if (args == nullptr)
    {
        task->finished();
        return;
    }

    // Set task status, cast args to backupArgs
    task->setThreadStatus("REPLACE THIS LATER");
    std::shared_ptr<backupArgs> argsIn = std::static_pointer_cast<backupArgs>(args);

    // Process shortcuts or get name for backup
    std::string backupName;
    if (sys::input::buttonHeld(HidNpadButton_R) || config::getByKey(CONFIG_AUTONAME_BACKUPS))
    {
        backupName = argsIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YMD);
    }
    else if (sys::input::buttonHeld(HidNpadButton_L))
    {
        backupName = argsIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YDM);
    }
    else if (sys::input::buttonHeld(HidNpadButton_ZL))
    {
        backupName = argsIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_ASC);
    }
    else
    {
        std::string defaultText = argsIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YMD);
        backupName = sys::input::getString(defaultText, "REPLACE THIS LATER", 64);
        backupName = stringUtil::getPathSafeString(backupName);
    }

    // If it's not empty, check ZIP option or extension. Normally I avoid nested ifs when possible but...
    if (backupName.empty() == false)
    {
        // Get extension and full path
        std::string extension = stringUtil::getExtensionFromString(backupName);
        std::string path = argsIn->outputBasePath + backupName + "/";

        if (config::getByKey(CONFIG_USE_ZIP) || extension == "zip")
        {
            zipFile zipOut = zipOpen64(path.c_str(), 0);
            fs::io::copyDirectoryToZip(fs::DEFAULT_SAVE_MOUNT_DEVICE, zipOut);
            zipClose(zipOut, "");
        }
        else
        {
            std::filesystem::create_directories(path);
            fs::io::copyDirectory(fs::DEFAULT_SAVE_MOUNT_DEVICE, path);
        }
    }
    {
        logger::log("Backup name empty.");
    }
    // Reload list to reflect changes
    argsIn->sendingState->loadDirectoryList();
    // Task is finished
    task->finished();
}

// Overwrites a backup already on SD
void overwriteBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    // Bail if no args present
    if (args == nullptr)
    {
        task->finished();
        return;
    }

    // Set status, cast args to type
    task->setThreadStatus("REPLACE THIS LATER");
    std::shared_ptr<pathArgs> argIn = std::static_pointer_cast<pathArgs>(args);

    // Get a test listing
    fs::directoryListing testListing(fs::DEFAULT_SAVE_MOUNT_DEVICE);

    // So JKSV doesn't create empty folders
    int saveFileCount = testListing.getListingCount();

    // For checking if overwriting a zip
    std::string fileExtension = stringUtil::getExtensionFromString(argIn->destination);

    if (std::filesystem::is_directory(argIn->destination) && saveFileCount > 0)
    {
        // Delete backup then recreate directory
        std::filesystem::remove_all(argIn->destination);
        std::filesystem::create_directories(argIn->destination);
        fs::io::copyDirectory(fs::DEFAULT_SAVE_MOUNT_DEVICE, argIn->destination);
    }
    else if (std::filesystem::is_directory(argIn->destination) == false && fileExtension == "zip" && saveFileCount > 0)
    {
        std::filesystem::remove(argIn->destination);
        zipFile zipOut = zipOpen64(argIn->destination.c_str(), 0);
        fs::io::copyDirectoryToZip(fs::DEFAULT_SAVE_MOUNT_DEVICE, zipOut);
        zipClose(zipOut, "");
    }
    task->finished();
}

// Wipes current save for game, then copies backup from SD to filesystem
void restoreBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    if (args == nullptr)
    {
        task->finished();
        return;
    }

    task->setThreadStatus("REPLACE THIS LATER");
    std::shared_ptr<pathArgs> argIn = std::static_pointer_cast<pathArgs>(args);

    // Test if there are actually files in the save
    fs::directoryListing testListing(argIn->source);
    int saveFileCount = testListing.getListingCount();
    if (saveFileCount <= 0)
    {
        return;
    }
}

void deleteBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    if(args == nullptr)
    {
        task->finished();
        return;
    }

    // Set thread status
    task->setThreadStatus(ui::strings::getString(LANG_THREAD_DELETE_FILE, 0));

    // Cast args
    std::shared_ptr<pathArgs> argsIn = std::static_pointer_cast<pathArgs>(args);

    if(std::filesystem::is_directory(argsIn->destination))
    {
        std::filesystem::remove_all(argsIn->destination);
    }
    else
    {
        std::filesystem::remove(argsIn->destination);
    }

    argsIn->sendingState->loadDirectoryList();

    task->finished();
}

backupMenuState::backupMenuState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo) : m_CurrentUser(currentUser), m_CurrentUserSaveInfo(currentUserSaveInfo), m_CurrentTitleInfo(currentTitleInfo)
{
    // This is the path used for all in/output. Create it if it doesn't exist.
    m_OutputBasePath = config::getWorkingDirectory() + m_CurrentTitleInfo->getPathSafeTitle() + "/";
    std::filesystem::create_directories(m_OutputBasePath);

    // Controls displayed at bottom
    m_BackupMenuControlGuide = ui::strings::getString(LANG_FOLDER_GUIDE, 0);
    m_PanelWidth = graphics::systemFont::getTextWidth(m_BackupMenuControlGuide, 18);

    // The actual panel
    m_BackupPanel = std::make_unique<ui::slidePanel>("backupMenuPanel", m_PanelWidth + 64, ui::slidePanelSide::PANEL_SIDE_RIGHT);
    // The menu of backups
    m_BackupMenu = std::make_unique<ui::menu>(10, 4, m_PanelWidth + 44, 18, 6);
    // Render target that menu is rendered to
    m_BackupMenuRenderTarget = graphics::textureCreate("backupMenuRenderTarget", m_PanelWidth + 64, 647, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
    // Directory listing of backups
    m_BackupListing = std::make_unique<fs::directoryListing>(m_OutputBasePath);
    // Directory listing of save
    m_SaveListing = std::make_unique<fs::directoryListing>(fs::DEFAULT_SAVE_MOUNT_DEVICE);

    backupMenuState::loadDirectoryList();
}

backupMenuState::~backupMenuState()
{
    fs::unmountSaveData();
}

void backupMenuState::update(void)
{
    // Update panel and menu
    m_BackupPanel->update();
    m_BackupMenu->update();

    // Get selected item to check what's selected
    int selected = m_BackupMenu->getSelected();
    if (sys::input::buttonDown(HidNpadButton_A) && selected == 0)
    {
        // Check to make sure save data isn't empty before wasting time and space
        if (m_SaveListing->getListingCount() > 0)
        {
            // Data to send to task
            std::shared_ptr<backupArgs> backupTaskArgs = std::make_shared<backupArgs>();
            backupTaskArgs->currentUser = m_CurrentUser;
            backupTaskArgs->currentTitleInfo = m_CurrentTitleInfo;
            backupTaskArgs->outputBasePath = m_OutputBasePath;
            backupTaskArgs->sendingState = this;

            // Task state
            std::unique_ptr<appState> backupTask = std::make_unique<taskState>(createNewBackup, backupTaskArgs);
            jksv::pushNewState(backupTask);
        }
        else
        {
            ui::popMessage::newMessage(ui::strings::getString(LANG_POP_SAVE_EMPTY, 0), ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
        }
    }
    else if(sys::input::buttonDown(HidNpadButton_A) && selected > 0)
    {
        // Selected item
        std::string selectedItem = m_BackupListing->getItemAt(selected - 1);
        // Get overwrite string
        std::string confirmOverwrite = stringUtil::getFormattedString(ui::strings::getCString(LANG_CONFIRM_OVERWRITE, 0), selectedItem.c_str());

        // Arg pointer
        std::shared_ptr<pathArgs> paths = std::make_shared<pathArgs>();
        paths->destination = m_BackupListing->getFullPathToItemAt(selected - 1);

        // Create confirmation
        std::unique_ptr<appState> overwriteConfirmationState = std::make_unique<confirmState>(confirmOverwrite, overwriteBackup, paths);
        // Push it real good
        jksv::pushNewState(overwriteConfirmationState);
    }
    else if (sys::input::buttonDown(HidNpadButton_Y) && selected > 0)
    {
        // Get selected menu item
        std::string selectedItem = m_BackupListing->getItemAt(selected - 1);
        // Get restoration string
        std::string confirmRestore = stringUtil::getFormattedString(ui::strings::getCString(LANG_CONFIRM_RESTORE, 0), selectedItem.c_str());

        // Setup confirmation arg pointer
        std::shared_ptr<pathArgs> paths = std::make_shared<pathArgs>();
        paths->source = m_BackupListing->getFullPathToItemAt(selected - 1); // Need to subtract one to account for 'New' option

        // Create confirmation
        std::unique_ptr<appState> restoreConfirmationState = std::make_unique<confirmState>(confirmRestore, restoreBackup, paths);
        // Push it to vector
        jksv::pushNewState(restoreConfirmationState);
    }
    else if(sys::input::buttonDown(HidNpadButton_X) && selected > 0)
    {
        // Get selected
        std::string selectedItem = m_BackupListing->getItemAt(selected - 1);
        // Get confirmation string
        std::string confirmDelete = stringUtil::getFormattedString(ui::strings::getCString(LANG_CONFIRM_DELETE, 0), selectedItem.c_str());

        // Setup args
        std::shared_ptr<pathArgs> paths = std::make_shared<pathArgs>();
        paths->destination = m_BackupListing->getFullPathToItemAt(selected - 1);
        paths->sendingState = this;

        // Create confirm and push
        std::unique_ptr<appState> deleteConfirmationState = std::make_unique<confirmState>(confirmDelete, deleteBackup, paths);
        jksv::pushNewState(deleteConfirmationState);
    }
    else if (sys::input::buttonDown(HidNpadButton_B))
    {
        m_BackupPanel->closePanel();
    }
    else if (m_BackupPanel->isClosed())
    {
        appState::deactivateState();
    }
}

void backupMenuState::render(void)
{
    // Get panel's render target and clear it first
    SDL_Texture *panelTarget = m_BackupPanel->getPanelRenderTarget();
    graphics::textureClear(panelTarget, COLOR_SLIDE_PANEL_TARGET);

    // Clear menu render target
    graphics::textureClear(m_BackupMenuRenderTarget, COLOR_TRANSPARENT);

    // Render menu to menu render target
    m_BackupMenu->render(m_BackupMenuRenderTarget);

    // Render menu target to panel target (this is so the menu can only be draw so far and not cover guider)
    graphics::textureRender(m_BackupMenuRenderTarget, panelTarget, 0, 0);

    // Draw divider and guide on bottom
    graphics::renderLine(panelTarget, 18, 648, m_PanelWidth + 54, 648, COLOR_WHITE);
    graphics::systemFont::renderText(m_BackupMenuControlGuide, panelTarget, 32, 673, 18, COLOR_WHITE);

    // Render panel to framebuffer
    m_BackupPanel->render();
}

void backupMenuState::loadDirectoryList(void)
{
    // Reload list, clear menu
    m_BackupListing->loadListing();
    m_BackupMenu->clearMenu();

    int listingCount = m_BackupListing->getListingCount();
    m_BackupMenu->addOpt(ui::strings::getString(LANG_FOLDER_MENU_NEW, 0));
    for (int i = 0; i < listingCount; i++)
    {
        m_BackupMenu->addOpt(m_BackupListing->getItemAt(i));
    }
}
