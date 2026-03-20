#include "appstates/BackupMenuState.hpp"

#include "StateManager.hpp"
#include "appstates/ConfirmState.hpp"
#include "appstates/FadeState.hpp"
#include "appstates/ProgressState.hpp"
#include "config/config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "graphics/ScopedRender.hpp"
#include "graphics/colors.hpp"
#include "graphics/fonts.hpp"
#include "keyboard/keyboard.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "tasks/backup.hpp"
#include "ui/PopMessageManager.hpp"
#include "ui/TextScroll.hpp"

#include <cstring>

//                      ---- Construction ----

BackupMenuState::BackupMenuState(data::User *user, data::TitleInfo *titleInfo, const FsSaveDataInfo *saveInfo)
    : m_user(user)
    , m_titleInfo(titleInfo)
    , m_saveInfo(saveInfo)
    , m_saveType(m_user->get_account_save_type())
    , m_directoryPath(config::get_working_directory() / m_titleInfo->get_path_safe_title())
    , m_dataStruct(std::make_shared<BackupMenuState::DataStruct>())
    , m_controlGuide(strings::get_by_name(strings::names::CONTROL_GUIDES, 2))
{
    BackupMenuState::initialize_static_members();
    BackupMenuState::ensure_target_directory();
    BackupMenuState::initialize_remote_storage();
    BackupMenuState::initialize_task_data();
    BackupMenuState::initialize_info_string();
    BackupMenuState::save_data_check();
    BackupMenuState::refresh();
}

//                      ---- Public functions ----

void BackupMenuState::update(const sdl2::Input &input)
{
    // Grab focus once and only once.
    const bool hasFocus = BaseState::has_focus();

    // Update the panel first.
    sm_slidePanel->update(input, hasFocus);

    // Grab these.
    const bool isOpen   = sm_slidePanel->is_open();
    const bool isClosed = sm_slidePanel->is_closed();

    // If the panel is closed, deactivate. If it's not open, return.
    if (isClosed) { BackupMenuState::deactivate_state(); }
    else if (!isOpen) { return; }

    // Grab the currently selected index.
    int selected{};
    {
        std::lock_guard menuGuard{sm_menuMutex};
        selected = sm_backupMenu->get_selected();
    }

    // Input bools.
    const bool aPressed  = input.button_pressed(HidNpadButton_A);
    const bool bPressed  = input.button_pressed(HidNpadButton_B);
    const bool xPressed  = input.button_pressed(HidNpadButton_B);
    const bool yPressed  = input.button_pressed(HidNpadButton_Y);
    const bool zrPressed = input.button_pressed(HidNpadButton_ZR);

    // Conditions.
    const bool newSelected     = selected == 0;
    const bool newBackup       = aPressed && newSelected && m_saveHasData;
    const bool overwriteBackup = aPressed && !newSelected && m_saveHasData;
    const bool restoreBackup   = yPressed && !newSelected;
    const bool deleteBackup    = xPressed && !newSelected;
    const bool uploadBackup    = zrPressed && !newSelected;
    const bool popEmpty        = aPressed && !m_saveHasData;

    if (newBackup) { BackupMenuState::name_and_create_backup(input); }
    else if (overwriteBackup) { BackupMenuState::confirm_overwrite(); }
    else if (restoreBackup) { BackupMenuState::confirm_restore(); }
    else if (deleteBackup) { BackupMenuState::confirm_delete(); }
    else if (uploadBackup) { BackupMenuState::upload_backup(); }
    else if (popEmpty) { BackupMenuState::pop_save_empty(); }
    else if (bPressed) { sm_slidePanel->close(); }
    else if (sm_slidePanel->is_closed()) { BackupMenuState::deactivate_state(); }

    // Lock and update the menu.
    std::lock_guard menuGuard{sm_menuMutex};
    sm_backupMenu->update(input, hasFocus);
}

