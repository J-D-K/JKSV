#include "appstates/ReceivedBackupsState.hpp"

#include "appstates/ConfirmState.hpp"
#include "appstates/UserSelectState.hpp"
#include "fs/save_mount.hpp"
#include "graphics/colors.hpp"
#include "input.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"

#include <algorithm>
#include <string_view>

namespace
{
constexpr std::string_view TARGET_NAME = "ReceivedBackupsTarget";

constexpr int TARGET_WIDTH  = 1080;
constexpr int TARGET_HEIGHT = 555;
constexpr int TARGET_X      = 201;
constexpr int TARGET_Y      = 91;

const char *received_string(int index, const char *fallback)
{
    const char *string = strings::get_by_name(strings::names::PEER, index);
    return string ? string : fallback;
}
} // namespace

// Post-receive flow:
//   Start -> pick a user -> list compatible backups -> mark one per game ->
//   confirm overwrite -> run the batch import -> report the result.
// Only account saves for installed titles can be applied automatically; every
// other received file stays on the SD card for Save Import.

//                      ---- Construction ----

ReceivedBackupsState::ReceivedBackupsState(std::vector<peer::ReceivedBackup> backups)
    : m_received(std::move(backups))
    , m_renderTarget(sdl::TextureManager::load(TARGET_NAME, TARGET_WIDTH, TARGET_HEIGHT, SDL_TEXTUREACCESS_TARGET))
{
    m_controlGuide = ui::ControlGuide::create(received_string(54, "A: Apply  Y: Mark  B: Back"));
    m_menu         = ui::Menu::create(32, 40, 1000, 23, TARGET_HEIGHT);
}

void ReceivedBackupsState::create_and_push(std::vector<peer::ReceivedBackup> backups)
{
    StateManager::push_state(ReceivedBackupsState::create(std::move(backups)));
}

//                      ---- Public functions ----

void ReceivedBackupsState::update()
{
    const bool hasFocus = BaseState::has_focus();

    switch (m_stage)
    {
        case Stage::Start:
            ReceivedBackupsState::start();
            break;

        case Stage::WaitingUser:
            if (m_userReady)
            {
                m_userReady = false;
                if (!m_user) { BaseState::deactivate(); }
                else { ReceivedBackupsState::build_list(); }
            }
            break;

        case Stage::List:
            ReceivedBackupsState::update_list(hasFocus);
            break;

        case Stage::Applying:
            if (m_batchData && m_batchData->finished)
            {
                const int succeeded = m_batchData->succeeded;
                const int failed    = m_batchData->failed;

                const std::string message =
                    stringutil::get_formatted_string(received_string(52, "Batch finished: %d applied, %d failed."),
                                                     succeeded,
                                                     failed);
                const ui::PopKind kind =
                    failed == 0 ? ui::PopKind::Success : (succeeded == 0 ? ui::PopKind::Error : ui::PopKind::Warning);
                m_status = message;
                ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS, message, kind);

                m_batchData.reset();
                m_stage = Stage::Done;
                BaseState::deactivate();
            }
            else if (m_batchData && !m_batchData->started)
            {
                // The confirmation was declined. Return to the list with marks intact.
                m_batchData.reset();
                m_stage = Stage::List;
            }
            break;

        case Stage::Done:
            break;
    }
}

void ReceivedBackupsState::sub_update() { m_controlGuide->sub_update(); }

void ReceivedBackupsState::render()
{
    const bool hasFocus = BaseState::has_focus();

    m_renderTarget->clear(colors::CLEAR_COLOR);
    sdl::text::render(m_renderTarget,
                      32,
                      10,
                      24,
                      sdl::text::NO_WRAP,
                      colors::WHITE,
                      received_string(48, "Select backups to apply"));
    m_menu->render(m_renderTarget, hasFocus);
    m_renderTarget->render(sdl::Texture::Null, TARGET_X, TARGET_Y);
    m_controlGuide->render(sdl::Texture::Null, hasFocus);
}

