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

// Names of panels, render targets, and coordinates + dimensions of menu
static const char *BACKUP_PANEL_NAME = "backupMenuPanel";
static const char *BACKUP_MENU_TARGET_NAME = "backupMenuRenderTarget";
static const int BACKUP_MENU_X = 10;
static const int BACKUP_MENU_Y = 4;
static const int BACKUP_MENU_FONT_SIZE = 18;
static const int BACKUP_MENU_SCROLL_LENGTH = 6;

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
// To do: Find a better name since this holds more than paths now
struct pathArgs : sys::taskArgs
{
    std::string source, destination;
    // Journal size for commits
    uint64_t journalSize = 0;
    // For reloading folder list
    backupMenuState *sendingState = NULL;
};

// These are the functions used for tasks
// Creates a new backup when new is selected. args must be a backupArgs struct as a shared pointer
static void createNewBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
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
            fs::zip::copyDirectoryToZip(fs::DEFAULT_SAVE_MOUNT_DEVICE, zipOut);
            zipClose(zipOut, "");
        }
        else
        {
            std::filesystem::create_directories(path);
            fs::io::copyDirectory(fs::DEFAULT_SAVE_MOUNT_DEVICE, path);
        }
    }

    // Reload list to reflect changes
    argsIn->sendingState->loadDirectoryList();
    // Task is finished
    task->finished();
}

// Overwrites a backup already on SD
static void overwriteBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    // Bail if no args present
    if (args == nullptr)
    {
        task->finished();
        return;
    }

    // Error code for filesystem
    std::error_code errorCode;

    // Set status, cast args to type
    task->setThreadStatus("REPLACE THIS LATER");
    std::shared_ptr<pathArgs> argsIn = std::static_pointer_cast<pathArgs>(args);

    // For checking if overwriting a zip
    std::string fileExtension = stringUtil::getExtensionFromString(argsIn->destination);

    if (std::filesystem::is_directory(argsIn->destination))
    {
        // Delete backup then recreate directory
        // Add trailing slash first.
        std::string target = argsIn->destination + "/";
        // Delete old backup
        std::filesystem::remove_all(target, errorCode);
        // Recreate whole path.
        std::filesystem::create_directories(argsIn->destination);
        // Create the new backup in the same folder
        fs::io::copyDirectory(fs::DEFAULT_SAVE_MOUNT_DEVICE, target);
    }
    else if (std::filesystem::is_directory(argsIn->destination) == false && fileExtension == "zip")
    {
        // Just delete zip file
        std::filesystem::remove(argsIn->destination);
        // Create a new one with the same name
        zipFile zipOut = zipOpen64(argsIn->destination.c_str(), 0);
        // Create new zip with old name
        fs::zip::copyDirectoryToZip(fs::DEFAULT_SAVE_MOUNT_DEVICE, zipOut);
        zipClose(zipOut, "");
    }
    task->finished();
}

// Wipes current save for game, then copies backup from SD to filesystem
// Note: Needs to be tested better some time
static void restoreBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    if (args == nullptr)
    {
        task->finished();
        return;
    }

    // Error code for later
    std::error_code errorCode;

    // Set status, cast args
    task->setThreadStatus("REPLACE THIS LATER");
    std::shared_ptr<pathArgs> argsIn = std::static_pointer_cast<pathArgs>(args);

    // Get extension before we try to unzip something that isn't a zip
    std::string fileExtension = stringUtil::getExtensionFromString(argsIn->source);

    if(std::filesystem::is_directory(argsIn->source))
    {
        // Add trailing slash
        std::string source = argsIn->source + "/";
        // Get a test listing first to prevent writing an empty folder to save
        fs::directoryListing testListing(source);

        if(testListing.getListingCount() > 0)
        {
            // Folder isn't empty, erase
            std::filesystem::remove_all(fs::DEFAULT_SAVE_MOUNT_DEVICE, errorCode);
            // Copy source to save container
            fs::io::copyDirectoryCommit(source, fs::DEFAULT_SAVE_MOUNT_DEVICE, argsIn->journalSize);
        }
    }
    else if(std::filesystem::is_directory(argsIn->source) == false && fileExtension == "zip")
    {
        // Open zip for decompressing
        unzFile unzip = unzOpen64(argsIn->source.c_str());
        // Check if opening was successful and not empty first
        if(unzip != NULL && fs::zip::zipFileIsNotEmpty(unzip))
        {
            fs::zip::copyZipToDirectory(unzip, fs::DEFAULT_SAVE_MOUNT_DEVICE, argsIn->journalSize);
        }
        // Close zip
        unzClose(unzip);
    }

    task->finished();
}

