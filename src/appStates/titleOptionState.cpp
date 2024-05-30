#include <memory>
#include "appStates/titleOptionState.hpp"
#include "appStates/confirmState.hpp"
#include "data/data.hpp"
#include "filesystem/filesystem.hpp"
#include "ui/ui.hpp"
#include "system/task.hpp"
#include "system/input.hpp"
#include "jksv.hpp"
#include "log.hpp"

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
    logger::log("Arg check");

    // Set status
    task->setThreadStatus(ui::strings::getString(LANG_THREAD_RESET_SAVE_DATA, 0));
    logger::log("Set status");
    
    // Cast args
    std::shared_ptr<titleOptsArgs> argsIn = std::static_pointer_cast<titleOptsArgs>(args);
    logger::log("Cast pointer");

    if(fs::mountSaveData(argsIn->currentUserSaveInfo->getSaveDataInfo()))
    {
        logger::log("Save mounted");
        // This does everything this needs
        fs::eraseSaveData();
        logger::log("Erase Save Data");
        // Close
        fs::unmountSaveData();
        logger::log("unmount");
        // Display success
        ui::popMessage::newMessage(ui::strings::getString(LANG_SAVEDATA_RESET_SUCCESS, 0), ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
        logger::log("POP");
    }
    else
    {
        // Display failure
        ui::popMessage::newMessage(ui::strings::getString(LANG_SAVEDATA_ERROR_MOUNTING, 0), ui::popMessage::POPMESSAGE_DEFAULT_TICKS);
    }

    task->finished();
}

titleOptionState::titleOptionState(data::user *currentUser, data::userSaveInfo *currentUserSaveInfo, data::titleInfo *currentTitleInfo) : m_CurrentUser(currentUser), m_CurrentUserSaveInfo(currentUserSaveInfo), m_CurrentTitleInfo(currentTitleInfo)
{
    // Slide out panel
    m_SlidePanel = std::make_unique<ui::slidePanel>("titleOptionPanel", 410, ui::PANEL_SIDE_RIGHT);
    // Menu
    m_OptionsMenu = std::make_unique<ui::menu>(10, 32, 390, 20, 7);
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