//                      ---- Private functions ----

void ReceivedBackupsState::start()
{
    // First pass: keep account saves whose title is installed. The user is not
    // known yet, so the final compatibility check happens in build_list().
    for (int i = 0; i < static_cast<int>(m_received.size()); i++)
    {
        const peer::ReceivedBackup &backup = m_received[i];
        const bool accountSave             = backup.metadata.saveDataType == static_cast<uint8_t>(FsSaveDataType_Account);
        if (backup.metadataValid && accountSave && data::is_application_installed(backup.applicationID))
        {
            m_applicable.push_back(i);
        }
    }

    if (m_applicable.empty())
    {
        ReceivedBackupsState::report(49, "Received backups saved, but none can be applied automatically.", ui::PopKind::Info);
        m_stage = Stage::Done;
        BaseState::deactivate();
        return;
    }

    std::vector<data::User *> users;
    data::get_users(users);

    std::vector<data::User *> candidates;
    for (data::User *user : users)
    {
        if (user && user->get_account_save_type() == FsSaveDataType_Account) { candidates.push_back(user); }
    }

    if (candidates.empty())
    {
        ReceivedBackupsState::report(49, "Received backups saved, but none can be applied automatically.", ui::PopKind::Info);
        m_stage = Stage::Done;
        BaseState::deactivate();
        return;
    }

    m_stage = Stage::WaitingUser;
    UserSelectState::create_and_push_if_needed(candidates,
                                               [this](data::User *user)
                                               {
                                                   m_user      = user;
                                                   m_userReady = true;
                                               });
}

void ReceivedBackupsState::build_list()
{
    // Narrow the list to backups whose save type matches the chosen user and
    // whose title is still installed.
    m_applicable.clear();
    for (int i = 0; i < static_cast<int>(m_received.size()); i++)
    {
        const peer::ReceivedBackup &backup = m_received[i];
        const bool compatible = backup.metadata.saveDataType == static_cast<uint8_t>(m_user->get_account_save_type());
        if (backup.metadataValid && compatible && data::is_application_installed(backup.applicationID))
        {
            m_applicable.push_back(i);
        }
    }

    if (m_applicable.empty())
    {
        ReceivedBackupsState::report(49, "Received backups saved, but none can be applied automatically.", ui::PopKind::Info);
        m_stage = Stage::Done;
        BaseState::deactivate();
        return;
    }

    m_menu->reset();
    m_display.clear();
    m_marked.clear();

    for (int index : m_applicable)
    {
        const peer::ReceivedBackup &backup = m_received[index];
        const std::string title            = ReceivedBackupsState::title_for(index);
        std::string display                = title + " - " + backup.path.get_filename();
        m_display.push_back(display);
        m_menu->add_option("[ ] " + display);
    }

    m_stage = Stage::List;
}

void ReceivedBackupsState::update_list(bool hasFocus)
{
    m_menu->update(hasFocus);
    m_controlGuide->update(hasFocus);

    if (input::button_pressed(HidNpadButton_Y)) { ReceivedBackupsState::toggle_mark(m_menu->get_selected()); }
    else if (input::button_pressed(HidNpadButton_A)) { ReceivedBackupsState::apply_selected(); }
    else if (input::button_pressed(HidNpadButton_B)) { BaseState::deactivate(); }
}

