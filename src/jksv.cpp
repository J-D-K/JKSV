#include <memory>
#include <vector>
#include "jksv.hpp"
#include "graphics/graphics.hpp"
#include "filesystem/filesystem.hpp"
#include "config.hpp"
#include "data/data.hpp"
#include "ui/ui.hpp"
#include "system/input.hpp"
#include "appStates/appState.hpp"
#include "appStates/mainMenuState.hpp"
#include "log.hpp"

// Things specifically only for here
namespace
{
    bool s_IsRunning = false;
    SDL_Texture *s_HeaderIcon = NULL;
    std::vector<std::unique_ptr<appState>> s_AppStateVector;
}

// Icon path
static const char *ICON_PATH = "romfs:/img/icn/iconWhite.png";

bool jksv::init(void)
{
    // Init logger
    logger::init();

    // Almost everything depends on graphics so first
    if (graphics::init("JKSV", 1280, 720) == false)
    {
        return false;
    }

    // Load the system font
    if (graphics::systemFont::init() == false)
    {
        return false;
    }

    // Config doesn't return anything
    config::init();

    // Filesystem
    if(fs::init() == false)
    {
        return false;
    }

    // ui just loads some things
    ui::init();

    // Load data from system
    if(data::init() == false)
    {
        return false;
    }

    // Input doesn't have anything to return.
    sys::input::init();

    // Load header icon
    s_HeaderIcon = graphics::textureLoadFromFile(TEXTURE_APP_ICON, ICON_PATH);

    // Push the first main view/state
    std::unique_ptr<appState> mainMenu = std::make_unique<mainMenuState>();
    jksv::pushNewState(mainMenu);

    // Everything is up and running
    s_IsRunning = true;

    return true;
}

void jksv::exit(void)
{
    config::exit();
    graphics::systemFont::exit();
    graphics::exit();
}

void jksv::update(void)
{
    // Update input
    sys::input::update();

    if (sys::input::buttonDown(HidNpadButton_Plus))
    {
        s_IsRunning = false;
    }

    // Clean up vector
    while(s_AppStateVector.back()->isActive() == false)
    {
        s_AppStateVector.pop_back();
        s_AppStateVector.back()->giveFocus();
    }

    // Update back of vector
    s_AppStateVector.back()->update();

    // Update pop up messages
    ui::popMessage::update();
}

void jksv::render(void)
{
    graphics::beginFrame(COLOR_DEFAULT_CLEAR);
    // Render the base
    graphics::renderLine(NULL, 30, 88, 1250, 88, COLOR_WHITE);
    graphics::renderLine(NULL, 30, 648, 1250, 648, COLOR_WHITE);
    graphics::textureRender(s_HeaderIcon, NULL, 66, 27);
    graphics::systemFont::renderText("JKSV", NULL, 130, 38, 24, COLOR_WHITE);

    // Render the state vector
    for (std::unique_ptr<appState> &a : s_AppStateVector)
    {
        a->render();
    }

    // Render pop up messages
    ui::popMessage::render();

    graphics::endFrame();
}

const bool jksv::isRunning(void)
{
    return s_IsRunning;
}

void jksv::pushNewState(std::unique_ptr<appState> &newAppState)
{
    if(s_AppStateVector.empty() == false)
    {
        s_AppStateVector.back()->takeFocus();
    }
    
    newAppState->giveFocus();
    s_AppStateVector.push_back(std::move(newAppState));
}