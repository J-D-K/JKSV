#include <memory>
#include <filesystem>
#include <cstring>

#include "appStates/backupMenuState.hpp"
#include "appStates/confirmState.hpp"
#include "appStates/taskState.hpp"
#include "appStates/progressState.hpp"
#include "ui/ui.hpp"
#include "system/input.hpp"
#include "graphics/graphics.hpp"
#include "filesystem/filesystem.hpp"
#include "system/task.hpp"
#include "config.hpp"
#include "jksv.hpp"
#include "log.hpp"
#include "stringUtil.hpp"

// Names of panels, render targets, and coordinates + dimensions of menu
namespace
{
    const std::string BACKUP_PANEL_NAME = "backupMenuPanel";
    const std::string BACKUP_MENU_TARGET_NAME = "backupMenuRenderTarget";
    // Names of strings used in this file
    const std::string FOLDER_MENU_CONTROL_GUIDE = "helpFolder";
    const std::string THREAD_STATUS_DELETING_FILE = "threadStatusDeletingFile";
    const std::string POP_MESSAGE_SAVE_EMPTY = "popSaveIsEmpty";
    const std::string CONFIRM_OVERWRITE = "confirmOverwrite";
    const std::string CONFIRM_RESTORE = "confirmRestore";
    const std::string CONFIRM_DELETE  = "confirmDelete";
    const std::string BACKUP_MENU_NEW = "folderMenuNew";
    const std::string SWKBD_PROMPT_TEXT = "swkbdEnterName";
}

// This is for backup task
struct backupData : sys::taskData
{
    data::user *currentUser;
    data::titleInfo *currentTitleInfo;
    std::string outputBasePath;
    // So we can force sending state to refresh
    backupMenuState *sendingState = NULL;
};

// This is shared by overwrite and restore
// To do: Find a better name since this holds more than paths now
struct pathData : sys::taskData
{
    std::string source, destination;
    // This is for commiting to device
    uint64_t journalSize = 0;
    // For refresh
    backupMenuState *sendingState = NULL;
};

// These are the functions used for tasks
// Creates a new backup when new is selected. args must be a backupArgs struct as a shared pointer
static void createNewBackup(sys::task *task, sys::sharedTaskData sharedData)
{
    // Make sure we weren't passed nullptr
    if (sharedData == nullptr)
    {
        task->finished();
        return;
    }

    // Set task status, cast args to backupArgs
    std::shared_ptr<backupData> dataIn = std::static_pointer_cast<backupData>(sharedData);
    
    // To do: Maybe find a better way to do this without rewriting everything
    sys::progressTask *progress = reinterpret_cast<sys::progressTask *>(task);

    // Process shortcuts or get name for backup
    std::string backupName;
    if (sys::input::buttonHeld(HidNpadButton_R) || config::getByKey(CONFIG_AUTONAME_BACKUPS))
    {
        backupName = dataIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YMD);
    }
    else if (sys::input::buttonHeld(HidNpadButton_L))
    {
        backupName = dataIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YDM);
    }
    else if (sys::input::buttonHeld(HidNpadButton_ZL))
    {
        backupName = dataIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_ASC);
    }
    else
    {
        std::string defaultText = dataIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YMD);
        std::string promptText = ui::strings::getString(SWKBD_PROMPT_TEXT, 0);
        backupName = sys::input::getString(SwkbdType_QWERTY, defaultText, promptText, 64);
        backupName = stringUtil::getPathSafeString(backupName);
    }

    // If it's not empty, check ZIP option or extension. Normally I avoid nested ifs when possible but...
    if (backupName.empty() == false)
    {
        // Get extension and full path
        std::string extension = stringUtil::getExtensionFromString(backupName);
        std::string path = dataIn->outputBasePath + backupName + "/";

        if (config::getByKey(CONFIG_USE_ZIP) || extension == "zip")
        {
            zipFile zipOut = zipOpen64(path.c_str(), 0);
            fs::zip::copyDirectoryToZip(fs::DEFAULT_SAVE_MOUNT_DEVICE, zipOut, progress);
            zipClose(zipOut, "");
        }
        else
        {
            std::filesystem::create_directories(path);
            fs::io::copyDirectory(fs::DEFAULT_SAVE_MOUNT_DEVICE, path, progress);
        }
    }

    // Reload list to reflect changes
    dataIn->sendingState->loadDirectoryList();
    // Task is finished
    task->finished();
}

