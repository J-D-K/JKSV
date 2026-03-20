#include "appstates/FileModeState.hpp"

#include "appstates/FileOptionState.hpp"
#include "config/config.hpp"
#include "graphics/ScopedRender.hpp"
#include "graphics/colors.hpp"
#include "graphics/screen.hpp"
#include "logging/logger.hpp"
#include "mathutil.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"

#include <cmath>

namespace
{
    /// @brief This is the X position of the file mode pop up.
    constexpr int PERMA_X = 15;

    /// @brief This is the target Y of the pop-up. The starting Y is the height of the screen.
    constexpr int TARGET_Y = 91;
}

//                      ---- Construction ----

FileModeState::FileModeState(std::string_view mountA, std::string_view mountB, int64_t journalSize, bool isSystem)
    : m_mountA(mountA)
    , m_mountB(mountB)
    , m_isSystem(isSystem)
    , m_allowSystem(config::get_by_key(config::keys::ALLOW_WRITING_TO_SYSTEM))
    , m_journalSize(journalSize)
    , m_transition(PERMA_X, graphics::SCREEN_HEIGHT, 0, 0, PERMA_X, TARGET_Y, 0, 0, ui::Transition::DEFAULT_THRESHOLD)
    , m_state(State::Rising)
{
    FileModeState::initialize_static_members();
    FileModeState::initialize_paths();
    FileModeState::initialize_menus();
}

//                      ---- Public functions ----

void FileModeState::update(const sdl2::Input &input)
{
    switch (m_state)
    {
        case State::Rising:
        case State::Dropping: FileModeState::update_y(); break;
        case State::Open:     FileModeState::update_handle_input(input); break;
    }
}

void FileModeState::render(sdl2::Renderer &renderer)
{
    // Coords for divider lines.
    static constexpr int LINE_A_X = 617;
    static constexpr int LINE_B_X = 618;

    // Shared.
    static constexpr int LINE_Y_A = 0;
    static constexpr int LINE_Y_B = 538;

    // Grab focus.
    const bool hasFocus = BaseState::has_focus();

    // Switch targets, clear.
    {
        graphics::ScopedRender scopedRender{renderer, sm_renderTarget};
        renderer.frame_begin(colors::TRANSPARENT);

        // Divider lines & menus.
        renderer.render_line(LINE_A_X, LINE_Y_A, LINE_A_X, LINE_Y_B, colors::WHITE);
        renderer.render_line(LINE_B_X, LINE_Y_A, LINE_B_X, LINE_Y_B, colors::DIALOG_DARK);
        m_dirMenuA->render(renderer, hasFocus && m_target == Target::MountA);
        m_dirMenuB->render(renderer, hasFocus && m_target == Target::MountB);
    }

    // This needs to be in this specific order to look right.
    sm_controlGuide->render(renderer, hasFocus);
    sm_frame->render(renderer, hasFocus);
    sm_renderTarget->render(23, m_transition.get_y() + 12);
}

//                      ---- Private functions ----

void FileModeState::initialize_static_members()
{
    // Frame coords and dimensions.
    static constexpr int FRAME_WIDTH  = 1250;
    static constexpr int FRAME_HEIGHT = 555;

    // Inner target coords and dimensions.
    static constexpr int INNER_WIDTH  = 1234;
    static constexpr int INNER_HEIGHT = 538;

    // This is the name of the render target for the main body.
    static constexpr std::string_view RENDER_TARGET_NAME = "FMRenderTarget";

    if (sm_frame && sm_renderTarget && sm_controlGuide) { return; }

    sm_frame = ui::Frame::create(PERMA_X, graphics::SCREEN_HEIGHT, FRAME_WIDTH, FRAME_HEIGHT);
    sm_renderTarget =
        sdl2::TextureManager::create_load_resource(RENDER_TARGET_NAME, INNER_WIDTH, INNER_HEIGHT, SDL_TEXTUREACCESS_TARGET);
    sm_controlGuide = ui::ControlGuide::create(strings::get_by_name(strings::names::CONTROL_GUIDES, 4));
}

void FileModeState::initialize_paths()
{
    m_pathA = m_mountA + ":/";
    m_pathB = m_mountB + ":/";
}

void FileModeState::initialize_menus()
{
    // Menu A.
    static constexpr int MENU_A_X = 8;

    // Menu B.
    static constexpr int MENU_B_X = 630;

    // Shared.
    static constexpr int MENU_Y             = 5;
    static constexpr int MENU_WIDTH         = 594;
    static constexpr int MENU_FONT_SIZE     = 20;
    static constexpr int MENU_TARGET_HEIGHT = 538;

    m_dirMenuA = ui::Menu::create(MENU_A_X, MENU_Y, MENU_WIDTH, MENU_FONT_SIZE, MENU_TARGET_HEIGHT);
    m_dirMenuB = ui::Menu::create(MENU_B_X, MENU_Y, MENU_WIDTH, MENU_FONT_SIZE, MENU_TARGET_HEIGHT);

    FileModeState::initialize_directory_menu(m_pathA, m_dirA, *m_dirMenuA.get());
    FileModeState::initialize_directory_menu(m_pathB, m_dirB, *m_dirMenuB.get());
}

