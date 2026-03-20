#include "JKSV.hpp"

#include "StateManager.hpp"
#include "appstates/FileModeState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TaskState.hpp"
#include "builddate.hpp"
#include "config/config.hpp"
#include "curl/curl.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "fslib.hpp"
#include "graphics/colors.hpp"
#include "graphics/fonts.hpp"
#include "graphics/screen.hpp"
#include "logging/logger.hpp"
#include "remote/remote.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "sys/sys.hpp"
#include "ui/PopMessageManager.hpp"

#include <chrono>
#include <switch.h>
#include <thread>

// Normally I try to avoid C macros in C++, but this cleans stuff up nicely.
#define ABORT_ON_FAILURE(x)                                                                                                    \
    if (!x) { return; }

namespace
{
    /// @brief Config for socket.
    constexpr SocketInitConfig SOCKET_INIT_CONFIG = {.tcp_tx_buf_size     = 0x20000,
                                                     .tcp_rx_buf_size     = 0x20000,
                                                     .tcp_tx_buf_max_size = 0x80000,
                                                     .tcp_rx_buf_max_size = 0x80000,
                                                     .udp_tx_buf_size     = 0x2400,
                                                     .udp_rx_buf_size     = 0xA500,
                                                     .sb_efficiency       = 8,
                                                     .num_bsd_sessions    = 3,
                                                     .bsd_service_type    = BsdServiceType_User};

} // namespace

// This function allows any service init to be logged with its name without repeating the code.
template <typename... Args>
static bool initialize_service(Result (*function)(Args...), const char *serviceName, Args &&...args)
{
    Result error = (*function)(std::forward<Args>(args)...);
    if (R_FAILED(error))
    {
        logger::log("Error initializing %s: 0x%X.", serviceName, error);
        return false;
    }
    return true;
}

//                      ---- Construction ----

JKSV::JKSV()
    : m_sdl2()
    , m_window(graphics::SCREEN_WIDTH, graphics::SCREEN_HEIGHT)
    , m_renderer(m_window)
    , m_audio()
    , m_input()
{
    // Ensure SDL initialized correctly. If not, return.
    if (!m_sdl2.is_initialized() || !m_window.is_initialized() || !m_renderer.is_initialized()) { return; }

    // Set boost mode first.
    JKSV::set_boost_mode();

    // Nothing in JKSV can really continue without these.
    ABORT_ON_FAILURE(JKSV::initialize_services());
    ABORT_ON_FAILURE(JKSV::initialize_filesystem());

    // Create the log file if it hasn't been already.
    logger::initialize();

    // SDL2
    ABORT_ON_FAILURE(JKSV::initialize_sdl());

    // Curl.
    ABORT_ON_FAILURE(curl::initialize());

    // Config and input.
    config::initialize();

    // These are the strings used in the UI.
    ABORT_ON_FAILURE(strings::initialize()); // This is fatal now.

    JKSV::setup_translation_info_strings();

    // This needs the config init'd or read to work.
    JKSV::create_directories();
    sys::threadpool::initialize(); // This is the thread pool so JKSV isn't constantly creating and destroying threads.

    // Push the remote init.
    sys::threadpool::push_job(remote::initialize, nullptr);

    // Launch the loading init. Finish init is called afterwards.
    auto init_finish = [&]() { MainMenuState::create_and_push(m_renderer); }; // Lambda that's exec'd after state is finished.
    data::launch_initialization(false, m_renderer, init_finish);

    // This isn't required, but why not?
    FadeState::create_and_push(colors::BLACK, 0xFF, 0x00, nullptr);

    // Push this warning so people can't complain if JKSV runs out of RAM.
    JKSV::applet_mode_warning();

    // JKSV is now running.
    sm_isRunning = true;
}

//                      ---- Destruction ----

JKSV::~JKSV()
{
    sys::threadpool::exit();
    config::save();
    curl::exit();
    JKSV::exit_services();

    appletSetCpuBoostMode(ApmCpuBoostMode_Normal);
    appletUnlockExit();
}

//                      ---- Public functions ----

