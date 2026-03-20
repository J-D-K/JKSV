#include "appstates/FileOptionState.hpp"

#include "appstates/ConfirmState.hpp"
#include "appstates/MessageState.hpp"
#include "appstates/ProgressState.hpp"
#include "appstates/TaskState.hpp"
#include "config/config.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "keyboard/keyboard.hpp"
#include "logging/logger.hpp"
#include "mathutil.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "sys/OpTimer.hpp"
#include "tasks/fileoptions.hpp"

#include <ctime>

namespace
{
    enum
    {
        COPY,
        DELETE,
        RENAME,
        CREATE_DIR,
        PROPERTIES,
        CLOSE
    };

    // X coordinates
    constexpr int LEFT_X  = 200;
    constexpr int RIGHT_X = 840;

    // Y coordinate.
    constexpr int TOP_Y = 218;

    // Initial width and height.
    constexpr int INITIAL_WIDTH_HEIGHT = 32;
    // Target width and height.
    constexpr int TARGET_WIDTH_HEIGHT = 256;
}

// Defined at bottom.
static std::string get_size_string(int64_t totalSize);

//                      ---- Construction ----

FileOptionState::FileOptionState(FileModeState *spawningState)
    : m_spawningState(spawningState)
    , m_target(spawningState->m_target)
    , m_state(State::Opening)
    , m_transition(m_target == FileModeState::Target::MountA ? LEFT_X : RIGHT_X,
                   0,
                   INITIAL_WIDTH_HEIGHT,
                   INITIAL_WIDTH_HEIGHT,
                   m_target == FileModeState::Target::MountA ? LEFT_X : RIGHT_X,
                   0,
                   TARGET_WIDTH_HEIGHT,
                   TARGET_WIDTH_HEIGHT,
                   ui::Transition::DEFAULT_THRESHOLD)
    , m_dataStruct(std::make_shared<FileOptionState::DataStruct>())
{
    FileOptionState::initialize_static_members();
    FileOptionState::initialize_data_struct();
}

//                      ---- Public functions ----

void FileOptionState::update(const sdl2::Input &input)
{
    switch (m_state)
    {
        case State::Opening:
        case State::Closing: FileOptionState::update_dimensions(); break;
        case State::Opened:  FileOptionState::update_handle_input(input); break;
    }
}

void FileOptionState::render(sdl2::Renderer &renderer)
{
    // Grab focus.
    const bool hasFocus = BaseState::has_focus();

    // Render the dialog and bail if we're not read for the menu.
    sm_dialog->render(renderer, hasFocus);
    if (!m_transition.in_place()) { return; }

    sm_copyMenu->render(renderer, hasFocus);
}

void FileOptionState::update_source() { m_updateSource = true; }

void FileOptionState::update_destination() { m_updateDest = true; }

//                      ---- Private functions ----

void FileOptionState::initialize_static_members()
{
    // Menu coords.
    static constexpr int MENU_Y         = 253;
    static constexpr int MENU_WIDTH     = 234;
    static constexpr int MENU_FONT_SIZE = 20;

    // Dialog
    static constexpr int DIALOG_X            = 2000;
    static constexpr int DIALOG_Y            = 1000;
    static constexpr int DIALOG_WIDTH_HEIGHT = 32;

    // If these are already allocated, just reset.
    if (sm_copyMenu && sm_dialog)
    {
        const int x       = m_transition.get_x();
        const int dialogX = x + (128 - 16);

        sm_dialog->set_x(dialogX);
        sm_copyMenu->set_x(x + 9);
        return;
    }

    sm_copyMenu = ui::Menu::create(m_target == FileModeState::Target::MountA ? LEFT_X + 9 : RIGHT_X + 9,
                                   MENU_Y,
                                   MENU_WIDTH,
                                   MENU_FONT_SIZE,
                                   graphics::SCREEN_HEIGHT); // Target height is a workaround.
    sm_dialog =
        ui::DialogBox::create(DIALOG_X, DIALOG_Y, DIALOG_WIDTH_HEIGHT, DIALOG_WIDTH_HEIGHT); // Create this off screen at first.

    // This never changes, so...
    const char *menuOption = nullptr;
    for (int i = 0; (menuOption = strings::get_by_name(strings::names::FILEOPTION_MENU, i)); i++)
    {
        sm_copyMenu->add_option(menuOption);
    }
}