static void deleteBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    if (args == nullptr)
    {
        task->finished();
        return;
    }

    // Error code for filesystem later
    std::error_code errorCode;

    // Set thread status
    task->setThreadStatus(ui::strings::getString(LANG_THREAD_DELETE_FILE, 0));

    // Cast args
    std::shared_ptr<pathArgs> argsIn = std::static_pointer_cast<pathArgs>(args);

    if (std::filesystem::is_directory(argsIn->destination))
    {
        logger::log("%s is directory.", argsIn->destination.c_str());
        std::uintmax_t deleted = std::filesystem::remove_all(argsIn->destination, errorCode);
        logger::log("Error code: %s, %i - %u", errorCode.message().c_str(), errorCode.value(), deleted);
    }
    else
    {
        logger::log("%s is not a directory", argsIn->destination.c_str());
        // Just remove it since it's probably a zip file
        std::filesystem::remove(argsIn->destination, errorCode);
    }
    // Refresh to reflect changes
    argsIn->sendingState->loadDirectoryList();

    task->finished();
}

backupMenuState::backupMenuState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo) :
m_CurrentUser(currentUser), 
m_CurrentUserSaveInfo(currentUserSaveInfo), 
m_CurrentTitleInfo(currentTitleInfo),
m_BackupMenuControlGuide(ui::strings::getString(LANG_FOLDER_GUIDE, 0)),
m_OutputBasePath(config::getWorkingDirectory() + m_CurrentTitleInfo->getPathSafeTitle() + "/"),
m_PanelWidth(graphics::systemFont::getTextWidth(m_BackupMenuControlGuide, 18)),
m_BackupPanel(std::make_unique<ui::slidePanel>(BACKUP_PANEL_NAME, m_PanelWidth + 64, ui::slidePanelSide::PANEL_SIDE_RIGHT)),
m_BackupListing(std::make_unique<fs::directoryListing>(m_OutputBasePath)),
m_SaveListing(std::make_unique<fs::directoryListing>(fs::DEFAULT_SAVE_MOUNT_DEVICE)),
m_BackupMenu(std::make_unique<ui::menu>(BACKUP_MENU_X, BACKUP_MENU_Y, m_PanelWidth + 44, BACKUP_MENU_FONT_SIZE, BACKUP_MENU_SCROLL_LENGTH))
{
    // Make sure path exists
    std::filesystem::create_directories(m_OutputBasePath);
    // Render target that menu is rendered to
    m_BackupMenuRenderTarget = graphics::textureCreate(BACKUP_MENU_TARGET_NAME, m_PanelWidth + 64, 647, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
    // Call this just to be sure.
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
    else if (sys::input::buttonDown(HidNpadButton_A) && selected > 0 && m_SaveListing->getListingCount() > 0)
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
        paths->journalSize = m_CurrentTitleInfo->getJournalSize(static_cast<FsSaveDataType>(m_CurrentUserSaveInfo->getSaveDataInfo().save_data_type)); // Needed to prevent commit errors

        // Create confirmation
        std::unique_ptr<appState> restoreConfirmationState = std::make_unique<confirmState>(confirmRestore, restoreBackup, paths);
        // Push it to vector
        jksv::pushNewState(restoreConfirmationState);
    }
    else if (sys::input::buttonDown(HidNpadButton_X) && selected > 0)
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
    m_BackupMenu->setSelected(0);

    int listingCount = m_BackupListing->getListingCount();
    m_BackupMenu->addOpt(ui::strings::getString(LANG_FOLDER_MENU_NEW, 0));
    for (int i = 0; i < listingCount; i++)
    {
        m_BackupMenu->addOpt(m_BackupListing->getItemAt(i));
    }
}