void BackupMenuState::render(sdl2::Renderer &renderer)
{
    // Line render coords.
    static constexpr int LINE_X   = 10;
    static constexpr int LINE_A_Y = 42;
    static constexpr int LINE_B_Y = 648;

    static constexpr int CONTROL_X = 32;
    static constexpr int CONTROL_Y = 673;

    // Grab whether or not the state has focus and the render target for the panel.
    const bool hasFocus = BaseState::has_focus();

    // This is the target for the menu so it can't render outside of the lines above.
    {
        graphics::ScopedRender menuRender{renderer, sm_menuRenderTarget};
        renderer.frame_begin(colors::TRANSPARENT);

        // Lock and render the menu.
        {
            std::lock_guard menuGuard{sm_menuMutex};
            sm_backupMenu->render(renderer, hasFocus);
        }

        // Get target and change target.
        {
            graphics::ScopedRender slideRender{renderer, sm_slidePanel->get_target()};
            renderer.frame_begin(colors::SLIDE_PANEL_CLEAR);

            // Render the top, bottom lines. Control guide string.
            renderer.render_line(LINE_X, LINE_A_Y, sm_panelWidth - LINE_X, LINE_A_Y, colors::WHITE);
            renderer.render_line(LINE_X, LINE_B_Y, sm_panelWidth - LINE_X, LINE_B_Y, colors::WHITE);
            sm_font->render_text(CONTROL_X, CONTROL_Y, colors::WHITE, m_controlGuide);

            // Render the menu target to the slide panel target.
            sm_menuRenderTarget->render(0, 43);
        }
    }

    // Finally, render the target to the screen.
    sm_slidePanel->render(renderer, hasFocus);
}

void BackupMenuState::refresh()
{
    // Grab pointer to remote service.
    remote::Storage *remote = remote::get_remote_storage();

    // Re-open and refresh directory.
    m_directoryListing.open(m_directoryPath);
    // If neither are valid, return.
    if (!remote && !m_directoryListing.is_open()) { return; }

    // Lock the menu.
    std::lock_guard menuGuard{sm_menuMutex};

    // Clear the menu and the entry array.
    sm_backupMenu->reset();
    m_menuEntries.clear();

    // Grab the "New" string and add it & it's NULL entry in the vector.
    const char *optionNew = strings::get_by_name(strings::names::BACKUPMENU_MENU, 0);
    sm_backupMenu->add_option(optionNew);
    m_menuEntries.push_back({MenuEntryType::Null, 0});

    // Remote->Local.
    if (remote)
    {
        const std::string_view prefix = remote->get_prefix();
        remote->get_directory_listing(m_remoteListing);
        int index{};
        for (const remote::Item *item : m_remoteListing)
        {
            const std::string_view name = item->get_name();
            const std::string option    = stringutil::get_formatted_string("%s %s", prefix.data(), name.data());

            sm_backupMenu->add_option(option);
            m_menuEntries.push_back({MenuEntryType::Remote, index++});
        }
    }

    int index{};
    for (const fslib::DirectoryEntry &entry : m_directoryListing)
    {
        sm_backupMenu->add_option(entry.get_filename());
        m_menuEntries.push_back({MenuEntryType::Local, index++});
    }
}

void BackupMenuState::save_data_written()
{
    // Temporarily mount the save.
    fs::ScopedSaveMount tempMount{fs::DEFAULT_SAVE_MOUNT, m_saveInfo};

    // Check to see if the root is empty.
    fslib::Directory saveRoot{fs::DEFAULT_SAVE_ROOT, false};

    // Just check if the root is empty.
    m_saveHasData = saveRoot.get_count() > 0;
}

//                      ---- Private functions ----

void BackupMenuState::initialize_static_members()
{
    // Name of the render target for the backup menu.
    static constexpr std::string_view BACKUP_TARGET = "BackupMenuTarget";

    if (sm_backupMenu && sm_slidePanel && sm_menuRenderTarget && sm_panelWidth && sm_font) { return; }

    sm_font       = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::TWENTY_TWO_PIXEL,
                                                                              graphics::fonts::sizes::TWENTY_TWO_PIXEL);
    sm_panelWidth = sm_font->get_text_width(m_controlGuide) + 64;
    sm_backupMenu = ui::Menu::create(8, 8, sm_panelWidth - 16, 22, 600);
    sm_slidePanel = ui::SlideOutPanel::create(sm_panelWidth, ui::SlideOutPanel::Side::Right);
    sm_menuRenderTarget =
        sdl2::TextureManager::create_load_resource(BACKUP_TARGET, sm_panelWidth, 600, SDL_TEXTUREACCESS_TARGET);
}

void BackupMenuState::ensure_target_directory()
{
    const int popTicks    = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popFailed = strings::get_by_name(strings::names::BACKUPMENU_POPS, 12);

    // If this is enabled, don't bother.
    const remote::Storage *remote = remote::get_remote_storage();
    const bool autoUpload         = config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool keepLocal          = config::get_by_key(config::keys::KEEP_LOCAL_BACKUPS);
    const bool directoryNeeded    = (!remote || !autoUpload || keepLocal) && !fslib::directory_exists(m_directoryPath);
    const bool directoryFailed    = directoryNeeded && error::fslib(fslib::create_directory(m_directoryPath));
    if (directoryNeeded && directoryFailed) { ui::PopMessageManager::push_message(popTicks, popFailed); }
}