void FileOptionState::initialize_data_struct() { m_dataStruct->spawningState = this; }

void FileOptionState::update_dimensions() noexcept
{
    // Update transition.
    m_transition.update();

    // Grab and calculate updated coords.
    const int width = m_transition.get_width();
    const int x     = (m_transition.get_x() + 128) - (width / 2);

    // Update the dialog. Use the transition and then manually override the x.
    sm_dialog->set_from_transition(m_transition, true);
    sm_dialog->set_x(x);

    // Conditions for state shifting.
    const bool isOpened = m_state == State::Opening && m_transition.in_place();
    const bool isClosed = m_state == State::Closing && m_transition.in_place();
    if (isOpened) { m_state = State::Opened; }
    else if (isClosed) { FileOptionState::deactivate_state(); }
}

void FileOptionState::update_handle_input(const sdl2::Input &input) noexcept
{
    // Grab whether or not the state has focus since this is actually used a lot.
    const bool hasFocus = BaseState::has_focus();

    // Check if we need to update the source or dest.
    if (m_updateSource) { FileOptionState::update_filemode_source(); }
    else if (m_updateDest) { FileOptionState::update_filemode_dest(); }

    // Update the menu input.
    sm_copyMenu->update(input, hasFocus);

    // Local input handling.
    const int selected  = sm_copyMenu->get_selected();
    const bool aPressed = input.button_pressed(HidNpadButton_A);
    const bool bPressed = input.button_pressed(HidNpadButton_B);

    if (aPressed)
    {
        switch (selected)
        {
            case COPY:       FileOptionState::copy_target(); break;
            case DELETE:     FileOptionState::delete_target(); break;
            case RENAME:     FileOptionState::rename_target(); break;
            case CREATE_DIR: FileOptionState::create_directory(); break;
            case PROPERTIES: FileOptionState::get_show_target_properties(); break;
            case CLOSE:      FileOptionState::close_dialog(); break;
            default:         break; // This should never happen.
        }
    }
    else if (bPressed) { FileOptionState::close_dialog(); }
}

void FileOptionState::update_filemode_source()
{
    const fslib::Path &sourcePath = m_spawningState->get_source_path();
    fslib::Directory &sourceDir   = m_spawningState->get_source_directory();
    ui::Menu &sourceMenu          = m_spawningState->get_source_menu();

    m_spawningState->initialize_directory_menu(sourcePath, sourceDir, sourceMenu);
    m_updateSource = false;
}

void FileOptionState::update_filemode_dest()
{
    const fslib::Path &destPath = m_spawningState->get_destination_path();
    fslib::Directory &destDir   = m_spawningState->get_destination_directory();
    ui::Menu &destMenu          = m_spawningState->get_destination_menu();

    m_spawningState->initialize_directory_menu(destPath, destDir, destMenu);
    m_updateDest = false;
}