// Overwrites a backup already on SD
static void overwriteBackup(sys::task *task, sys::sharedTaskData sharedData)
{
    // Bail if no args present
    if (sharedData == nullptr)
    {
        task->finished();
        return;
    }

    // Error code for filesystem
    std::error_code errorCode;

    // Set status, cast args to type
    std::shared_ptr<pathData> dataIn = std::static_pointer_cast<pathData>(sharedData);
    sys::progressTask *progress = reinterpret_cast<sys::progressTask *>(task);

    // For checking if overwriting a zip
    std::string fileExtension = stringUtil::getExtensionFromString(dataIn->destination);

    if (std::filesystem::is_directory(dataIn->destination))
    {
        // Delete backup then recreate directory
        // Add trailing slash first.
        std::string target = dataIn->destination + "/";
        // Delete old backup
        fs::io::deleteDirectoryRecursively(target);
        // Recreate whole path.
        std::filesystem::create_directories(dataIn->destination);
        // Create the new backup in the same folder
        fs::io::copyDirectory(fs::DEFAULT_SAVE_MOUNT_DEVICE, target, progress);
    }
    else if (std::filesystem::is_directory(dataIn->destination) == false && fileExtension == "zip")
    {
        // Just delete zip file
        std::filesystem::remove(dataIn->destination);
        // Create a new one with the same name
        zipFile zipOut = zipOpen64(dataIn->destination.c_str(), 0);
        // Create new zip with old name
        fs::zip::copyDirectoryToZip(fs::DEFAULT_SAVE_MOUNT_DEVICE, zipOut, progress);
        zipClose(zipOut, "");
    }
    task->finished();
}

// Wipes current save for game, then copies backup from SD to filesystem
// Note: Needs to be tested better some time
static void restoreBackup(sys::task *task, sys::sharedTaskData sharedData)
{
    if (sharedData == nullptr)
    {
        task->finished();
        return;
    }

    // Error code for later
    std::error_code errorCode;

    // Set status, cast args
    std::shared_ptr<pathData> dataIn = std::static_pointer_cast<pathData>(sharedData);
    sys::progressTask *progress = reinterpret_cast<sys::progressTask *>(task);

    // Get extension before we try to unzip something that isn't a zip
    std::string fileExtension = stringUtil::getExtensionFromString(dataIn->source);

    if(std::filesystem::is_directory(dataIn->source))
    {
        // Add trailing slash
        std::string source = dataIn->source + "/";
        // Get a test listing first to prevent writing an empty folder to save
        fs::directoryListing testListing(source);

        if(testListing.getListingCount() > 0)
        {
            // Folder isn't empty, erase
            fs::io::deleteDirectoryRecursively(fs::DEFAULT_SAVE_MOUNT_DEVICE);
            // Copy source to save container
            fs::io::copyDirectoryCommit(source, fs::DEFAULT_SAVE_MOUNT_DEVICE, dataIn->journalSize, progress);
        }
    }
    else if(std::filesystem::is_directory(dataIn->source) == false && fileExtension == "zip")
    {
        // Open zip for decompressing
        unzFile unzip = unzOpen64(dataIn->source.c_str());
        // Check if opening was successful and not empty first
        if(unzip != NULL && fs::zip::zipFileIsNotEmpty(unzip))
        {
            fs::zip::copyZipToDirectory(unzip, fs::DEFAULT_SAVE_MOUNT_DEVICE, dataIn->journalSize, progress);
        }
        // Close zip
        unzClose(unzip);
    }
    task->finished();
}

static void deleteBackup(sys::task *task, sys::sharedTaskData sharedData)
{
    if (sharedData == nullptr)
    {
        task->finished();
        return;
    }

    // Error code for filesystem later
    std::error_code errorCode;

    // Set thread status
    task->setThreadStatus(ui::strings::getString(THREAD_STATUS_DELETING_FILE, 0));

    // Cast args
    std::shared_ptr<pathData> dataIn = std::static_pointer_cast<pathData>(sharedData);

    if (std::filesystem::is_directory(dataIn->destination))
    {
        fs::io::deleteDirectoryRecursively(dataIn->destination);
    }
    else
    {
        std::filesystem::remove(dataIn->destination, errorCode);
    }
    // Refresh to reflect changes
    dataIn->sendingState->loadDirectoryList();

    task->finished();
}

