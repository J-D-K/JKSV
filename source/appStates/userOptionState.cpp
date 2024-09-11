#include "appStates/userOptionState.hpp"
#include "appStates/progressState.hpp"
#include "appStates/saveCreateState.hpp"
#include "appStates/confirmState.hpp"
#include "system/progressTask.hpp"
#include "filesystem/filesystem.hpp"
#include "jksv.hpp"
#include "config.hpp"
#include "stringUtil.hpp"

namespace
{
    // Slide panel name
    const std::string SLIDE_PANEL_NAME = "userOptionPanel";
    // This is how many options are in the map for this menu
    const int OPTION_MENU_COUNT = 4;
    // UI Strings this state uses
    const std::string OPTION_MENU_STRING_NAME = "userOptions";
    const std::string THREAD_STATUS_CREATING_SAVE_DATA = "threadStatusCreatingSaveData";
    const std::string THREAD_STATUS_DELETING_SAVE_DATA = "threadStatusDeletingSaveData";
    const std::string CONFIRM_DELETE_ALL_USER_SAVES = "saveDataDeleteAllUser";
    // Enum for menu opts
    enum
    {
        OPTION_BACKUP_ALL_FOR_USER,
        OPTION_CREATE_SAVE_DATA,
        OPTION_CREATE_ALL_SAVE_DATA,
        OPTION_DELETE_ALL_SAVES_FOR_USER
    };
}

// Data used for tasks here
struct userOptionData : sys::taskData
{
    data::user *currentUser;
};

// Tasks for this state
void dumpAllForUser(sys::task *task, sys::sharedTaskData sharedData)
{
    // Check if args are nullptr and bail
    if (sharedData == nullptr)
    {
        task->finished();
        return;
    }
    // Cast to progress since that's what we need
    sys::progressTask *progressTask = reinterpret_cast<sys::progressTask *>(task);
    // Cast args to what we need
    std::shared_ptr<userOptionData> optionsData = static_pointer_cast<userOptionData>(sharedData);
    // Get current user's save count
    int currentUserSaveCount = optionsData->currentUser->getTotalUserSaveInfo();
    // Loop through and make backups of all of them
    for (int i = 0; i < currentUserSaveCount; i++)
    {
        // Get pointer to current save info
        data::userSaveInfo *currentSaveInfo = optionsData->currentUser->getUserSaveInfoAt(i);
        // Get pointer to title data
        data::titleInfo *currentTitleInfo = data::getTitleInfoByTitleID(currentSaveInfo->getTitleID());
        // Make sure destination exists
        fs::createTitleDirectoryByTID(currentSaveInfo->getTitleID());
        // Try to mount it
        bool saveIsMounted = fs::mountSaveData(currentSaveInfo->getSaveDataInfo());

        if (saveIsMounted && fs::directoryContainsFiles(fs::DEFAULT_SAVE_MOUNT_DEVICE) && config::getByKey(CONFIG_USE_ZIP))
        {
            std::string destinationPath = config::getWorkingDirectory() + currentTitleInfo->getPathSafeTitle() + "/" + optionsData->currentUser->getPathSafeUsername() +
                                          " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YMD) + ".zip";
            zipFile outputZip = zipOpen64(destinationPath.c_str(), 0);
            fs::zip::copyDirectoryToZip(fs::DEFAULT_SAVE_MOUNT_DEVICE, outputZip, progressTask);
        }
        else if (saveIsMounted && fs::directoryContainsFiles(fs::DEFAULT_SAVE_MOUNT_DEVICE))
        {
            std::string destinationPath = config::getWorkingDirectory() + currentTitleInfo->getPathSafeTitle() + "/" + optionsData->currentUser->getPathSafeUsername() +
                                          " - " + stringUtil::getTimeAndDateString(stringUtil::DATE_FORMAT_YMD) + "/";
            std::filesystem::create_directories(destinationPath);
            fs::io::copyDirectory(fs::DEFAULT_SAVE_MOUNT_DEVICE, destinationPath, progressTask);
        }
        fs::unmountSaveData();
    }
    task->finished();
}

void createAllSaveDataForUser(sys::task *task, sys::sharedTaskData sharedData)
{
    if (sharedData == nullptr)
    {
        task->finished();
        return;
    }

    // Cast to correct type
    std::shared_ptr<userOptionData> dataIn = std::static_pointer_cast<userOptionData>(sharedData);

    // Get title map reference
    data::titleMap &titleMap = data::getTitleMap();

    // Loop through map. Have to be careful though.
    for (auto &currentTitle : titleMap)
    {
        if (dataIn->currentUser->getUserType() == data::userType::TYPE_USER && currentTitle.second.hasAccountSaveData())
        {
            // Set status string.
            std::string threadStatus = stringUtil::getFormattedString(ui::strings::getCString(THREAD_STATUS_CREATING_SAVE_DATA, 0), currentTitle.second.getTitle().c_str());
            task->setThreadStatus(threadStatus);

            // Try to:
            fs::createSaveDataFileSystem(FsSaveDataType_Account, currentTitle.first, dataIn->currentUser->getAccountID(), 0);
        }
    }

    data::loadUserSaveInfo();

    task->finished();
}

