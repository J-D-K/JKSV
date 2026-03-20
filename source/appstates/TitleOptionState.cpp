#include "appstates/TitleOptionState.hpp"

#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/FileModeState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TitleInfoState.hpp"
#include "config/config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "graphics/colors.hpp"
#include "keyboard/keyboard.hpp"
#include "logging/logger.hpp"
#include "remote/remote.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "tasks/titleoptions.hpp"
#include "ui/PopMessageManager.hpp"

#include <cstring>

namespace
{
    // Enum for menu.
    enum
    {
        INFORMATION,
        BLACKLIST,
        CHANGE_OUTPUT,
        FILE_MODE,
        DELETE_ALL_LOCAL_BACKUPS,
        DELETE_ALL_REMOTE_BACKUPS,
        RESET_SAVE_DATA,
        DELETE_SAVE_FROM_SYSTEM,
        EXTEND_CONTAINER,
        EXPORT_SVI
    };
} // namespace

//                      ---- Construction ----

TitleOptionState::TitleOptionState(data::User *user,
                                   data::TitleInfo *titleInfo,
                                   const FsSaveDataInfo *saveInfo,
                                   TitleSelectCommon *titleSelect)
    : m_user(user)
    , m_titleInfo(titleInfo)
    , m_saveInfo(saveInfo)
    , m_titleSelect(titleSelect)
    , m_dataStruct(std::make_shared<TitleOptionState::DataStruct>())
{
    TitleOptionState::initialize_static_members();
    TitleOptionState::initialize_data_struct();
}

//                      ---- Public functions ----

void TitleOptionState::update(const sdl2::Input &input)
{
    const bool hasFocus = BaseState::has_focus();

    sm_slidePanel->unhide_on_focus(hasFocus);
    sm_slidePanel->update(input, hasFocus);

    const bool isOpen   = sm_slidePanel->is_open();
    const bool aPressed = input.button_pressed(HidNpadButton_A);
    const bool bPressed = input.button_pressed(HidNpadButton_B);
    const int selected  = sm_titleOptionMenu->get_selected();

    if (m_refreshRequired)
    {
        MainMenuState::refresh_view_states();
        m_refreshRequired = false;
        return;
    }

    if (aPressed && isOpen)
    {
        switch (selected)
        {
            case INFORMATION:               TitleOptionState::create_push_info_state(); break;
            case BLACKLIST:                 TitleOptionState::add_to_blacklist(); break;
            case CHANGE_OUTPUT:             TitleOptionState::change_output_directory(); break;
            case FILE_MODE:                 TitleOptionState::create_push_file_mode(); break;
            case DELETE_ALL_LOCAL_BACKUPS:  TitleOptionState::delete_all_local_backups(); break;
            case DELETE_ALL_REMOTE_BACKUPS: TitleOptionState::delete_all_remote_backups(); break;
            case RESET_SAVE_DATA:           TitleOptionState::reset_save_data(); break;
            case DELETE_SAVE_FROM_SYSTEM:   TitleOptionState::delete_save_from_system(); break;
            case EXTEND_CONTAINER:          TitleOptionState::extend_save_container(); break;
            case EXPORT_SVI:                TitleOptionState::export_svi_file(); break;
        }
    }
    else if (bPressed || m_exitRequired)
    {
        sm_slidePanel->close();
        m_exitRequired = false;
    }
    else if (sm_slidePanel->is_closed()) { TitleOptionState::deactivate_state(); }
}

void TitleOptionState::sub_update() { sm_slidePanel->sub_update(); }

void TitleOptionState::render(sdl2::Renderer &renderer)
{
    // Grab focus.
    const bool hasFocus = BaseState::has_focus();

    // Render panel.
    sm_slidePanel->render(renderer, hasFocus);
}

void TitleOptionState::close_on_update() { m_exitRequired = true; }

void TitleOptionState::refresh_required() { m_refreshRequired = true; }

//                      ---- Private functions ----

void TitleOptionState::initialize_static_members()
{
    if (sm_slidePanel && sm_titleOptionMenu) { return; }

    sm_slidePanel      = std::make_unique<ui::SlideOutPanel>(480, ui::SlideOutPanel::Side::Right);
    sm_titleOptionMenu = std::make_shared<ui::Menu>(8, 8, 460, 22, graphics::SCREEN_HEIGHT);

    for (int i = 0; const char *option = strings::get_by_name(strings::names::TITLEOPTION, i); i++)
    {
        sm_titleOptionMenu->add_option(option);
    }
    sm_slidePanel->push_new_element(sm_titleOptionMenu);
}

void TitleOptionState::initialize_data_struct()
{
    m_dataStruct->user          = m_user;
    m_dataStruct->titleInfo     = m_titleInfo;
    m_dataStruct->saveInfo      = m_saveInfo;
    m_dataStruct->spawningState = this;
    m_dataStruct->titleSelect   = m_titleSelect;
}