backupMenuState::backupMenuState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo) :
m_CurrentUser(currentUser), 
m_CurrentUserSaveInfo(currentUserSaveInfo), 
m_CurrentTitleInfo(currentTitleInfo),
m_BackupMenuControlGuide(ui::strings::getString(FOLDER_MENU_CONTROL_GUIDE, 0)),
m_OutputBasePath(config::getWorkingDirectory() + m_CurrentTitleInfo->getPathSafeTitle() + "/"),
m_PanelWidth(graphics::systemFont::getTextWidth(m_BackupMenuControlGuide, 18)),
m_BackupPanel(std::make_unique<ui::slidePanel>(BACKUP_PANEL_NAME, m_PanelWidth + 64, ui::PANEL_SIDE_RIGHT)),
m_BackupListing(std::make_unique<fs::directoryListing>(m_OutputBasePath)),
m_SaveListing(std::make_unique<fs::directoryListing>(fs::DEFAULT_SAVE_MOUNT_DEVICE)),
m_BackupMenu(std::make_unique<ui::menu>(10, 4, m_PanelWidth + 44, 18, 6))
{
    // Make sure path exists
    std::filesystem::create_directories(m_OutputBasePath);
    // Render target that menu is rendered to
    m_BackupMenuRenderTarget = graphics::textureManager::createTexture(BACKUP_MENU_TARGET_NAME, m_PanelWidth + 64, 720, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
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
            std::shared_ptr<backupData> backupTaskData = std::make_shared<backupData>();
            backupTaskData->currentUser = m_CurrentUser;
            backupTaskData->currentTitleInfo = m_CurrentTitleInfo;
            backupTaskData->outputBasePath = m_OutputBasePath;
            backupTaskData->sendingState = this;

            // Task state
            createAndPushNewProgressState(createNewBackup, backupTaskData);
        }
        else
        {
            ui::popMessage::newMessage(ui::strings::getString(POP_MESSAGE_SAVE_EMPTY, 0), ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
        }
    }
    else if (sys::input::buttonDown(HidNpadButton_A) && selected > 0 && m_SaveListing->getListingCount() > 0)
    {
        // Selected item
        std::string selectedItem = m_BackupListing->getItemAt(selected - 1);
        // Get overwrite string
        std::string confirmOverwrite = stringUtil::getFormattedString(ui::strings::getCString(CONFIRM_OVERWRITE, 0), selectedItem.c_str());

        // Arg pointer
        std::shared_ptr<pathData> paths = std::make_shared<pathData>();
        paths->destination = m_BackupListing->getFullPathToItemAt(selected - 1);

        // Create confirmation
        confirmAction(confirmOverwrite, overwriteBackup, paths, sys::taskTypes::TASK_TYPE_PROGRESS);
    }
    else if (sys::input::buttonDown(HidNpadButton_Y) && selected > 0)
    {
        // Get selected menu item
        std::string selectedItem = m_BackupListing->getItemAt(selected - 1);
        // Get restoration string
        std::string confirmRestore = stringUtil::getFormattedString(ui::strings::getCString(CONFIRM_RESTORE, 0), selectedItem.c_str());

        // Setup confirmation arg pointer
        std::shared_ptr<pathData> paths = std::make_shared<pathData>();
        paths->source = m_BackupListing->getFullPathToItemAt(selected - 1); // Need to subtract one to account for 'New' option
        paths->journalSize = m_CurrentTitleInfo->getJournalSize(static_cast<FsSaveDataType>(m_CurrentUserSaveInfo->getSaveDataInfo().save_data_type)); // Needed to prevent commit errors

        // Create confirmation
        confirmAction(confirmRestore, restoreBackup, paths, sys::taskTypes::TASK_TYPE_PROGRESS);
    }
    else if (sys::input::buttonDown(HidNpadButton_X) && selected > 0)
    {
        // Get selected
        std::string selectedItem = m_BackupListing->getItemAt(selected - 1);
        // Get confirmation string
        std::string confirmDelete = stringUtil::getFormattedString(ui::strings::getCString(CONFIRM_DELETE, 0), selectedItem.c_str());

        // Setup args
        std::shared_ptr<pathData> paths = std::make_shared<pathData>();
        paths->destination = m_BackupListing->getFullPathToItemAt(selected - 1);
        paths->sendingState = this;

        // Create confirm and push
        confirmAction(confirmDelete, deleteBackup, paths, sys::taskTypes::TASK_TYPE_TASK);
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
    graphics::sdlTexture panelTarget = m_BackupPanel->getPanelRenderTarget();
    graphics::textureClear(panelTarget.get(), COLOR_SLIDE_PANEL_TARGET);

    // Clear menu render target
    graphics::textureClear(m_BackupMenuRenderTarget.get(), COLOR_TRANSPARENT);

    // Render menu to menu render target
    m_BackupMenu->render(m_BackupMenuRenderTarget.get());

    // Render menu target to panel target (this is so the menu can only be draw so far and not cover guider)
    graphics::textureRender(m_BackupMenuRenderTarget.get(), panelTarget.get(), 0, 0);

    // Draw divider and guide on bottom
    graphics::renderLine(panelTarget.get(), 18, 648, m_PanelWidth + 54, 648, COLOR_WHITE);
    graphics::systemFont::renderText(m_BackupMenuControlGuide, panelTarget.get(), 32, 673, 18, COLOR_WHITE);

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
    m_BackupMenu->addOption(ui::strings::getString(BACKUP_MENU_NEW, 0));
    for (int i = 0; i < listingCount; i++)
    {
        m_BackupMenu->addOption(m_BackupListing->getItemAt(i));
    }
}

void createAndPushNewBackupMenuState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo)
{
    // Create new backupmenu
    std::unique_ptr<appState> newBackupMenuState = std::make_unique<backupMenuState>(currentUser, currentUserSaveInfo, currentTitleInfo);
    // have JKSV push it to back of vector
    jksv::pushNewState(newBackupMenuState);
}