void FileOptionState::copy_target()
{
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;

    if (FileOptionState::system_write_check())
    {
        FileOptionState::pop_system_error();
        return;
    }

    const int64_t journalSize = m_spawningState->m_journalSize;

    const fslib::Path &sourcePath     = m_spawningState->get_source_path();
    const fslib::Directory &sourceDir = m_spawningState->get_source_directory();
    const ui::Menu &sourceMenu        = m_spawningState->get_source_menu();

    const fslib::Path &destPath     = m_spawningState->get_destination_path();
    const fslib::Directory &destDir = m_spawningState->get_destination_directory();
    const ui::Menu &destMenu        = m_spawningState->get_destination_menu();

    const int sourceSelected = sourceMenu.get_selected();
    const int destSelected   = destMenu.get_selected();

    const int sourceIndex = sourceSelected - 2;
    const int destIndex   = destSelected - 2;

    fslib::Path fullSource{sourcePath};
    if (sourceSelected > 1) { fullSource /= sourceDir[sourceIndex]; }

    fslib::Path fullDest{destPath};
    if (destSelected == 0 && sourceSelected > 1) { fullDest /= sourceDir[sourceIndex]; }
    if (destSelected > 1)
    {
        const fslib::DirectoryEntry &entry = destDir[destIndex];
        if (!entry.is_directory())
        {
            const char *errorFormat = strings::get_by_name(strings::names::FILEOPTION_POPS, 4);
            std::string pop         = stringutil::get_formatted_string(errorFormat, entry.get_filename());
            ui::PopMessageManager::push_message(popTicks, pop);
            return;
        }

        fullDest /= entry;
        if (sourceSelected > 1) { fullDest /= sourceDir[sourceIndex]; }
    }

    // Reminder: JK, you move these. That's why the string is blank if they're declared past this point.
    const std::string sourceString = fullSource.string();
    const std::string destString   = fullDest.string();
    m_dataStruct->sourcePath       = std::move(fullSource);
    m_dataStruct->destPath         = std::move(fullDest);
    m_dataStruct->journalSize      = journalSize;

    const char *copyFormat  = strings::get_by_name(strings::names::FILEOPTION_CONFS, 0);
    const std::string query = stringutil::get_formatted_string(copyFormat, sourceString.c_str(), destString.c_str());

    ConfirmProgress::create_push_fade(query, false, tasks::fileoptions::copy_source_to_destination, nullptr, m_dataStruct);
}

void FileOptionState::delete_target()
{
    if (FileOptionState::system_operation_check())
    {
        FileOptionState::pop_system_error();
        return;
    }

    const fslib::Path &targetPath     = m_spawningState->get_source_path();
    const fslib::Directory &targetDir = m_spawningState->get_source_directory();
    const ui::Menu &targetMenu        = m_spawningState->get_source_menu();

    fslib::Path fullTarget{targetPath};
    const int selected = targetMenu.get_selected();
    if (selected == 0) { return; } // I have no way to handle this right now.
    else if (selected > 1)
    {
        const int dirIndex = selected - 2;
        fullTarget /= targetDir[dirIndex];
    }

    const bool holdRequired  = config::get_by_key(config::keys::HOLD_FOR_DELETION);
    const char *deleteFormat = strings::get_by_name(strings::names::FILEOPTION_CONFS, 1);
    const std::string query  = stringutil::get_formatted_string(deleteFormat, fullTarget.string().c_str());

    m_dataStruct->sourcePath  = std::move(fullTarget);
    m_dataStruct->journalSize = m_spawningState->m_journalSize;

    ConfirmTask::create_push_fade(query, holdRequired, tasks::fileoptions::delete_target, nullptr, m_dataStruct);
}