void TitleOptionState::create_push_info_state()
{
    sm_slidePanel->hide();

    TitleInfoState::create_and_push(m_user, m_titleInfo, m_saveInfo);
}

void TitleOptionState::add_to_blacklist()
{
    const char *title         = m_titleInfo->get_title();
    const char *confirmFormat = strings::get_by_name(strings::names::TITLEOPTION_CONFS, 0);
    std::string query         = stringutil::get_formatted_string(confirmFormat, title);

    ConfirmTask::create_push_fade(query, false, tasks::titleoptions::blacklist_title, nullptr, m_dataStruct);
}

void TitleOptionState::change_output_directory()
{
    static constexpr size_t SIZE_PATH_BUFFER = 0x200;

    const int popTicks                            = ui::PopMessageManager::DEFAULT_TICKS;
    const char *pathSafe                          = m_titleInfo->get_path_safe_title();
    const char *popFailed                         = strings::get_by_name(strings::names::TITLEOPTION_POPS, 9);
    std::array<char, SIZE_PATH_BUFFER> pathBuffer = {0};

    {
        const char *title        = m_titleInfo->get_title();
        const char *headerFormat = strings::get_by_name(strings::names::KEYBOARD, 7);
        const std::string header = stringutil::get_formatted_string(headerFormat, title);
        const bool inputValid    = keyboard::get_input(SwkbdType_QWERTY, pathSafe, header, pathBuffer.data(), SIZE_PATH_BUFFER);
        if (!inputValid) { return; }
    }

    const bool sanitized = stringutil::sanitize_string_for_path(pathBuffer.data(), pathBuffer.data(), SIZE_PATH_BUFFER);
    const bool empty     = std::char_traits<char>::length(pathBuffer.data()) <= 0;
    if (!sanitized || empty)
    {
        ui::PopMessageManager::push_message(popTicks, popFailed);
        return;
    }

    const fslib::Path workDir{config::get_working_directory()};
    const fslib::Path oldPath{workDir / pathSafe};
    const fslib::Path newPath{workDir / pathBuffer.data()};
    const bool oldExists    = fslib::directory_exists(oldPath);
    const bool renameFailed = oldExists && error::fslib(fslib::rename_directory(oldPath, newPath));
    if (oldExists && renameFailed) { ui::PopMessageManager::push_message(popTicks, popFailed); }

    // Need to change WebDav to match.
    remote::Storage *remote = remote::get_remote_storage();
    const bool dirExists = remote && !remote->supports_utf8() && remote->directory_exists(pathSafe); // This is guaranteed DAV
    if (dirExists)
    {
        remote::Item *item = remote->get_directory_by_name(pathSafe);
        remote->rename_item(item, pathBuffer.data());
    }

    const uint64_t applicationID = m_titleInfo->get_application_id();
    m_titleInfo->set_path_safe_title(pathBuffer.data());
    config::add_custom_path(applicationID, pathBuffer.data());

    const char *popSuccessFormat = strings::get_by_name(strings::names::TITLEOPTION_POPS, 8);
    std::string popSuccess       = stringutil::get_formatted_string(popSuccessFormat, pathBuffer.data());
    ui::PopMessageManager::push_message(popTicks, popSuccess);
}

void TitleOptionState::create_push_file_mode()
{
    const bool saveOpened = fslib::open_save_data_with_save_info(fs::DEFAULT_SAVE_MOUNT, *m_saveInfo);
    if (!saveOpened) { return; }

    const FsSaveDataType saveType = m_user->get_account_save_type();
    const bool isSystem           = saveType == FsSaveDataType_System || saveType == FsSaveDataType_SystemBcat;

    FsSaveDataExtraData extraData{};
    const bool readExtra      = fs::read_save_extra_data(m_saveInfo, extraData);
    const int64_t journalSize = readExtra ? extraData.journal_size : m_titleInfo->get_journal_size(saveType);

    FileModeState::create_and_push(fs::DEFAULT_SAVE_MOUNT, "sdmc", journalSize, isSystem);
    sm_slidePanel->hide();
}

void TitleOptionState::delete_all_local_backups()
{
    const char *title         = m_titleInfo->get_title();
    const char *confirmFormat = strings::get_by_name(strings::names::TITLEOPTION_CONFS, 1);
    std::string query         = stringutil::get_formatted_string(confirmFormat, title);

    ConfirmTask::create_push_fade(query, true, tasks::titleoptions::delete_all_local_backups_for_title, nullptr, m_dataStruct);
}

