#include "appstates/SaveImportState.hpp"

#include "appstates/BackupMenuState.hpp"
#include "appstates/ConfirmState.hpp"
#include "config/config.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "tasks/saveimport.hpp"

//                      ---- Construction ----

SaveImportState::SaveImportState(data::User *user)
    : m_user(user)
    , m_saveDir()
{
    SaveImportState::initialize_static_members();
    SaveImportState::initialize_directory_menu();
}

//                      ---- Public functions ----

void SaveImportState::update(const sdl2::Input &input)
{
    const bool hasFocus = BaseState::has_focus();

    sm_slidePanel->update(input, hasFocus);
    if (!sm_slidePanel->is_open()) { return; }

    const bool aPressed = input.button_pressed(HidNpadButton_A);
    const bool bPressed = input.button_pressed(HidNpadButton_B);

    if (aPressed) { SaveImportState::import_backup(); }
    else if (bPressed) { sm_slidePanel->close(); }
    else if (sm_slidePanel->is_closed()) { SaveImportState::deactivate_state(); }
}

void SaveImportState::render(sdl2::Renderer &renderer)
{
    const bool hasFocus = BaseState::has_focus();

    sm_slidePanel->render(renderer, hasFocus);
}

//                      ---- Private functions ----

void SaveImportState::initialize_static_members()
{
    static constexpr int SIZE_PANEL_WIDTH = 512;

    if (sm_taskData && sm_saveMenu && sm_slidePanel) { return; }

    sm_taskData = std::make_shared<SaveImportState::DataStruct>();

    sm_saveMenu   = ui::Menu::create(8, 8, 492, 22, graphics::SCREEN_HEIGHT);
    sm_slidePanel = ui::SlideOutPanel::create(SIZE_PANEL_WIDTH, ui::SlideOutPanel::Side::Right);

    sm_slidePanel->push_new_element(sm_saveMenu);
}

void SaveImportState::initialize_directory_menu()
{
    const fslib::Path saveDirPath{config::get_working_directory() / "saves"};

    m_saveDir.open(saveDirPath);
    if (!m_saveDir.is_open()) { return; }

    for (const fslib::DirectoryEntry &entry : m_saveDir) { sm_saveMenu->add_option(entry.get_filename()); }
}

void SaveImportState::import_backup()
{
    const int selected      = sm_saveMenu->get_selected();
    const bool holdRequired = config::get_by_key(config::keys::HOLD_FOR_RESTORATION);

    const fslib::DirectoryEntry &target = m_saveDir[selected];
    const char *targetName              = target.get_filename();
    sm_taskData->user                   = m_user;
    sm_taskData->path                   = config::get_working_directory() / "saves" / targetName;

    const char *confirmFormat = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 1);
    std::string query         = stringutil::get_formatted_string(confirmFormat, targetName);

    ConfirmProgress::create_and_push(query, holdRequired, tasks::saveimport::import_save_backup, nullptr, sm_taskData);
}

void SaveImportState::deactivate_state()
{
    sm_saveMenu->reset();
    sm_slidePanel->reset();
    BaseState::deactivate();
}
