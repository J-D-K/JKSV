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

// Texture/target names
const char *PANEL_RENDER_TARGET_NAME = "titleOptionPanel";

// Dimensions and coordinates for panel and menu
static const int PANEL_WIDTH = 410;
static const int MENU_X = 10;
static const int MENU_Y = 32;
static const int MENU_WIDTH = 390;
static const int MENU_FONT_SIZE = 20;
static const int MENU_SCROLL_LENGTH = 7;

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
struct titleOptsArgs : sys::taskArgs
{
    data::user *currentUser;
    data::userSaveInfo *currentUserSaveInfo;
    data::titleInfo *currentTitleInfo;
};

// These are the task/thread functions for this state
// This was needed to debug something. Only one for now.
void resetSaveData(sys::task *task, std::shared_ptr<sys::taskArgs> args)
{
    if(args == nullptr)
    {
        task->finished();
        return;
    }

    // Error code for filesystem
    std::error_code errorCode;

    // Set status
    task->setThreadStatus(ui::strings::getString(LANG_THREAD_RESET_SAVE_DATA, 0));
    
    // Cast args
    std::shared_ptr<titleOptsArgs> argsIn = std::static_pointer_cast<titleOptsArgs>(args);

    if(fs::mountSaveData(argsIn->currentUserSaveInfo->getSaveDataInfo()))
    {
        // Erase save data in container
        // To do: remove_all keeps failing if the folder has sub dirs. Maybe I should just stick with my own?
        std::filesystem::remove_all(fs::DEFAULT_SAVE_MOUNT_DEVICE, errorCode);
        // Commit changes
        fs::commitSaveData();
        // Close
        fs::unmountSaveData();
        // Display success
        std::string successMessage = stringUtil::getFormattedString(ui::strings::getCString(LANG_SAVEDATA_RESET_SUCCESS, 0), argsIn->currentTitleInfo->getTitle().c_str());
        ui::popMessage::newMessage(successMessage, ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
    }
    else
    {
        // Display failure
        ui::popMessage::newMessage(ui::strings::getString(LANG_SAVEDATA_ERROR_MOUNTING, 0), ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
    }

    task->finished();
}

titleOptionState::titleOptionState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo) : 
m_CurrentUser(currentUser), 
m_CurrentUserSaveInfo(currentUserSaveInfo), 
m_CurrentTitleInfo(currentTitleInfo),
m_SlidePanel(std::make_unique<ui::slidePanel>(PANEL_RENDER_TARGET_NAME, PANEL_WIDTH, ui::PANEL_SIDE_RIGHT)),
m_OptionsMenu(std::make_unique<ui::menu>(MENU_X, MENU_Y, MENU_WIDTH, MENU_FONT_SIZE, MENU_SCROLL_LENGTH))
{
    // Setup menu
    for(int i = 0; i < 9; i++)
    {
        m_OptionsMenu->addOpt(ui::strings::getString(LANG_TITLE_OPTIONS_MENU, i));
    }
}

titleOptionState::~titleOptionState() { }

void titleOptionState::update(void)
{
    // Update panel and menu
    m_SlidePanel->update();
    m_OptionsMenu->update();

    if(sys::input::buttonDown(HidNpadButton_A))
    {
        switch(m_OptionsMenu->getSelected())
        {
            // This is needed to debug something... and itself.
            case RESET_SAVE_DATA:
            {
                // Prepare confirmation
                std::string confirmationString = ui::strings::getString(LANG_CONFIRM_RESET_SAVEDATA, 0);

                // Data to send
                std::shared_ptr<titleOptsArgs> args = std::make_shared<titleOptsArgs>();
                args->currentUserSaveInfo = m_CurrentUserSaveInfo;
                args->currentTitleInfo = m_CurrentTitleInfo;

                // Confirm state
                std::unique_ptr<appState> confirmReset = std::make_unique<confirmState>(confirmationString, resetSaveData, args);

                // Push
                jksv::pushNewState(confirmReset);
            }
            break;

            default:
            {
                ui::popMessage::newMessage("Not implemented yet...", ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
            }
            break;
        }
    }
    else if(sys::input::buttonDown(HidNpadButton_B))
    {
        m_SlidePanel->closePanel();
    }
    else if(m_SlidePanel->isClosed())
    {
        appState::deactivateState();
    }
}

void titleOptionState::render(void)
{
    // Get panel render target
    SDL_Texture *panelTarget = m_SlidePanel->getPanelRenderTarget();
    // Clear it
    graphics::textureClear(panelTarget, COLOR_SLIDE_PANEL_TARGET);
    // Render menu to panel
    m_OptionsMenu->render(panelTarget);
    // Render panel to framebuffer
    m_SlidePanel->render();
}