bool JKSV::is_running() const noexcept { return sm_isRunning && appletMainLoop(); }

void JKSV::update()
{
    // Update the input instance.
    m_input.update();

    // Check if we should close.
    const bool plusPressed = m_input.button_pressed(HidNpadButton_Plus);
    const bool isClosable  = StateManager::back_is_closable();
    if (plusPressed && isClosable) { sm_isRunning = false; }

    // Update states and pop-ups.
    StateManager::update(m_input);
    ui::PopMessageManager::update();
}

void JKSV::render()
{
    // Set target to frame buffer and clear.
    m_renderer.frame_begin(colors::CLEAR_COLOR);

    // Render the base.
    JKSV::render_base();

    // Render the states.
    StateManager::render(m_renderer);

    // Render pop-ups.
    ui::PopMessageManager::render(m_renderer);

    // Present to screen.
    m_renderer.frame_end();
}

void JKSV::request_quit() noexcept { sm_isRunning = false; }

//                      ---- Private functions ----

void JKSV::set_boost_mode()
{
    // Log for errors, but not fatal.
    error::libnx(appletSetCpuBoostMode(ApmCpuBoostMode_FastLoad));
}

bool JKSV::initialize_filesystem()
{
    // This needs to be in this specific order
    const bool fslib    = fslib::is_initialized();
    const bool romfs    = initialize_service(romfsInit, "RomFS");
    const bool fslibDev = fslib && fslib::dev::initialize_sdmc();
    if (!fslib || !romfs || !fslibDev) { return false; }

    return true;
}

bool JKSV::initialize_services()
{
    // This looks cursed, but it works.
    bool serviceInit = initialize_service(accountInitialize, "Account", AccountServiceType_Administrator);
    serviceInit      = serviceInit && initialize_service(nsInitialize, "NS");
    serviceInit      = serviceInit && initialize_service(pdmqryInitialize, "PDMQry");
    serviceInit      = serviceInit && initialize_service(plInitialize, "PL", PlServiceType_User);
    serviceInit      = serviceInit && initialize_service(pmshellInitialize, "PMShell");
    serviceInit      = serviceInit && initialize_service(setInitialize, "Set");
    serviceInit      = serviceInit && initialize_service(setsysInitialize, "SetSys");
    serviceInit      = serviceInit && initialize_service(socketInitialize, "Socket", &SOCKET_INIT_CONFIG);
    serviceInit      = serviceInit && initialize_service(nifmInitialize, "NIFM", NifmServiceType_User);
    serviceInit      = serviceInit && initialize_service(appletLockExit, "AppletLockExit");
    return serviceInit;
}

bool JKSV::initialize_sdl()
{
    // These are for loading the header icon.
    static constexpr std::string_view HEADER_PATH = "romfs:/Textures/HeaderIcon.png";

    // These are our breakpoints for wrapping lines.
    static constexpr std::array<uint32_t, 7> BREAKPOINTS = {L' ', L'　', L'/', L'_', L'-', L'。', L'、'};

    // These are our color changing points.
    static constexpr std::array<std::pair<uint32_t, SDL_Color>, 7> COLOR_POINTS = {{{L'#', colors::BLUE},
                                                                                    {L'*', colors::DARK_RED},
                                                                                    {L'<', colors::YELLOW},
                                                                                    {L'>', colors::GREEN},
                                                                                    {L'`', colors::BLUE_GREEN},
                                                                                    {L'^', colors::PINK},
                                                                                    {L'$', colors::GOLD}}};

    // Ensure SDL2 textures and sounds are ready to use.
    sdl2::Texture::initialize(m_renderer);
    sdl2::Sound::initialize(m_audio);

    // Register break and color points.
    sdl2::Font::add_break_points(BREAKPOINTS);
    sdl2::Font::add_color_points(COLOR_POINTS);

    // Load our fonts.
    m_titleFont = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::THIRTY_FOUR_PIXEL,
                                                                            graphics::fonts::sizes::THIRTY_FOUR_PIXEL);
    m_buildFont = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::FOURTEEN_PIXEL,
                                                                            graphics::fonts::sizes::FOURTEEN_PIXEL);

    // Load the icon in the top left.
    m_headerIcon = sdl2::TextureManager::create_load_resource(HEADER_PATH, HEADER_PATH);
    if (!m_headerIcon) { return false; }

    return true;
}

