#include "appstates/UserOptionState.hpp"

#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/SaveCreateState.hpp"
#include "appstates/SaveImportState.hpp"
#include "appstates/TaskState.hpp"
#include "config/config.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "logging/logger.hpp"
#include "remote/remote.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "tasks/useroptions.hpp"
#include "ui/PopMessageManager.hpp"

namespace
{
    // Enum for menu switch case.
    enum
    {
        BACKUP_ALL,
        CREATE_SAVE,
        CREATE_ALL_SAVE,
        DELETE_ALL_SAVE,
        IMPORT_SAVE_DATA
    };
} // namespace

//                      ---- Construction ----

UserOptionState::UserOptionState(data::User *user, TitleSelectCommon *titleSelect)
    : m_user(user)
    , m_titleSelect(titleSelect)
    , m_userOptionMenu(ui::Menu::create(8, 8, 460, 22, graphics::SCREEN_HEIGHT))
    , m_dataStruct(std::make_shared<UserOptionState::DataStruct>())
{
    UserOptionState::create_menu_panel();
    UserOptionState::load_menu_strings();
    UserOptionState::initialize_data_struct();
}

//                      ---- Public functions ----

void UserOptionState::update(const sdl2::Input &input)
{
    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input.button_pressed(HidNpadButton_A);
    const bool bPressed = input.button_pressed(HidNpadButton_B);

    // Start by running the unhide check routine and then update the panel.
    sm_menuPanel->unhide_on_focus(hasFocus);
    sm_menuPanel->update(input, hasFocus);

    // Refresh here if needed to avoid threading issues.
    if (m_refreshRequired)
    {
        m_user->load_user_data();
        m_titleSelect->refresh();
        m_refreshRequired = false;
    }

    if (aPressed)
    {
        const int selected = m_userOptionMenu->get_selected();

        switch (selected)
        {
            case BACKUP_ALL:       UserOptionState::backup_all(); break;
            case CREATE_SAVE:      UserOptionState::create_save_create(); break;
            case CREATE_ALL_SAVE:  UserOptionState::create_all_save_data(); break;
            case DELETE_ALL_SAVE:  UserOptionState::delete_all_save_data(); break;
            case IMPORT_SAVE_DATA: UserOptionState::create_push_save_import(); break;
        }
    }
    else if (bPressed) { sm_menuPanel->close(); }
    else if (sm_menuPanel->is_closed()) { UserOptionState::deactivate_state(); }
}

void UserOptionState::sub_update() { sm_menuPanel->sub_update(); }

void UserOptionState::render(sdl2::Renderer &renderer)
{
    // Render target user's title selection screen.
    m_titleSelect->render(renderer);

    // Render panel.
    sm_menuPanel->render(renderer, BaseState::has_focus());
}

void UserOptionState::refresh_required() { m_refreshRequired = true; }

//                      ---- Private functions ----

void UserOptionState::create_menu_panel()
{
    static constexpr int SIZE_PANEL_WIDTH = 480;
    if (sm_menuPanel) { return; }

    sm_menuPanel = ui::SlideOutPanel::create(SIZE_PANEL_WIDTH, ui::SlideOutPanel::Side::Right);
}

void UserOptionState::load_menu_strings()
{
    const char *nickname = m_user->get_nickname();

    for (int i = 0; const char *format = strings::get_by_name(strings::names::USEROPTION_MENU, i); i++)
    {
        const std::string option = stringutil::get_formatted_string(format, nickname);
        m_userOptionMenu->add_option(option);
    }
    sm_menuPanel->push_new_element(m_userOptionMenu);
}

void UserOptionState::initialize_data_struct()
{
    m_dataStruct->user          = m_user;
    m_dataStruct->spawningState = this;
}

void UserOptionState::backup_all()
{
    remote::Storage *remote   = remote::get_remote_storage();
    bool autoUpload           = config::get_by_key(config::keys::AUTO_UPLOAD);
    const char *confirmFormat = strings::get_by_name(strings::names::USEROPTION_CONFS, 0);
    const char *nickname      = m_user->get_nickname();

    const std::string query = stringutil::get_formatted_string(confirmFormat, nickname);
    if (remote && autoUpload)
    {
        ConfirmProgress::create_push_fade(query, true, tasks::useroptions::backup_all_for_user_remote, nullptr, m_dataStruct);
    }
    else
    {
        ConfirmProgress::create_push_fade(query, true, tasks::useroptions::backup_all_for_user_local, nullptr, m_dataStruct);
    }
}

void UserOptionState::create_save_create()
{
    if (m_user->get_account_save_type() == FsSaveDataType_System) { return; }

    sm_menuPanel->hide();
    SaveCreateState::create_and_push(m_user, m_titleSelect);
}

void UserOptionState::create_all_save_data()
{
    if (m_user->get_account_save_type() == FsSaveDataType_System) { return; }

    const char *confirmFormat = strings::get_by_name(strings::names::USEROPTION_CONFS, 1);
    const char *nickname      = m_user->get_nickname();

    const std::string query = stringutil::get_formatted_string(confirmFormat, nickname);
    ConfirmTask::create_push_fade(query, true, tasks::useroptions::create_all_save_data_for_user, nullptr, m_dataStruct);
}

void UserOptionState::delete_all_save_data()
{
    const uint8_t saveType = m_user->get_account_save_type();
    if (saveType == FsSaveDataType_System || saveType == FsSaveDataType_SystemBcat) { return; }

    const char *confirmFormat     = strings::get_by_name(strings::names::USEROPTION_CONFS, 2);
    const char *nickname          = m_user->get_nickname();
    const std::string queryString = stringutil::get_formatted_string(confirmFormat, nickname);
    ConfirmTask::create_push_fade(queryString, true, tasks::useroptions::delete_all_save_data_for_user, nullptr, m_dataStruct);
}

void UserOptionState::create_push_save_import()
{
    const fslib::Path saveDirPath{config::get_working_directory() / "saves"};
    if (!fs::directory_has_contents(saveDirPath)) { return; }

    sm_menuPanel->hide();
    SaveImportState::create_and_push(m_user);
}

void UserOptionState::deactivate_state()
{
    sm_menuPanel->clear_elements();
    sm_menuPanel->reset();
    BaseState::deactivate();
}
