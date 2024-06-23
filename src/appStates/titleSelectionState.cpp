#include <memory>
#include "appStates/titleSelectionState.hpp"
#include "appStates/backupMenuState.hpp"
#include "appStates/titleOptionState.hpp"
#include "filesystem/filesystem.hpp"
#include "graphics/graphics.hpp"
#include "system/input.hpp"
#include "ui/ui.hpp"
#include "log.hpp"
#include "jksv.hpp"

// Texture names
const char *TITLE_SELECT_RENDER_TARGET = "titleSelectionRenderTarget";

// Render target dimensions
static const int RENDER_TARGET_WIDTH = 1080;
static const int RENDER_TARGET_HEIGHT = 555;

// X coordinate to subtract from for guide text
static const int GUIDE_STARTING_X = 1220;

titleSelectionState::titleSelectionState(data::user *currentUser) :
m_CurrentUser(currentUser),
m_TitleSelection(std::make_unique<ui::titleSelection>(m_CurrentUser)),
m_TitleControlGuide(ui::strings::getString(LANG_TITLE_GUIDE, 0)),
m_TitleControlGuideX(GUIDE_STARTING_X - graphics::systemFont::getTextWidth(m_TitleControlGuide, 18))
{
    // Render target to render everything too
    m_RenderTarget = graphics::textureCreate(TITLE_SELECT_RENDER_TARGET, RENDER_TARGET_WIDTH, RENDER_TARGET_HEIGHT, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
}

titleSelectionState::~titleSelectionState() {}

void titleSelectionState::update(void)
{
    m_TitleSelection->update();

    if(sys::input::buttonDown(HidNpadButton_A))
    {
        int selected = m_TitleSelection->getSelected();
        data::userSaveInfo *currentUserSaveInfo = m_CurrentUser->getUserSaveInfoAt(selected);
        if(fs::mountSaveData(currentUserSaveInfo->getSaveDataInfo()))
        {
            data::titleInfo    *currentTitleInfo = data::getTitleInfoByTitleID(currentUserSaveInfo->getTitleID());
            std::unique_ptr<appState> backupState = std::make_unique<backupMenuState>(m_CurrentUser, currentUserSaveInfo, currentTitleInfo);
            jksv::pushNewState(backupState);
        }
        else
        {
            logger::log("Error mounting save for %016lX", currentUserSaveInfo->getTitleID());
        }
    }
    else if(sys::input::buttonDown(HidNpadButton_X))
    {
        // Get some stuff needed
        int selected = m_TitleSelection->getSelected();
        data::userSaveInfo *currentUserSaveInfo = m_CurrentUser->getUserSaveInfoAt(selected);
        data::titleInfo *currentTitleInfo = data::getTitleInfoByTitleID(currentUserSaveInfo->getTitleID());

        // Create option state
        std::unique_ptr<appState> optionState = std::make_unique<titleOptionState>(m_CurrentUser, currentUserSaveInfo, currentTitleInfo);
        // Push it
        jksv::pushNewState(optionState);
    }
    else if(sys::input::buttonDown(HidNpadButton_B))
    {
        appState::deactivateState();
    }
}

void titleSelectionState::render(void)
{
    // Clear render target
    graphics::textureClear(m_RenderTarget, COLOR_TRANSPARENT);
    m_TitleSelection->render(m_RenderTarget);

    // Render to screen
    graphics::textureRender(m_RenderTarget, NULL, 201, 91);

    if(appState::hasFocus())
    {
        graphics::systemFont::renderText(m_TitleControlGuide, NULL, m_TitleControlGuideX, 673, 18, COLOR_WHITE);
    }
}

void createAndPushNewTitleSelectionState(data::user *currentUser)
{
    std::unique_ptr<appState> newTitleSelectionState = std::make_unique<titleSelectionState>(currentUser);
    jksv::pushNewState(newTitleSelectionState);
}