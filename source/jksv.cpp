#include <memory>
#include <vector>
#include <chrono>
#include "graphics/graphics.hpp"
#include "filesystem/filesystem.hpp"
#include "data/data.hpp"
#include "ui/ui.hpp"
#include "system/input.hpp"
#include "appStates/mainMenuState.hpp"
#include "stringUtil.hpp"
#include "config.hpp"
#include "jksv.hpp"
#include "log.hpp"



// Things specifically only for here
namespace
{
    // Date built: Todo automate updating this.
    constexpr uint8_t BUILD_MONTH = 9;
    constexpr uint8_t BUILD_DAY   = 11;
    constexpr uint16_t BUILD_YEAR  = 2024;
    // This is for returning whether or not the program was successfully initialized and is running
    bool s_IsRunning = false;
    // This is the icon at the top left
    graphics::sdlTexture s_HeaderIcon;
    // This is the vector of states
    std::vector<sharedAppState> s_AppStateVector;
    // Name of icon texture
    const std::string ICON_NAME = "appIcon";
    const std::string ICON_PATH = "romfs:/img/headIcon.png";
    // Build date string.
    std::string s_BuildDateString;
    // Author string
    std::string s_TranslationAuthorString = ""; // Empty unless...
    // Strings to retrieve from ui::strings
    const std::string LANG_TRANSLATION_AUTHOR_STRING = "author";
    const std::string LANG_TRANSLATION_MAIN_PAGE = "translationMainPage";
}

// This is a special struct so I

bool jksv::init(void)
{
    // Init logger
    logger::init();

    // Almost everything depends on graphics so first
    if (graphics::init("JKSV", 1280, 720, 0) == false)
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

    // Input doesn't have anything to return.
    sys::input::init();

    if(data::init() == false)
    {
        return false;
    }

    // Load header icon
    s_HeaderIcon = graphics::textureManager::loadTextureFromFile(ICON_PATH);

    // Load & Create Translation string
    std::string author = ui::strings::getString(LANG_TRANSLATION_AUTHOR_STRING, 0);
    std::string translation = ui::strings::getString(LANG_TRANSLATION_MAIN_PAGE, 0);
    s_TranslationAuthorString = stringUtil::getFormattedString("%s %s", translation.c_str(), author.c_str());

    // Create build date string
    s_BuildDateString = stringUtil::getFormattedString("Version: %02d.%02d.%04d", BUILD_MONTH, BUILD_DAY, BUILD_YEAR);

    // Hope this works and will add main menu state
    sharedAppState mainMenu = createMainMenuState();
    jksv::pushState(mainMenu);

    // Hope everything is up and running
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
    if(s_AppStateVector.empty() == false)
    {
        while(s_AppStateVector.back()->isActive() == false)
        {
            s_AppStateVector.pop_back();
            s_AppStateVector.back()->giveFocus();
        }
        // Update only back
        s_AppStateVector.back()->update();
    }

    // Update pop up messages
    ui::popMessage::update();
}

void jksv::render(void)
{
    graphics::beginFrame(COLOR_DEFAULT_CLEAR);
    // Render the base
    // Div lines
    graphics::renderLine(NULL, 30, 88, 1250, 88, COLOR_WHITE);
    graphics::renderLine(NULL, 30, 648, 1250, 648, COLOR_WHITE);
    // Icon
    graphics::textureRender(s_HeaderIcon.get(), NULL, 66, 27);
    // JKSV Text
    graphics::systemFont::renderText("JKSV", NULL, 130, 38, 24, COLOR_WHITE);
    // Translation: Author if available...
    if(s_TranslationAuthorString.empty() == false)
    {
        graphics::systemFont::renderText(s_TranslationAuthorString, NULL, 8, 682, 12, COLOR_WHITE);
    }
    // Build Date
    graphics::systemFont::renderText(s_BuildDateString, NULL, 8, 700, 12, COLOR_WHITE);


    // Render the state vector
    for (sharedAppState &currentAppState : s_AppStateVector)
    {
        currentAppState->render();
    }

    // Render pop up messages
    ui::popMessage::render();

    graphics::endFrame();
}

const bool jksv::isRunning(void)
{
    return s_IsRunning;
}

void jksv::pushState(std::shared_ptr<appState> state)
{
    if(s_AppStateVector.empty() == false)
    {
        // Take focus away from the one in the back now.
        s_AppStateVector.back()->takeFocus();
    }
    // Give focus to incoming one.
    state->giveFocus();
    // Push to back
    s_AppStateVector.push_back(state);
}

void jksv::popState(void)
{
    // If it's not empty, pop_back
    if(s_AppStateVector.empty() == false)
    {
        // Pop
        s_AppStateVector.pop_back();
    }

    // Have to check again just in case.
    if(s_AppStateVector.empty() == false)
    {
        s_AppStateVector.back()->giveFocus();
    }
}