void BackupMenuState::initialize_task_data()
{
    // The other members are set upon actions being taken.
    m_dataStruct->user          = m_user;
    m_dataStruct->titleInfo     = m_titleInfo;
    m_dataStruct->saveInfo      = m_saveInfo;
    m_dataStruct->basePath      = &m_directoryPath;
    m_dataStruct->spawningState = this;
}

void BackupMenuState::initialize_info_string()
{
    const char *nickname         = m_user->get_nickname();
    const char *title            = m_titleInfo->get_title();
    const std::string infoString = stringutil::get_formatted_string("`%s` - %s", nickname, title);
    m_titleScroll = ui::TextScroll::create(infoString, 8, 8, sm_panelWidth - 16, 30, 22, colors::WHITE, colors::TRANSPARENT);

    sm_slidePanel->push_new_element(m_titleScroll);
}

void BackupMenuState::save_data_check()
{
    fs::ScopedSaveMount saveMount{fs::DEFAULT_SAVE_MOUNT, m_saveInfo};
    fslib::Directory saveRoot{fs::DEFAULT_SAVE_ROOT};
    m_saveHasData = saveRoot.is_open() && saveRoot.get_count() > 0;
}

void BackupMenuState::initialize_remote_storage()
{
    remote::Storage *remote = remote::get_remote_storage();
    if (!remote) { return; }

    const bool supportsUtf8            = remote->supports_utf8();
    const std::string_view remoteTitle = supportsUtf8 ? m_titleInfo->get_title() : m_titleInfo->get_path_safe_title();
    const bool remoteDirExists         = remote->directory_exists(remoteTitle);
    const bool remoteDirCreated        = !remoteDirExists && remote->create_directory(remoteTitle);
    if (!remoteDirExists && !remoteDirCreated) { return; }

    const remote::Item *remoteDir = remote->get_directory_by_name(remoteTitle);
    if (!remoteDir) { return; }

    remote->change_directory(remoteDir);
}

void BackupMenuState::name_and_create_backup(const sdl2::Input &input)
{
    // Size of the buffer for naming backups.
    static constexpr size_t SIZE_NAME_LENGTH = 0x80;

    // Zip extension because it's used in multiple spots.
    static constexpr const char *STRING_ZIP_EXT = ".zip";

    // Remote storage pointer. This is only used for testing if it's valid at this point.
    remote::Storage *remote = remote::get_remote_storage();

    // Config needed.
    const bool autoName   = config::get_by_key(config::keys::AUTO_NAME_BACKUPS);
    const bool autoUpload = config::get_by_key(config::keys::AUTO_UPLOAD);
    const bool exportZip  = autoUpload || config::get_by_key(config::keys::EXPORT_TO_ZIP);

    // Input.
    const bool zrHeld = input.button_held(HidNpadButton_ZR);

    // Whether or not we should skip the keyboard.
    const bool autoNamed = (autoName || zrHeld); // This can be eval'd here.

    // This is the buffer for naming. It's auto filled as [User] - [Date].
    char name[SIZE_NAME_LENGTH + 1] = {0};
    {
        const char *nickname   = m_user->get_path_safe_nickname();
        const std::string date = stringutil::get_date_string();
        std::snprintf(name, SIZE_NAME_LENGTH, "%s - %s", nickname, date.c_str());
    }

    // Doing this like this so the strings don't linger. Dictionary entries for the keyboard.
    keyboard::Dictionary dictionary{};
    {
        // Array of dictionary strings.
        const std::array<std::string, 8> dictionaryStrings = {
            stringutil::get_date_string(stringutil::DateFormat::Year_Month_Day),
            stringutil::get_date_string(stringutil::DateFormat::Year_Day_Month),
            stringutil::get_date_string(stringutil::DateFormat::YearMonthDay),
            stringutil::get_date_string(stringutil::DateFormat::YearDayMonth),
            stringutil::get_date_string(stringutil::DateFormat::AscTime),
            m_user->get_path_safe_nickname(),
            STRING_ZIP_EXT};

        // Add them before continuing.
        for (const std::string_view word : dictionaryStrings) { dictionary.add_word_to_list(word); }
    }

    // Header string for the keyboard.
    const char *keyboardHeader = strings::get_by_name(strings::names::KEYBOARD, 0);
    // Stores whether or not the input was successful.
    const bool named =
        autoNamed || keyboard::get_input(SwkbdType_QWERTY, name, keyboardHeader, name, SIZE_NAME_LENGTH, dictionary);
    if (!named) { return; }

    // Send the signal that the backup task should signal completion.
    m_dataStruct->killTask = true;

    // Check for and append zip extension if needed.
    const bool hasZipExt = std::strstr(name, STRING_ZIP_EXT); // This might not be the best check.
    const bool needsZip  = !hasZipExt && (autoUpload || exportZip);
    if (needsZip) { std::strncat(name, STRING_ZIP_EXT, SIZE_NAME_LENGTH); }

    // This is used by both if the keep local is enabled anyway.
    m_dataStruct->path = m_directoryPath / name;

    if (autoUpload && remote) // If both autoUpload and remote is valid.
    {
        // Set the name.
        m_dataStruct->remoteName = name;

        // Start the process.
        ProgressState::create_push_fade(tasks::backup::create_new_backup_remote, m_dataStruct);
    }
    else
    {
        // Start the process.
        ProgressState::create_push_fade(tasks::backup::create_new_backup_local, m_dataStruct);
    }
}

