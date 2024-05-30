#include "appStates/titleOptionState.hpp"
#include "filesystem/filesystem.hpp"
#include "system/input.hpp"

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
            case RESET_SAVE_DATA:
            {
                if(fs::mountSaveData(m_CurrentUserSaveInfo->getSaveDataInfo()))
                {
                    fs::io::file::deleteDirectoryRecursively(fs::DEFAULT_SAVE_MOUNT_DEVICE);
                    fs::commitSaveData();
                    fs::unmountSaveData();
                }
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