void ReceivedBackupsState::apply_selected()
{
    std::vector<int> selection = m_marked;
    if (selection.empty())
    {
        const int selected = m_menu->get_selected();
        if (selected >= 0 && selected < static_cast<int>(m_applicable.size())) { selection.push_back(selected); }
    }
    if (selection.empty()) { return; }

    int overwrite = 0;
    for (int menuIndex : selection)
    {
        const peer::ReceivedBackup &backup = m_received[m_applicable[menuIndex]];
        if (ReceivedBackupsState::save_exists_for_user(m_user, backup.metadata)) { overwrite++; }
    }

    const int total = static_cast<int>(selection.size());
    std::string query;
    if (overwrite > 0)
    {
        query = stringutil::get_formatted_string(
            received_string(51, "%d of %d selected backup(s) would overwrite existing saves. Apply?"),
            overwrite,
            total);
    }
    else { query = stringutil::get_formatted_string(received_string(50, "Apply %d selected backup(s)?"), total); }

    m_batchData       = std::make_shared<tasks::saveimport::BatchData>();
    m_batchData->user = m_user;
    for (int menuIndex : selection) { m_batchData->paths.push_back(m_received[m_applicable[menuIndex]].path); }

    m_stage = Stage::Applying;
    ConfirmProgress::create_and_push(query, true, tasks::saveimport::import_save_backups, nullptr, m_batchData);
}

void ReceivedBackupsState::report(int index, const char *fallback, ui::PopKind kind)
{
    const std::string message = received_string(index, fallback);
    m_status                  = message;
    ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS, message, kind);
}

void ReceivedBackupsState::set_option(int index, bool marked)
{
    if (index < 0 || index >= static_cast<int>(m_display.size())) { return; }

    const std::string option = (marked ? "[x] " : "[ ] ") + m_display[index];
    m_menu->edit_option(index, option);
}

void ReceivedBackupsState::toggle_mark(int index)
{
    if (index < 0 || index >= static_cast<int>(m_applicable.size())) { return; }

    auto found = std::find(m_marked.begin(), m_marked.end(), index);
    if (found != m_marked.end())
    {
        // Already marked: unmark it.
        m_marked.erase(found);
        ReceivedBackupsState::set_option(index, false);
        return;
    }

    // Only one backup per game may be marked. Unmark any other backup of the same title first.
    const uint64_t applicationID = m_received[m_applicable[index]].applicationID;
    for (auto it = m_marked.begin(); it != m_marked.end();)
    {
        const int other = *it;
        if (m_received[m_applicable[other]].applicationID == applicationID)
        {
            ReceivedBackupsState::set_option(other, false);
            it = m_marked.erase(it);
        }
        else { ++it; }
    }

    m_marked.push_back(index);
    ReceivedBackupsState::set_option(index, true);
}

bool ReceivedBackupsState::save_exists_for_user(data::User *user, const fs::SaveMetaData &meta)
{
    if (!user) { return false; }

    bool saveExists = false;
    switch (user->get_account_save_type())
    {
        case FsSaveDataType_Account:
            saveExists =
                fslib::open_account_save_file_system(fs::DEFAULT_SAVE_MOUNT, meta.applicationID, user->get_account_id());
            break;
        case FsSaveDataType_Bcat:
            saveExists = fslib::open_bcat_save_file_system(fs::DEFAULT_SAVE_MOUNT, meta.applicationID);
            break;
        case FsSaveDataType_Device:
            saveExists = fslib::open_device_save_file_system(fs::DEFAULT_SAVE_MOUNT, meta.applicationID);
            break;
        case FsSaveDataType_Cache:
            saveExists = fslib::open_cache_save_file_system(fs::DEFAULT_SAVE_MOUNT,
                                                            meta.applicationID,
                                                            meta.saveDataIndex,
                                                            static_cast<FsSaveDataSpaceId>(meta.saveDataIndex));
            break;
        default:
            saveExists = false;
    }
    if (saveExists) { fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT); }

    return saveExists;
}

std::string ReceivedBackupsState::title_for(int receivedIndex) const
{
    const peer::ReceivedBackup &backup = m_received[receivedIndex];
    data::TitleInfo *titleInfo         = data::get_title_info_by_id(backup.applicationID);
    if (titleInfo && titleInfo->get_title()) { return titleInfo->get_title(); }
    return backup.folderName;
}