bool JKSV::create_directories()
{
    // Working directory creation.
    const fslib::Path workDir{config::get_working_directory()};
    const bool needsWorkDir   = !fslib::directory_exists(workDir);
    const bool workDirCreated = needsWorkDir && fslib::create_directories_recursively(workDir);
    if (needsWorkDir && !workDirCreated) { return false; }

    // Trash folder. This can fail without being fatal.
    const fslib::Path trashDir{workDir / "_TRASH_"};
    const bool trashEnabled = config::get_by_key(config::keys::ENABLE_TRASH_BIN);
    const bool needsTash    = trashEnabled && !fslib::directory_exists(trashDir);
    if (needsTash) { error::fslib(fslib::create_directory(trashDir)); }

    return true;
}

void JKSV::setup_translation_info_strings()
{
    const char *translationFormat = strings::get_by_name(strings::names::TRANSLATION, 0);
    const char *author            = strings::get_by_name(strings::names::TRANSLATION, 1);
    m_showTranslationInfo         = std::char_traits<char>::compare(author, "NULL", 4) != 0; // This is whether or not to show.
    m_translationInfo             = stringutil::get_formatted_string(translationFormat, author);
    m_buildString = stringutil::get_formatted_string("v. %02d.%02d.%04d", builddate::MONTH, builddate::DAY, builddate::YEAR);
}

void JKSV::applet_mode_warning() noexcept
{
    // This hangs longer than other pop messages.
    static constexpr int APPLET_TICKS = 5000;

    // Anything that doesn't register as an application is an applet as far as I'm concerned.
    const bool isApplet = appletGetAppletType() != AppletType_Application;
    if (!isApplet) { return; }

    // Get the string and push the pop message.
    const char *appletString = strings::get_by_name(strings::names::APPLET_MODE, 0);
    ui::PopMessageManager::push_message(APPLET_TICKS, appletString);
}

void JKSV::render_base()
{
    // These are the same for both.
    static constexpr int LINE_X_BEGIN = 30;
    static constexpr int LINE_X_END   = 1250;

    // These are the Y's.
    static constexpr int LINE_A_Y = 88;
    static constexpr int LINE_B_Y = 648;

    // Coordinates for the header icon.
    static constexpr int HEADER_X = 66;
    static constexpr int HEADER_Y = 27;

    // Coordinates for the title text.
    static constexpr int TITLE_X = 130;
    static constexpr int TITLE_Y = 32;

    // Coordinates for the translation info and build date.
    static constexpr int BUILD_X = 8;
    static constexpr int BUILD_Y = 700;
    static constexpr int TRANS_Y = 680;

    // This is just the JKSV string.
    static constexpr std::string_view TITLE_TEXT = "JKSV";

    // Top and bottom framing lines.
    m_renderer.render_line(LINE_X_BEGIN, LINE_A_Y, LINE_X_END, LINE_A_Y, colors::WHITE);
    m_renderer.render_line(LINE_X_BEGIN, LINE_B_Y, LINE_X_END, LINE_B_Y, colors::WHITE);

    // Icon
    m_headerIcon->render(HEADER_X, HEADER_Y);

    // "JKSV"
    m_titleFont->render_text(TITLE_X, TITLE_Y, colors::WHITE, TITLE_TEXT);

    // Translation info in bottom left.
    if (m_showTranslationInfo) { m_buildFont->render_text(BUILD_X, TRANS_Y, colors::WHITE, m_translationInfo); }

    // Build date
    m_buildFont->render_text(BUILD_X, BUILD_Y, colors::WHITE, m_buildString);
}

void JKSV::exit_services()
{
    nifmExit();
    socketExit();
    setsysExit();
    setExit();
    pmshellExit();
    plExit();
    pdmqryExit();
    nsExit();
    accountExit();
}
