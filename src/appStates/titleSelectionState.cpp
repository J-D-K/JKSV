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


titleSelectionState::titleSelectionState(data::user *currentUser) : m_CurrentUser(currentUser)
{
    // Setup control string
    m_TitleControlGuide = ui::strings::getString(LANG_TITLE_GUIDE, 0);
    m_TitleControlGuideX = 1220 - graphics::systemFont::getTextWidth(m_TitleControlGuide, 18);

    // Render target to render everything too
    m_RenderTarget = graphics::textureCreate("titleSelectionRenderTarget", 1080, 555, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);

    // Icon 
    m_TitleSelection = std::make_unique<ui::titleSelection>(m_CurrentUser);
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