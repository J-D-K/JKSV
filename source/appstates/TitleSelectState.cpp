#include "appstates/TitleSelectState.hpp"

#include "StateManager.hpp"
#include "appstates/BackupMenuState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TitleOptionState.hpp"
#include "config/config.hpp"
#include "graphics/ScopedRender.hpp"
#include "graphics/colors.hpp"
#include "graphics/targets.hpp"
#include "logging/logger.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"

#include <string_view>

namespace
{
    // All of these states share the same render target.
    constexpr std::string_view SECONDARY_TARGET = "SecondaryTarget";
} // namespace

//                      ---- Construction ----

TitleSelectState::TitleSelectState(data::User *user)
    : TitleSelectCommon()
    , m_user(user)
    , m_renderTarget(sdl2::TextureManager::create_load_resource(graphics::targets::names::SECONDARY,
                                                                graphics::targets::dims::SECONDARY_WIDTH,
                                                                graphics::targets::dims::SECONDARY_HEIGHT,
                                                                SDL_TEXTUREACCESS_TARGET))
    , m_titleView(ui::TitleView::create(m_user)) {};

//                      ---- Public functions ----

void TitleSelectState::update(const sdl2::Input &input)
{
    if (!TitleSelectState::title_count_check()) { return; }

    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input.button_pressed(HidNpadButton_A);
    const bool bPressed = input.button_pressed(HidNpadButton_B);
    const bool xPressed = input.button_pressed(HidNpadButton_X);
    const bool yPressed = input.button_pressed(HidNpadButton_Y);

    if (aPressed) { TitleSelectState::create_backup_menu(); }
    else if (xPressed) { TitleSelectState::create_title_option_menu(); }
    else if (yPressed) { TitleSelectState::add_remove_favorite(); }
    else if (bPressed)
    {
        TitleSelectState::deactivate_state();
        return;
    }

    m_titleView->update(input, hasFocus);
    sm_controlGuide->update(input, hasFocus);
}

void TitleSelectState::render(sdl2::Renderer &renderer)
{
    // Grab focus status.
    const bool hasFocus = BaseState::has_focus();

    {
        // Set target, clear.
        graphics::ScopedRender scopedRender{renderer, m_renderTarget};
        renderer.frame_begin(colors::TRANSPARENT);

        // Render view.
        m_titleView->render(renderer, hasFocus);
    }

    m_renderTarget->render(201, 91);
    sm_controlGuide->render(renderer, hasFocus);
}

void TitleSelectState::refresh() { m_titleView->refresh(); }

//                      ---- Private functions ----

bool TitleSelectState::title_count_check()
{
    const int titleCount = m_user->get_total_data_entries();

    if (titleCount <= 0)
    {
        BaseState::deactivate();
        return false;
    }
    return true;
}

void TitleSelectState::create_backup_menu()
{
    const int selected             = m_titleView->get_selected();
    const uint64_t applicationID   = m_user->get_application_id_at(selected);
    data::TitleInfo *titleInfo     = data::get_title_info_by_id(applicationID);
    const FsSaveDataInfo *saveInfo = m_user->get_save_info_at(selected);

    auto backupMenu = std::make_shared<BackupMenuState>(m_user, titleInfo, saveInfo);
    StateManager::push_state(backupMenu);
    m_titleView->play_sound();
}

void TitleSelectState::create_title_option_menu()
{
    const int selected             = m_titleView->get_selected();
    const uint64_t applicationID   = m_user->get_application_id_at(selected);
    data::TitleInfo *titleInfo     = data::get_title_info_by_id(applicationID);
    const FsSaveDataInfo *saveInfo = m_user->get_save_info_at(selected);

    TitleOptionState::create_and_push(m_user, titleInfo, saveInfo, this);
}

void TitleSelectState::deactivate_state()
{
    m_titleView->reset();
    BaseState::deactivate();
}

void TitleSelectState::add_remove_favorite()
{
    const int selected           = m_titleView->get_selected();
    const uint64_t applicationID = m_user->get_application_id_at(selected);

    config::add_remove_favorite(applicationID);

    // This applies to all users.
    data::UserList userList;
    data::get_users(userList);
    for (data::User *user : userList) { user->sort_data(); }

    // This is dirty, but it's the only way to pull it off without rewriting tons of code.
    const int count = m_user->get_total_data_entries();
    int i{};
    for (i = 0; i < count; i++)
    {
        const uint64_t appIDAt = m_user->get_application_id_at(i);
        if (appIDAt == applicationID) { break; }
    }
    m_titleView->set_selected(i);

    MainMenuState::refresh_view_states();
}
