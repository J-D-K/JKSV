#include "appstates/ExtrasMenuState.hpp"

#include "appstates/FileModeState.hpp"
#include "appstates/MainMenuState.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "graphics/ScopedRender.hpp"
#include "graphics/colors.hpp"
#include "graphics/targets.hpp"
#include "keyboard/keyboard.hpp"
#include "strings/strings.hpp"
#include "ui/PopMessageManager.hpp"

#include <string_view>

namespace
{
    // Enum for switch case readability.
    enum
    {
        REINIT_DATA,
        SD_TO_SD_BROWSER,
        BIS_PRODINFO_F,
        BIS_SAFE,
        BIS_SYSTEM,
        BIS_USER,
        TERMINATE_PROCESS
    };
} // namespace

// Definition at bottom.
static void finish_reinitialization();

//                      ---- Construction ----

ExtrasMenuState::ExtrasMenuState(sdl2::Renderer &renderer)
    : m_renderer(renderer)
    , m_renderTarget(sdl2::TextureManager::create_load_resource(graphics::targets::names::SECONDARY,
                                                                graphics::targets::dims::SECONDARY_WIDTH,
                                                                graphics::targets::dims::SECONDARY_HEIGHT,
                                                                SDL_TEXTUREACCESS_TARGET))
    , m_controlGuide(ui::ControlGuide::create(strings::get_by_name(strings::names::CONTROL_GUIDES, 5)))
{ ExtrasMenuState::initialize_menu(); }

//                      ---- Public functions ----

void ExtrasMenuState::update(const sdl2::Input &input)
{
    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input.button_pressed(HidNpadButton_A);
    const bool bPressed = input.button_pressed(HidNpadButton_B);

    m_extrasMenu->update(input, hasFocus);
    m_controlGuide->update(input, hasFocus);

    if (aPressed)
    {
        switch (m_extrasMenu->get_selected())
        {
            case REINIT_DATA:       ExtrasMenuState::reinitialize_data(); break;
            case SD_TO_SD_BROWSER:  ExtrasMenuState::sd_to_sd_browser(); break;
            case BIS_PRODINFO_F:    ExtrasMenuState::prodinfof_to_sd(); break;
            case BIS_SAFE:          ExtrasMenuState::safe_to_sd(); break;
            case BIS_SYSTEM:        ExtrasMenuState::system_to_sd(); break;
            case BIS_USER:          ExtrasMenuState::user_to_sd(); break;
            case TERMINATE_PROCESS: ExtrasMenuState::terminate_process(); break;
        }
    }
    else if (bPressed) { BaseState::deactivate(); }
}

void ExtrasMenuState::sub_update() { m_controlGuide->sub_update(); }

void ExtrasMenuState::render(sdl2::Renderer &renderer)
{
    const bool hasFocus = BaseState::has_focus();

    { // Set the render target.
        graphics::ScopedRender scopedRender{renderer, m_renderTarget};
        renderer.frame_begin(colors::TRANSPARENT);

        // Render the menu to it.
        m_extrasMenu->render(renderer, hasFocus);
    }

    m_renderTarget->render(201, 91);
    m_controlGuide->render(renderer, hasFocus);
}

//                      ---- Private functions ----

void ExtrasMenuState::initialize_menu()
{
    if (!m_extrasMenu) { m_extrasMenu = ui::Menu::create(32, 10, 1000, 23, 555); }

    for (int i = 0; const char *option = strings::get_by_name(strings::names::EXTRASMENU_MENU, i); i++)
    {
        m_extrasMenu->add_option(option);
    }
}

void ExtrasMenuState::reinitialize_data() { data::launch_initialization(true, m_renderer, finish_reinitialization); }

void ExtrasMenuState::sd_to_sd_browser() { FileModeState::create_and_push("sdmc", "sdmc", false); }

void ExtrasMenuState::prodinfof_to_sd()
{
    const bool mountError = error::fslib(fslib::open_bis_filesystem("prodinfo-f", FsBisPartitionId_CalibrationFile));
    if (mountError) { return; }

    FileModeState::create_and_push("prodinfo-f", "sdmc", false, true);
}

void ExtrasMenuState::safe_to_sd()
{
    const bool mountError = error::fslib(fslib::open_bis_filesystem("safe", FsBisPartitionId_SafeMode));
    if (mountError) { return; }

    FileModeState::create_and_push("safe", "sdmc", false, true);
}

void ExtrasMenuState::system_to_sd()
{
    const bool mountError = error::fslib(fslib::open_bis_filesystem("system", FsBisPartitionId_System));
    if (mountError) { return; }

    FileModeState::create_and_push("system", "sdmc", false, true);
}

void ExtrasMenuState::user_to_sd()
{
    const bool mountError = error::fslib(fslib::open_bis_filesystem("user", FsBisPartitionId_User));
    if (mountError) { return; }

    FileModeState::create_and_push("user", "sdmc", false, true);
}

void ExtrasMenuState::terminate_process() {}

static void finish_reinitialization()
{
    const int popTicks     = ui::PopMessageManager::DEFAULT_TICKS;
    const char *popSuccess = strings::get_by_name(strings::names::EXTRASMENU_POPS, 0);

    MainMenuState::refresh_view_states();
    ui::PopMessageManager::push_message(popTicks, popSuccess);
}