void TitleOptionState::delete_all_remote_backups()
{
    const char *title         = m_titleInfo->get_title();
    const char *confirmFormat = strings::get_by_name(strings::names::TITLEOPTION_CONFS, 1);
    std::string query         = stringutil::get_formatted_string(confirmFormat, title);

    ConfirmTask::create_push_fade(query, true, tasks::titleoptions::delete_all_remote_backups_for_title, nullptr, m_dataStruct);
}

void TitleOptionState::reset_save_data()
{
    const int popTicks            = ui::PopMessageManager::DEFAULT_TICKS;
    const bool allowSystemWriting = config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM);
    if (!allowSystemWriting && fs::is_system_save_data(m_saveInfo))
    {
        const char *popNoSysWrite = strings::get_by_name(strings::names::TITLEOPTION_POPS, 6);
        ui::PopMessageManager::push_message(popTicks, popNoSysWrite);
        return;
    }

    const char *title         = m_titleInfo->get_title();
    const char *confirmFormat = strings::get_by_name(strings::names::TITLEOPTION_CONFS, 2);
    std::string query         = stringutil::get_formatted_string(confirmFormat, title);

    ConfirmTask::create_push_fade(query, true, tasks::titleoptions::reset_save_data, nullptr, m_dataStruct);
}

void TitleOptionState::delete_save_from_system()
{
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;

    // This isn't allowed at ALL.
    if (fs::is_system_save_data(m_saveInfo))
    {
        const char *popUnavailable = strings::get_by_name(strings::names::TITLEOPTION_POPS, 6);
        ui::PopMessageManager::push_message(popTicks, popUnavailable);
        return;
    }

    const char *nickname      = m_user->get_nickname();
    const char *title         = m_titleInfo->get_title();
    const char *confirmFormat = strings::get_by_name(strings::names::TITLEOPTION_CONFS, 3);
    std::string query         = stringutil::get_formatted_string(confirmFormat, nickname, title);

    ConfirmTask::create_push_fade(query, true, tasks::titleoptions::delete_save_data_from_system, nullptr, m_dataStruct);
}

void TitleOptionState::extend_save_container()
{
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;

    if (fs::is_system_save_data(m_saveInfo))
    {
        const char *popUnavailable = strings::get_by_name(strings::names::TITLEOPTION_POPS, 6);
        ui::PopMessageManager::push_message(popTicks, popUnavailable);
        return;
    }

    TaskState::create_and_push(tasks::titleoptions::extend_save_data, m_dataStruct);
}

void TitleOptionState::export_svi_file()
{
    static constexpr size_t SIZE_SVI_FILE = sizeof(uint64_t) + sizeof(NsApplicationControlData);

    const int popTicks    = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popFailed = strings::get_by_name(strings::names::TITLEOPTION_POPS, 5);

    const uint64_t applicationID = m_titleInfo->get_application_id();
    const std::string titleIdHex = stringutil::get_formatted_string("%016llX", applicationID);
    const fslib::Path sviDir{config::get_working_directory() / "svi"};
    const fslib::Path sviPath{sviDir / titleIdHex + ".svi"};

    const bool dirExists   = fslib::directory_exists(sviDir);
    const bool createError = !dirExists && error::fslib(fslib::create_directory(sviDir));
    if (!dirExists && createError)
    {
        ui::PopMessageManager::push_message(popTicks, popFailed);
        return;
    }

    const bool exists = fslib::file_exists(sviPath);
    if (exists)
    {
        ui::PopMessageManager::push_message(popTicks, popFailed);
        return;
    }

    fslib::File sviFile{sviPath, FsOpenMode_Create | FsOpenMode_Write, SIZE_SVI_FILE};
    if (!sviFile.is_open())
    {
        ui::PopMessageManager::push_message(popTicks, popFailed);
        return;
    }

    const NsApplicationControlData *controlData = m_titleInfo->get_control_data();
    const bool magicWritten                     = sviFile.write(&fs::SAVE_META_MAGIC, sizeof(uint32_t)) == sizeof(uint32_t);
    const bool appIdWritten = magicWritten && sviFile.write(&applicationID, sizeof(uint64_t)) == sizeof(uint64_t);
    const bool controlWritten =
        appIdWritten && sviFile.write(controlData, sizeof(NsApplicationControlData)) == sizeof(NsApplicationControlData);
    if (!magicWritten || !appIdWritten || !controlWritten)
    {
        ui::PopMessageManager::push_message(popTicks, popFailed);
        return;
    }

    const char *popSuccess = strings::get_by_name(strings::names::TITLEOPTION_POPS, 4);
    ui::PopMessageManager::push_message(popTicks, popSuccess);
}

void TitleOptionState::deactivate_state()
{
    sm_slidePanel->reset();
    sm_titleOptionMenu->set_selected(0);
    BaseState::deactivate();
}