void BackupMenuState::confirm_overwrite()
{
    // Grab selected index and entry.
    const int selected     = sm_backupMenu->get_selected();
    const MenuEntry &entry = m_menuEntries.at(selected);

    // Whether or not holding is required.
    const bool holdRequired = config::get_by_key(config::keys::HOLD_FOR_OVERWRITE);

    // Template/format for the confirmation.
    const char *confirmTemplate = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 0);

    if (entry.type == MenuEntryType::Remote)
    {
        m_dataStruct->remoteItem = m_remoteListing.at(entry.index);
        const char *itemName     = m_dataStruct->remoteItem->get_name().data();
        std::string query        = stringutil::get_formatted_string(confirmTemplate, itemName);

        ConfirmProgress::create_push_fade(query, holdRequired, tasks::backup::overwrite_backup_remote, nullptr, m_dataStruct);
    }
    else if (entry.type == MenuEntryType::Local)
    {
        m_dataStruct->path     = m_directoryPath / m_directoryListing[entry.index];
        const char *targetName = m_directoryListing[entry.index].get_filename();
        std::string query      = stringutil::get_formatted_string(confirmTemplate, targetName);

        ConfirmProgress::create_push_fade(query, holdRequired, tasks::backup::overwrite_backup_local, nullptr, m_dataStruct);
    }
}

void BackupMenuState::confirm_restore()
{
    // Tired of typing it out.
    static constexpr int POP_TICKS = ui::PopMessageManager::DEFAULT_TICKS;

    // Grab selected.
    const int selected = sm_backupMenu->get_selected();

    // Entry reference.
    const MenuEntry &entry = m_menuEntries.at(selected);

    // Config needed.
    const bool holdRequired = config::get_by_key(config::keys::HOLD_FOR_RESTORATION);
    const bool allowSystem  = config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM);

    // Template for confirmation string.
    const char *confirmTemplate = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 1);

    // Whether or not we're working with a system type.
    const bool isSystem = BackupMenuState::user_is_system();

    // Whether or not the restoration is valid under the current conditions.
    const bool isValidRestore = !isSystem || allowSystem;
    if (!isValidRestore)
    {
        // Pop and return on trying to restore system when not enabled.
        const char *popInvalid = strings::get_by_name(strings::names::BACKUPMENU_POPS, 6);
        ui::PopMessageManager::push_message(POP_TICKS, popInvalid);
        return;
    }

    if (entry.type == MenuEntryType::Local)
    {
        // Target path we're working with.
        const fslib::Path target{m_directoryPath / m_directoryListing[entry.index]};

        // Check if it's a directory. Ensure it has contents either way.
        const bool targetIsDirectory = fslib::directory_exists(target);
        const bool backupIsGood      = targetIsDirectory ? fs::directory_has_contents(target) : fs::zip_has_contents(target);
        if (!backupIsGood)
        {
            const char *popEmpty = strings::get_by_name(strings::names::BACKUPMENU_POPS, 1);
            ui::PopMessageManager::push_message(POP_TICKS, popEmpty);
            return;
        }

        // Move the target to the struct path to pass it.
        m_dataStruct->path = std::move(target);

        // Construct our confirmation string.
        const char *targetName = m_directoryListing[entry.index].get_filename();
        std::string query      = stringutil::get_formatted_string(confirmTemplate, targetName);

        // Begin process.
        ConfirmProgress::create_push_fade(query, holdRequired, tasks::backup::restore_backup_local, nullptr, m_dataStruct);
    }
    else if (entry.type == MenuEntryType::Remote)
    {
        // Pointer to target we're working with.
        remote::Item *target = m_remoteListing[entry.index];

        // String for the confirmation
        std::string query = stringutil::get_formatted_string(confirmTemplate, target->get_name().data());

        // Set data needed.
        m_dataStruct->remoteItem = target;
        m_dataStruct->path       = m_directoryPath;

        // Pass go and collect $200.
        ConfirmProgress::create_push_fade(query, holdRequired, tasks::backup::restore_backup_remote, nullptr, m_dataStruct);
    }
}

