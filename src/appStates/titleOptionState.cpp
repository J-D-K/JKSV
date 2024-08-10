#include <memory>
#include <filesystem>

#include "appStates/titleOptionState.hpp"
#include "appStates/confirmState.hpp"
#include "data/data.hpp"
#include "filesystem/filesystem.hpp"
#include "ui/ui.hpp"
#include "system/task.hpp"
#include "system/input.hpp"
#include "stringUtil.hpp"
#include "jksv.hpp"
#include "log.hpp"

namespace
{
    // Texture/target names
    const std::string PANEL_RENDER_TARGET_NAME = "titleOptionPanel";
    // Strings
    const std::string CONFIRM_RESET_SAVE_DATA = "confirmResetSaveData";
    const std::string THREAD_STATUS_RESETTING_SAVE = "threadStatusResettingSaveData";
    const std::string POP_SAVE_RESET_SUCCESS = "saveDataResetSuccess";
    const std::string POP_ERROR_MOUNTING_SAVE = "popErrorMountingSave";
    const std::string TITLE_OPTIONS_MENU_STRINGS = "titleOptions";
}

enum
{
    TITLE_INFO,
    ADD_TO_BLACKLIST,
    CHANGE_OUTPUT_FOLDER,
    OPEN_IN_FILE_MODE,
    DELETE_ALL_BACKUPS,
    RESET_SAVE_DATA,
    DELETE_SAVE_DATA,
    EXTEND_SAVE_DATA,
    EXPORT_SVI
};

// This struct is used to send data to tasks
struct titleOptsData : sys::taskData
{
    data::user *currentUser;
    data::userSaveInfo *currentUserSaveInfo;
    data::titleInfo *currentTitleInfo;
};

// These are the task/thread functions for this state
// This was needed to debug something. Only one for now.
void resetSaveData(sys::task *task, sys::sharedTaskData sharedData)
{
    if (sharedData == nullptr)
    {
        task->finished();
        return;
    }

    // Error code for filesystem
    std::error_code errorCode;

    // Set status
    task->setThreadStatus(ui::strings::getString(THREAD_STATUS_RESETTING_SAVE, 0));

    // Cast args
    std::shared_ptr<titleOptsData> dataIn = std::static_pointer_cast<titleOptsData>(sharedData);

    if (fs::mountSaveData(dataIn->currentUserSaveInfo->getSaveDataInfo()))
    {
        // Erase save data in container
        // To do: remove_all keeps failing if the folder has sub dirs. Maybe I should just stick with my own?
        fs::io::deleteDirectoryRecursively(fs::DEFAULT_SAVE_MOUNT_DEVICE);
        // Commit changes
        fs::commitSaveData();
        // Close
        fs::unmountSaveData();
        // Display success
        std::string successMessage = stringUtil::getFormattedString(ui::strings::getCString(POP_SAVE_RESET_SUCCESS, 0), dataIn->currentTitleInfo->getTitle().c_str());
        ui::popMessage::newMessage(successMessage, ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
    }
    else
    {
        // Display failure
        ui::popMessage::newMessage(ui::strings::getString(POP_ERROR_MOUNTING_SAVE, 0), ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
    }

    task->finished();
}

titleOptionState::titleOptionState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo) : m_CurrentUser(currentUser),
                                                                                                                                          m_CurrentUserSaveInfo(currentUserSaveInfo),
                                                                                                                                          m_CurrentTitleInfo(currentTitleInfo),
                                                                                                                                          m_SlidePanel(std::make_unique<ui::slidePanel>(PANEL_RENDER_TARGET_NAME, 410, ui::PANEL_SIDE_RIGHT)),
                                                                                                                                          m_OptionsMenu(std::make_unique<ui::menu>(8, 8, 390, 20, 7))
{
    // Setup menu
    for (int i = 0; i < 9; i++)
    {
        m_OptionsMenu->addOption(ui::strings::getString(TITLE_OPTIONS_MENU_STRINGS, i));
    }
}

titleOptionState::~titleOptionState() {}

void titleOptionState::update(void)
{
    // Update panel and menu
    m_SlidePanel->update();
    m_OptionsMenu->update();

    if (sys::input::buttonDown(HidNpadButton_A))
    {
        switch (m_OptionsMenu->getSelected())
        {
            // This is needed to debug something... and itself.
            case RESET_SAVE_DATA:
            {
                // Prepare confirmation string
                std::string confirmationString = ui::strings::getString(CONFIRM_RESET_SAVE_DATA, 0);

                // Data to send
                std::shared_ptr<titleOptsData> optsData = std::make_shared<titleOptsData>();
                optsData->currentUserSaveInfo = m_CurrentUserSaveInfo;
                optsData->currentTitleInfo = m_CurrentTitleInfo;

                confirmAction(confirmationString, resetSaveData, optsData, sys::taskTypes::TASK_TYPE_TASK);
            }
            break;

            default:
            {
                ui::popMessage::newMessage("Not implemented yet...", ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
            }
            break;
        }
    }
    else if (sys::input::buttonDown(HidNpadButton_B))
    {
        m_SlidePanel->closePanel();
    }
    else if (m_SlidePanel->isClosed())
    {
        appState::deactivateState();
    }
}

void titleOptionState::render(void)
{
    // Get panel render target
    graphics::sdlTexture panelTarget = m_SlidePanel->getPanelRenderTarget();
    // Clear it
    graphics::textureClear(panelTarget.get(), COLOR_SLIDE_PANEL_TARGET);
    // Render menu to panel
    m_OptionsMenu->render(panelTarget.get());
    // Render panel to framebuffer
    m_SlidePanel->render();
}

void createAndPushNewTitleOptionState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo)
{
    std::unique_ptr<appState> newTitleOptionState = std::make_unique<titleOptionState>(currentUser, currentUserSaveInfo, currentTitleInfo);
    jksv::pushNewState(newTitleOptionState);
}