void FileOptionState::rename_target()
{
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;

    if (FileOptionState::system_operation_check())
    {
        FileOptionState::pop_system_error();
        return;
    }

    const fslib::Path &targetPath = m_spawningState->get_source_path();
    fslib::Directory &targetDir   = m_spawningState->get_source_directory();
    ui::Menu &targetMenu          = m_spawningState->get_source_menu();
    const int selected            = targetMenu.get_selected();
    if (selected < 2) { return; }

    char nameBuffer[FS_MAX_PATH]     = {0};
    const int dirIndex               = selected - 2;
    const char *filename             = targetDir[dirIndex].get_filename();
    const char *keyboardFormat       = strings::get_by_name(strings::names::KEYBOARD, 9);
    const std::string keyboardHeader = stringutil::get_formatted_string(keyboardFormat, filename);
    const bool validInput            = keyboard::get_input(SwkbdType_QWERTY, filename, keyboardHeader, nameBuffer, FS_MAX_PATH);
    if (!validInput) { return; }

    const fslib::Path oldPath{targetPath / filename};
    const fslib::Path newPath{targetPath / nameBuffer};

    const std::string oldString = oldPath.string();
    const std::string newString = newPath.string();

    // If this is false and there's a journaling size set, we need to commit on renaming for it to stick.
    const bool isSource       = m_target == FileModeState::Target::MountA;
    const int64_t journalSize = m_spawningState->m_journalSize;
    const bool commitNeeded   = isSource && journalSize > 0;

    const bool isDir       = fslib::directory_exists(oldPath);
    const bool dirError    = isDir && error::fslib(fslib::rename_directory(oldPath, newPath));
    const bool fileError   = !isDir && error::fslib(fslib::rename_file(oldPath, newPath));
    const bool commitError = commitNeeded && error::fslib(fslib::commit_data_to_file_system(oldPath.get_device_name()));
    if (dirError && fileError && commitError)
    {
        const char *popFormat = strings::get_by_name(strings::names::FILEOPTION_POPS, 2);
        std::string pop       = stringutil::get_formatted_string(popFormat, filename);
        ui::PopMessageManager::push_message(popTicks, pop);
    }

    m_spawningState->initialize_directory_menu(targetPath, targetDir, targetMenu);
}

void FileOptionState::create_directory()
{
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;

    if (FileOptionState::system_operation_check())
    {
        FileOptionState::pop_system_error();
        return;
    }

    const fslib::Path &targetPath = m_spawningState->get_source_path();
    fslib::Directory &targetDir   = m_spawningState->get_source_directory();
    ui::Menu &targetMenu          = m_spawningState->get_source_menu();
    const int64_t journalSize     = m_spawningState->m_journalSize;

    char nameBuffer[FS_MAX_PATH] = {0};
    const char *keyboardHeader   = strings::get_by_name(strings::names::KEYBOARD, 6);
    const bool validInput        = keyboard::get_input(SwkbdType_QWERTY, {}, keyboardHeader, nameBuffer, FS_MAX_PATH);
    if (!validInput) { return; }

    const fslib::Path fullTarget{targetPath / nameBuffer};
    const bool commitRequired = journalSize > 0;
    const bool createError    = error::fslib(fslib::create_directory(fullTarget));
    const bool commitError    = !createError && error::fslib(fslib::commit_data_to_file_system(fullTarget.get_device_name()));
    if (createError || (commitRequired && commitError))
    {
        const char *popFormat = strings::get_by_name(strings::names::FILEOPTION_POPS, 3);
        std::string pop       = stringutil::get_formatted_string(popFormat, nameBuffer);
        ui::PopMessageManager::push_message(popTicks, pop);
    }

    m_spawningState->initialize_directory_menu(targetPath, targetDir, targetMenu);
}

void FileOptionState::get_show_target_properties()
{
    const fslib::Path &sourcePath     = m_spawningState->get_source_path();
    const fslib::Directory &sourceDir = m_spawningState->get_source_directory();
    const ui::Menu &sourceMenu        = m_spawningState->get_source_menu();

    fslib::Path targetPath{sourcePath};
    const int selected = sourceMenu.get_selected();
    if (selected > 1)
    {
        const int dirIndex = selected - 2;
        targetPath /= sourceDir[dirIndex];
    }

    const bool isDir = fslib::directory_exists(targetPath);
    if (isDir) { FileOptionState::get_show_directory_properties(targetPath); }
    else
    {
        FileOptionState::get_show_file_properties(targetPath);
    }
}

void FileOptionState::close_dialog() noexcept
{
    // Set the state.
    m_state = State::Closing;

    // Set the targets to their initial values.
    m_transition.set_target_width(INITIAL_WIDTH_HEIGHT);
    m_transition.set_target_height(INITIAL_WIDTH_HEIGHT);
}