void BackupMenuState::confirm_delete()
{
    // Grab the selected index and entry.
    const int selected     = sm_backupMenu->get_selected();
    const MenuEntry &entry = m_menuEntries.at(selected);

    // Whether or not the use desires to be forced to hold the button.
    const bool holdRequired = config::get_by_key(config::keys::HOLD_FOR_DELETION);

    // Template for the confirmation string.
    const char *confirmTemplate = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 2);

    if (entry.type == MenuEntryType::Local)
    {
        // Target path to delete.
        m_dataStruct->path = m_directoryPath / m_directoryListing[entry.index];

        // This is just the name for the confirmation.
        const char *targetName = m_directoryListing[entry.index].get_filename();
        std::string query      = stringutil::get_formatted_string(confirmTemplate, targetName);

        ConfirmTask::create_push_fade(query, holdRequired, tasks::backup::delete_backup_local, nullptr, m_dataStruct);
    }
    else if (entry.type == MenuEntryType::Remote)
    {
        m_dataStruct->remoteItem = m_remoteListing.at(entry.index);
        const char *itemName     = m_dataStruct->remoteItem->get_name().data();
        std::string query        = stringutil::get_formatted_string(confirmTemplate, itemName);

        ConfirmTask::create_push_fade(query, holdRequired, tasks::backup::delete_backup_remote, nullptr, m_dataStruct);
    }
}

void BackupMenuState::upload_backup()
{
    static constexpr int POP_TICKS = ui::PopMessageManager::DEFAULT_TICKS;

    // Grab pointer to remote. Don't continue if it's not valid.
    remote::Storage *remote = remote::get_remote_storage();
    if (error::is_null(remote)) { return; }

    // Grab index, entry. If isn't local, it can't be uploaded.
    const int selected     = sm_backupMenu->get_selected();
    const MenuEntry &entry = m_menuEntries[selected];
    if (entry.type != BackupMenuState::MenuEntryType::Local) { return; }

    // Our final target.
    fslib::Path target{m_directoryPath / m_directoryListing[entry.index]};

    // If it's a directory, pop and bail. Only zip can be uploaded because it's simpler that way.
    const bool isDir = fslib::directory_exists(target);
    if (isDir)
    {
        const char *popNotZip = strings::get_by_name(strings::names::BACKUPMENU_POPS, 13);
        ui::PopMessageManager::push_message(POP_TICKS, popNotZip);
        return;
    }

    // Move our final path to our data struct.
    m_dataStruct->path = std::move(target);

    // Grab the name of the target. If it already exists, patch. If not, new upload.
    const std::string_view itemName = m_dataStruct->path.get_filename();
    const bool exists               = remote->file_exists(itemName);
    if (exists)
    {
        // This is needed, since we're patching and overwriting something.
        const bool holdRequired = config::get_by_key(config::keys::HOLD_FOR_OVERWRITE);

        // Get the target.
        remote::Item *remoteItem = remote->get_file_by_name(itemName);
        const char *queryFormat  = strings::get_by_name(strings::names::BACKUPMENU_CONFS, 0);
        const char *itemName     = remoteItem->get_name().data();

        // Confirmation string.
        std::string query = stringutil::get_formatted_string(queryFormat, itemName);

        // Push the confirmation.
        ConfirmProgress::create_push_fade(query, holdRequired, tasks::backup::patch_backup, nullptr, m_dataStruct);
    }
    else
    {
        ProgressState::create_push_fade(tasks::backup::upload_backup, m_dataStruct);
    }
}

void BackupMenuState::pop_save_empty()
{
    const int ticks      = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popEmpty = strings::get_by_name(strings::names::BACKUPMENU_POPS, 0);
    ui::PopMessageManager::push_message(ticks, popEmpty);
}

void BackupMenuState::deactivate_state()
{
    sm_slidePanel->clear_elements();
    sm_slidePanel->reset();
    sm_backupMenu->reset();

    remote::Storage *remote = remote::get_remote_storage();
    if (remote) { remote->return_to_root(); }

    BaseState::deactivate();
}
