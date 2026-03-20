#include "appstates/TextTitleSelectState.hpp"

#include "StateManager.hpp"
#include "appstates/BackupMenuState.hpp"
#include "appstates/MainMenuState.hpp"
#include "appstates/TitleOptionState.hpp"
#include "config/config.hpp"
#include "fs/save_mount.hpp"
#include "fslib.hpp"
#include "graphics/ScopedRender.hpp"
#include "graphics/colors.hpp"
#include "graphics/targets.hpp"
#include "logging/logger.hpp"
#include "sdl.hpp"

#include <string_view>

//                      ---- Construction ----

TextTitleSelectState::TextTitleSelectState(data::User *user)
    : TitleSelectCommon()
    , m_user(user)
    , m_titleSelectMenu(ui::Menu::create(32, 10, 1000, 23, 555))
    , m_renderTarget(sdl2::TextureManager::create_load_resource(graphics::targets::names::SECONDARY,
                                                                graphics::targets::dims::SECONDARY_WIDTH,
                                                                graphics::targets::dims::SECONDARY_HEIGHT,
                                                                SDL_TEXTUREACCESS_TARGET))
{ TextTitleSelectState::refresh(); }

//                      ---- Public functions ----

void TextTitleSelectState::update(const sdl2::Input &input)
{
    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input.button_pressed(HidNpadButton_A);
    const bool bPressed = input.button_pressed(HidNpadButton_B);
    const bool xPressed = input.button_pressed(HidNpadButton_X);
    const bool yPressed = input.button_pressed(HidNpadButton_Y);

    m_titleSelectMenu->update(input, hasFocus);

    if (aPressed) { TextTitleSelectState::create_backup_menu(); }
    else if (xPressed) { TextTitleSelectState::create_title_option_menu(); }
    else if (yPressed) { TextTitleSelectState::add_remove_favorite(); }
    else if (bPressed) { BaseState::deactivate(); }

    sm_controlGuide->update(input, hasFocus);
}

void TextTitleSelectState::render(sdl2::Renderer &renderer)
{
    // Grab focus.
    const bool hasFocus = BaseState::has_focus();

    {
        // Switch targets, clear.
        graphics::ScopedRender scopedRender{renderer, m_renderTarget};
        renderer.frame_begin(colors::TRANSPARENT);

        // Render menu to target.
        m_titleSelectMenu->render(renderer, hasFocus);
    }

    // Switch back, render target and guide.
    m_renderTarget->render(201, 91);
    sm_controlGuide->render(renderer, hasFocus);
}

void TextTitleSelectState::refresh()
{
    static constexpr const char *STRING_HEART = "^\uE017^ ";

    m_titleSelectMenu->reset(false);

    const size_t totalEntries = m_user->get_total_data_entries();
    for (size_t i = 0; i < totalEntries; i++)
    {
        const uint64_t applicationID = m_user->get_application_id_at(i);
        const bool favorite          = config::is_favorite(applicationID);
        data::TitleInfo *titleInfo   = data::get_title_info_by_id(applicationID);
        const char *title            = titleInfo->get_title();

        std::string option{};
        if (favorite) { option = std::string{STRING_HEART} + title; }
        else
        {
            option = title;
        }

        m_titleSelectMenu->add_option(option);
    }
}

//                      ---- Private functions ----

void TextTitleSelectState::create_backup_menu()
{
    const int selected             = m_titleSelectMenu->get_selected();
    const uint64_t applicationID   = m_user->get_application_id_at(selected);
    data::TitleInfo *titleInfo     = data::get_title_info_by_id(applicationID);
    const FsSaveDataInfo *saveInfo = m_user->get_save_info_at(selected);

    BackupMenuState::create_and_push(m_user, titleInfo, saveInfo);
}

void TextTitleSelectState::create_title_option_menu()
{
    const int selected             = m_titleSelectMenu->get_selected();
    const uint64_t applicationID   = m_user->get_application_id_at(selected);
    data::TitleInfo *titleInfo     = data::get_title_info_by_id(applicationID);
    const FsSaveDataInfo *saveInfo = m_user->get_save_info_at(selected);

    TitleOptionState::create_and_push(m_user, titleInfo, saveInfo, this);
}

void TextTitleSelectState::add_remove_favorite()
{
    const int selected           = m_titleSelectMenu->get_selected();
    const uint64_t applicationID = m_user->get_application_id_at(selected);

    config::add_remove_favorite(applicationID);

    // This applies to all users.
    data::UserList list{};
    data::get_users(list);
    for (data::User *user : list) { user->sort_data(); }

    const int count = m_user->get_total_data_entries();
    int i{};
    for (i = 0; i < count; i++)
    {
        const uint64_t appIDAt = m_user->get_application_id_at(i);
        if (appIDAt == applicationID) { break; }
    }

    MainMenuState::refresh_view_states();
    m_titleSelectMenu->set_selected(i);
}