void deleteAllSaveDataForUser(sys::task *task, sys::sharedTaskData sharedData)
{
    if (sharedData == nullptr)
    {
        task->finished();
        return;
    }

    std::shared_ptr<userOptionData> dataIn = std::static_pointer_cast<userOptionData>(sharedData);

    // Loop through and nuke them
    int totalUserSaves = dataIn->currentUser->getTotalUserSaveInfo();
    for (int i = 0; i < totalUserSaves; i++)
    {
        // Save info is needed first for titleID
        data::userSaveInfo *currentUserSaveInfo = dataIn->currentUser->getUserSaveInfoAt(i);

        // Need this for game title
        data::titleInfo *currentTitleInfo = data::getTitleInfoByTitleID(currentUserSaveInfo->getTitleID());

        // Status string
        std::string threadStatus = stringUtil::getFormattedString(ui::strings::getCString(THREAD_STATUS_DELETING_SAVE_DATA, 0), currentTitleInfo->getTitle().c_str());
        task->setThreadStatus(threadStatus);

        fs::deleteSaveDataFileSystem(currentUserSaveInfo->getSaveDataInfo().save_data_id);
    }

    data::loadUserSaveInfo();

    task->finished();
}

userOptionState::userOptionState(data::user *currentUser) : m_CurrentUser(currentUser),
                                                            m_UserOptionPanel(std::make_unique<ui::slidePanel>(SLIDE_PANEL_NAME, 410, ui::slidePanelSide::PANEL_SIDE_RIGHT)),
                                                            m_OptionsMenu(std::make_unique<ui::menu>(8, 8, 390, 20, 6))
{
    // First option is special and needs user name
    std::string firstMenuOption = stringUtil::getFormattedString(ui::strings::getCString(OPTION_MENU_STRING_NAME, 0), m_CurrentUser->getUsername().c_str());
    // Add it
    m_OptionsMenu->addOption(firstMenuOption);
    // Add other options to menu
    for (int i = 1; i < OPTION_MENU_COUNT; i++)
    {
        m_OptionsMenu->addOption(ui::strings::getString(OPTION_MENU_STRING_NAME, i));
    }
}

userOptionState::~userOptionState() {}

void userOptionState::update(void)
{
    // Update panel and menu
    m_UserOptionPanel->update();
    m_OptionsMenu->update();

    if (sys::input::buttonDown(HidNpadButton_A))
    {
        switch (m_OptionsMenu->getSelected())
        {
            case OPTION_BACKUP_ALL_FOR_USER:
            {
                // Data to send
                std::shared_ptr<userOptionData> optionsData = std::make_shared<userOptionData>();
                optionsData->currentUser = m_CurrentUser;
                // Push new state/task
                sharedAppState dumpAllProgressState = createNewProgressTask(dumpAllForUser, optionsData);
                jksv::pushState(dumpAllProgressState);
            }
            break;

            case OPTION_CREATE_SAVE_DATA:
            {
                // What save type we're working with. Using UserID to figure it out.
                FsSaveDataType saveDataType;
                switch (m_CurrentUser->getAccountIDU128())
                {
                    case 2:
                    {
                        saveDataType = FsSaveDataType_Bcat;
                    }
                    break;

                    case 3:
                    {
                        saveDataType = FsSaveDataType_Device;
                    }
                    break;

                    case 5:
                    {
                        saveDataType = FsSaveDataType_Cache;
                    }
                    break;

                    default:
                    {
                        saveDataType = FsSaveDataType_Account;
                    }
                    break;
                }

                // Push the state
                sharedAppState saveCreateState = createSaveCreationState(m_CurrentUser, saveDataType);
                jksv::pushState(saveCreateState);
            }
            break;

            case OPTION_CREATE_ALL_SAVE_DATA: // This needs to be fixed up...
            {
                std::shared_ptr<userOptionData> userData = std::make_shared<userOptionData>();
                userData->currentUser = m_CurrentUser;
                sharedAppState createAllSaveTaskState = createNewTask(createAllSaveDataForUser, userData);
                jksv::pushState(createAllSaveTaskState);
            }
            break;

            case OPTION_DELETE_ALL_SAVES_FOR_USER:
            {
                // Create confirmation string
                std::string confirmDeleteAll = stringUtil::getFormattedString(ui::strings::getCString(CONFIRM_DELETE_ALL_USER_SAVES, 0), m_CurrentUser->getUsername().c_str());

                // Shared data
                std::shared_ptr<userOptionData> userData = std::make_shared<userOptionData>();
                userData->currentUser = m_CurrentUser;

                // Confirm it first.
                confirmAndPerformAction(confirmDeleteAll, deleteAllSaveDataForUser, userData, sys::taskTypes::TASK_TYPE_TASK);
            }
            break;
        }
    }
    else if (sys::input::buttonDown(HidNpadButton_B))
    {
        m_UserOptionPanel->closePanel();
    }
    else if (m_UserOptionPanel->isClosed())
    {
        appState::deactivateState();
    }
}

void userOptionState::render(void)
{
    // Get parent classes rendertarget
    graphics::sdlTexture panelTarget = m_UserOptionPanel->getPanelRenderTarget();
    // Clear it
    graphics::textureClear(panelTarget.get(), COLOR_SLIDE_PANEL_TARGET);
    // Render menu to it
    m_OptionsMenu->render(panelTarget.get());
    // Render panel
    m_UserOptionPanel->render();
}