void FileOptionState::get_show_directory_properties(const fslib::Path &path)
{
    int64_t subDirCount{};
    int64_t fileCount{};
    int64_t totalSize{};

    const bool getInfo = fs::get_directory_information(path, subDirCount, fileCount, totalSize);
    if (!getInfo) { return; }

    const char *messageFormat    = strings::get_by_name(strings::names::FILEOPTION_MESSAGES, 0);
    const std::string pathString = path.string(); // This is needed as backup incase of root directories.
    const std::string sizeString = get_size_string(totalSize);
    const std::string message =
        stringutil::get_formatted_string(messageFormat, pathString.c_str(), subDirCount, fileCount, sizeString.c_str());

    MessageState::create_and_push(message);
}

void FileOptionState::get_show_file_properties(const fslib::Path &path)
{

    static constexpr size_t BUFFER_SIZE = 0x40;

    FsTimeStampRaw timestamp{};
    const int64_t fileSize = fslib::get_file_size(path);
    error::fslib(fslib::get_file_timestamp(path, timestamp)); // Recorded, but not required. This doesn't work int saves.
    if (fileSize == -1) { return; }

    // I don't like this, but it's the easiest way to pull this off.
    const std::time_t created  = static_cast<std::time_t>(timestamp.created);
    const std::time_t modified = static_cast<std::time_t>(timestamp.modified);
    const std::time_t accessed = static_cast<std::time_t>(timestamp.accessed);

    const std::tm createdTm  = *std::localtime(&created);
    const std::tm modifiedTm = *std::localtime(&modified);
    const std::tm accessedTm = *std::localtime(&accessed);

    char createdBuffer[BUFFER_SIZE] = {0};
    char lastModified[BUFFER_SIZE]  = {0};
    char lastAccessed[BUFFER_SIZE]  = {0};

    std::strftime(createdBuffer, BUFFER_SIZE, "%c", &createdTm);
    std::strftime(lastModified, BUFFER_SIZE, "%c", &modifiedTm);
    std::strftime(lastAccessed, BUFFER_SIZE, "%c", &accessedTm);

    const char *messageFormat    = strings::get_by_name(strings::names::FILEOPTION_MESSAGES, 1);
    const std::string pathString = path.string();
    const std::string sizeString = get_size_string(fileSize);
    const std::string message    = stringutil::get_formatted_string(messageFormat,
                                                                    pathString.c_str(),
                                                                    sizeString.c_str(),
                                                                    createdBuffer,
                                                                    lastModified,
                                                                    lastAccessed);

    MessageState::create_and_push(message);
}

void FileOptionState::pop_system_error()
{
    const int popTicks = ui::PopMessageManager::DEFAULT_TICKS;
    const char *error  = strings::get_by_name(strings::names::FILEOPTION_POPS, 5);
    ui::PopMessageManager::push_message(popTicks, error);
}

void FileOptionState::deactivate_state()
{
    sm_copyMenu->set_selected(0);
    BaseState::deactivate();
}

//                      ---- Static definitions ----

static std::string get_size_string(int64_t totalSize)
{
    static constexpr int64_t THRESHOLD_BYTES = 0x400;
    static constexpr int64_t THRESHOLD_KB    = 0x100000;
    static constexpr int64_t THRESHOLD_MB    = 0x40000000;

    std::string sizeString{};
    if (totalSize > THRESHOLD_MB)
    {
        const double gigabytes = static_cast<double>(totalSize) / static_cast<double>(THRESHOLD_MB);
        sizeString             = stringutil::get_formatted_string("%.02f GB", gigabytes);
    }
    else if (totalSize < THRESHOLD_BYTES) { sizeString = stringutil::get_formatted_string("%lli bytes", totalSize); }
    else if (totalSize < THRESHOLD_KB)
    {
        const double kilobytes = static_cast<double>(totalSize) / static_cast<double>(THRESHOLD_BYTES);
        sizeString             = stringutil::get_formatted_string("%.02f KB", kilobytes);
    }
    else
    {
        const double megabytes = static_cast<double>(totalSize) / static_cast<double>(THRESHOLD_KB);
        sizeString             = stringutil::get_formatted_string("%.02f MB", megabytes);
    }

    return sizeString;
}