void FileModeState::initialize_directory_menu(const fslib::Path &path, fslib::Directory &directory, ui::Menu &menu)
{
    static constexpr const char *DIR_PREFIX  = "[D] ";
    static constexpr const char *FILE_PREFIX = "[F] ";

    directory.open(path);
    if (!directory.is_open()) { return; }

    menu.reset();
    menu.add_option(".");
    menu.add_option("..");

    for (const fslib::DirectoryEntry &entry : directory)
    {
        std::string option{};
        if (entry.is_directory()) { option = DIR_PREFIX; }
        else
        {
            option = FILE_PREFIX;
        }

        option += entry.get_filename();
        menu.add_option(option);
    }
}

void FileModeState::update_y() noexcept
{
    // Update the transition.
    m_transition.update();

    // Grab the Y and update the frame.
    const int y = m_transition.get_y();
    sm_frame->set_y(y);

    // Conditions for changing to next state.
    const bool finishedRising   = m_state == State::Rising && m_transition.in_place_xy();
    const bool finishedDropping = m_state == State::Dropping && m_transition.in_place_xy();
    if (finishedRising) { m_state = State::Open; }
    else if (finishedDropping) { FileModeState::deactivate_state(); }
}

void FileModeState::update_handle_input(const sdl2::Input &input) noexcept
{
    // Get whether or not the state has focus.
    const bool hasFocus = BaseState::has_focus();

    // Grab references to the current target we're working with.
    ui::Menu &menu              = FileModeState::get_source_menu();
    fslib::Path &path           = FileModeState::get_source_path();
    fslib::Directory &directory = FileModeState::get_source_directory();

    // Input bools.
    const bool aPressed     = input.button_pressed(HidNpadButton_A);
    const bool bPressed     = input.button_pressed(HidNpadButton_B);
    const bool xPressed     = input.button_pressed(HidNpadButton_X);
    const bool zlZRPressed  = input.button_pressed(HidNpadButton_ZL) || input.button_pressed(HidNpadButton_ZR);
    const bool minusPressed = input.button_pressed(HidNpadButton_Minus);

    // Conditions
    if (aPressed) { FileModeState::enter_selected(path, directory, menu); }
    else if (bPressed) { FileModeState::up_one_directory(path, directory, menu); }
    else if (xPressed) { FileModeState::open_option_menu(directory, menu); }
    else if (zlZRPressed) { FileModeState::change_target(); }
    else if (minusPressed)
    {
        m_state = State::Dropping;
        m_transition.set_target_y(graphics::SCREEN_HEIGHT);
    }

    // Update the menu and control guide.
    menu.update(input, hasFocus);
    sm_controlGuide->update(input, hasFocus);
}

void FileModeState::enter_selected(fslib::Path &path, fslib::Directory &directory, ui::Menu &menu)
{
    const int selected = menu.get_selected();

    switch (selected)
    {
        case 0: return;
        case 1: FileModeState::up_one_directory(path, directory, menu); break;
        default:
        {
            const int dirIndex                 = selected - 2;
            const fslib::DirectoryEntry &entry = directory[dirIndex];
            if (entry.is_directory()) { FileModeState::enter_directory(path, directory, menu, entry); }
        }
        break;
    }
}

void FileModeState::open_option_menu(fslib::Directory &directory, ui::Menu &menu)
{
    const int selected = menu.get_selected();

    // Don't push the menu if the '..' is highlighted.
    if (selected == 0 || selected > 1) { FileOptionState::create_and_push(this); }
}

void FileModeState::change_target() { m_target = m_target == Target::MountA ? Target::MountB : Target::MountA; }

void FileModeState::up_one_directory(fslib::Path &path, fslib::Directory &directory, ui::Menu &menu)
{
    if (FileModeState::path_is_root(path)) { return; }

    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == path.NOT_FOUND) { return; }
    else if (lastSlash <= 0) { lastSlash = 1; }

    fslib::Path newPath{path.sub_path(lastSlash)};
    path = std::move(newPath);
    FileModeState::initialize_directory_menu(path, directory, menu);
}

void FileModeState::enter_directory(fslib::Path &path,
                                    fslib::Directory &directory,
                                    ui::Menu &menu,
                                    const fslib::DirectoryEntry &entry)
{
    if (!entry.is_directory()) { return; }

    path /= entry.get_filename();
    FileModeState::initialize_directory_menu(path, directory, menu);
}

ui::Menu &FileModeState::get_source_menu() noexcept
{ return m_target == Target::MountA ? *m_dirMenuA.get() : *m_dirMenuB.get(); }

ui::Menu &FileModeState::get_destination_menu() noexcept
{ return m_target == Target::MountA ? *m_dirMenuB.get() : *m_dirMenuA.get(); }

fslib::Path &FileModeState::get_source_path() noexcept { return m_target == Target::MountA ? m_pathA : m_pathB; }

fslib::Path &FileModeState::get_destination_path() noexcept { return m_target == Target::MountA ? m_pathB : m_pathA; }

fslib::Directory &FileModeState::get_source_directory() noexcept { return m_target == Target::MountA ? m_dirA : m_dirB; }

fslib::Directory &FileModeState::get_destination_directory() noexcept { return m_target == Target::MountA ? m_dirB : m_dirA; }

void FileModeState::deactivate_state() noexcept
{
    // This should be already set to this, but just to be sure.
    sm_frame->set_y(graphics::SCREEN_HEIGHT);

    // Close both mount points.
    fslib::close_file_system(m_mountA);
    fslib::close_file_system(m_mountB);

    // Reset the control guide.
    sm_controlGuide->reset();

    // Mark the state for deletion.
    BaseState::deactivate();
}
