#include <memory>
#include <filesystem>
#include <cstring>
#include "ui/ui.hpp"
#include "config.hpp"
#include "system/input.hpp"
#include "graphics/graphics.hpp"
#include "appstates/backupMenuState.hpp"
#include "appStates/taskState.hpp"
#include "filesystem/filesystem.hpp"
#include "system/task.hpp"
#include "stringUtil.hpp"
#include "jksv.hpp"

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
struct pathArg : sys::taskArgs
{
    std::string path;
};

// These are the functions used for tasks
// Creates a new backup when new is selected. args must be a backupArgs struct as a shared pointer
void createNewBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    // Make sure we weren't passed nullptr
    if(args == nullptr)
    {
        task->finished();
        return;
    }

    // Set task status, cast args to backupArgs
    task->setThreadStatus("REPLACE THIS LATER");
    std::shared_ptr<backupArgs> argsIn = std::static_pointer_cast<backupArgs>(args);

    // Process shortcuts or get name for backup
    std::string backupName;
    if(sys::input::buttonHeld(HidNpadButton_R) || config::getByKey(CONFIG_AUTONAME_BACKUPS))
    {
        backupName = argsIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YMD);
    }
    else if(sys::input::buttonHeld(HidNpadButton_L))
    {
        backupName = argsIn->currentUser->getPathSafeUsername() + " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YDM);
    }
    else if(sys::input::buttonHeld(HidNpadButton_ZL))
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
    if(backupName.empty() == false)
    {
        // Get extension and full path
        std::string extension = stringUtil::getExtensionFromString(backupName);
        std::string path = argsIn->outputBasePath + backupName + "/";

        if(config::getByKey(CONFIG_USE_ZIP) || extension == "zip")
        {
            zipFile zipOut = zipOpen64(path.c_str(), 0);
            fs::io::copyDirectoryToZip(path, zipOut);
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
void overwriteBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    // Bail if no args present
    if(args == nullptr)
    {
        task->finished();
        return;
    }

    // Set status, cast args to type
    task->setThreadStatus("REPLACE THIS LATER");
    std::shared_ptr<pathArg> argIn = std::static_pointer_cast<pathArg>(args);

    // Get a test listing
    fs::directoryListing testListing(fs::DEFAULT_SAVE_MOUNT_DEVICE);

    // So JKSV doesn't create empty folders
    int saveFileCount = testListing.getListingCount();

    // For checking if overwriting a zip
    std::string fileExtension = stringUtil::getExtensionFromString(argIn->path);

    if(std::filesystem::is_directory(argIn->path) && saveFileCount > 0)
    {
        // Delete backup then recreate directory
        std::filesystem::remove_all(argIn->path);
        std::filesystem::create_directory(argIn->path);
        fs::io::copyDirectory(fs::DEFAULT_SAVE_MOUNT_DEVICE, argIn->path);
    }
    else if(std::filesystem::is_directory(argIn->path) == false && fileExtension == "zip" && saveFileCount > 0)
    {
        std::filesystem::remove(argIn->path);
        zipFile zipOut  = zipOpen64(argIn->path.c_str(), 0);
        fs::io::copyDirectoryToZip(argIn->path, zipOut);
        zipClose(zipOut, "");
    }
    task->finished();
}

// Wipes current save for game, then copies backup from SD to filesystem
void restoreBackup(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    if(args == nullptr)
    {
        task->finished();
        return;
    }

    task->setThreadStatus("REPLACE THIS LATER");
    std::shared_ptr<pathArg> argIn = std::static_pointer_cast<pathArg>(args);

    // Test if there are actually files in the save
    fs::directoryListing testListing(argIn->path);
    int saveFileCount = testListing.getListingCount();
}

backupMenuState::backupMenuState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo) : m_CurrentUser(currentUser), m_CurrentUserSaveInfo(currentUserSaveInfo), m_CurrentTitleInfo(currentTitleInfo)
{
    // This is the path used for all in/output. Create it if it doesn't exist.
    m_OutputBasePath = config::getWorkingDirectory() + m_CurrentTitleInfo->getPathSafeTitle() + "/";
    std::filesystem::create_directories(m_OutputBasePath);

    // Controls displayed at bottom
    m_BackupMenuControlGuide = ui::strings::getString(LANG_FOLDER_GUIDE, 0);
    int panelWidth = graphics::systemFont::getTextWidth(m_BackupMenuControlGuide, 18);

    // The actual panel
    m_BackupPanel = std::make_unique<ui::slidePanel>("backupMenuPanel", panelWidth + 64, ui::slidePanelSide::PANEL_SIDE_RIGHT);
    // The menu of backups
    m_BackupMenu = std::make_unique<ui::menu>(10, 4, panelWidth + 44, 18, 7);
    // Directory listing of backups
    m_BackupListing = std::make_unique<fs::directoryListing>(m_OutputBasePath);

    backupMenuState::loadDirectoryList();
}

backupMenuState::~backupMenuState() 
{
    fs::unmountSaveData();
}

void backupMenuState::update(void)
{
    m_BackupPanel->update();

    m_BackupMenu->update();

    int selected = m_BackupMenu->getSelected();
    if(sys::input::buttonDown(HidNpadButton_A) && selected == 0)
    {
        // Data to send to task
        std::shared_ptr<backupArgs> backupTaskArgs = std::make_shared<backupArgs>();
        backupTaskArgs->currentUser = m_CurrentUser;
        backupTaskArgs->currentTitleInfo = m_CurrentTitleInfo;
        backupTaskArgs->outputBasePath = m_OutputBasePath;
        backupTaskArgs->sendingState = this;

        // Task state
        std::unique_ptr<appState> backupTask = std::make_unique<taskState> (createNewBackup, backupTaskArgs);
        jksv::pushNewState(backupTask);
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
    SDL_Texture *panelTarget = m_BackupPanel->getPanelRenderTarget();

    graphics::textureClear(panelTarget, COLOR_SLIDE_PANEL_TARGET);
    m_BackupMenu->render(panelTarget);

    m_BackupPanel->render();
}

void backupMenuState::loadDirectoryList(void)
{
    // Reload list, clear menu
    m_BackupListing->loadListing();
    m_BackupMenu->clearMenu();

    int listingCount = m_BackupListing->getListingCount();
    m_BackupMenu->addOpt(ui::strings::getString(LANG_FOLDER_MENU_NEW, 0));
    for(int i = 0; i < listingCount; i++)
    {
        m_BackupMenu->addOpt(m_BackupListing->getItemAt(i));
    }
}
