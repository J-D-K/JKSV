#include "appstates/BlacklistEditState.hpp"

#include "StateManager.hpp"
#include "appstates/MainMenuState.hpp"
#include "config/config.hpp"
#include "data/data.hpp"
#include "error.hpp"
#include "graphics/screen.hpp"

//                      ---- Construction ----

BlacklistEditState::BlacklistEditState()
    : BaseState()
{
    BlacklistEditState::initialize_static_members();
    BlacklistEditState::load_blacklist();
    BlacklistEditState::initialize_menu();
    BlacklistEditState::refresh_menu();
}

//                      ---- Public functions ----

void BlacklistEditState::update(const sdl2::Input &input)
{
    const bool hasFocus = BaseState::has_focus();
    const bool aPressed = input.button_pressed(HidNpadButton_A);
    const bool bPressed = input.button_pressed(HidNpadButton_B);

    sm_slidePanel->update(input, hasFocus);

    if (aPressed) { BlacklistEditState::remove_from_blacklist(); }
    else if (sm_slidePanel->is_closed()) { BlacklistEditState::deactivate_state(); }
    else if (bPressed || m_blacklist.empty()) { sm_slidePanel->close(); }
}

void BlacklistEditState::render(sdl2::Renderer &renderer)
{
    const bool hasFocus = BaseState::has_focus();

    sm_slidePanel->render(renderer, hasFocus);
}

//                      ---- Private functions ----

void BlacklistEditState::initialize_static_members()
{
    if (sm_slidePanel) { return; }
    sm_slidePanel = std::make_unique<ui::SlideOutPanel>(480, ui::SlideOutPanel::Side::Right);
}

void BlacklistEditState::load_blacklist() { config::get_blacklisted_titles(m_blacklist); }

void BlacklistEditState::initialize_menu()
{
    m_blacklistMenu = std::make_shared<ui::Menu>(8, 8, 460, 22, graphics::SCREEN_HEIGHT);
    sm_slidePanel->push_new_element(m_blacklistMenu);
}

void BlacklistEditState::refresh_menu()
{
    m_blacklistMenu->reset();

    config::get_blacklisted_titles(m_blacklist);
    for (const uint64_t applicationID : m_blacklist)
    {
        data::TitleInfo *titleInfo = data::get_title_info_by_id(applicationID);
        if (error::is_null(titleInfo)) { continue; }

        const char *title = titleInfo->get_title();
        m_blacklistMenu->add_option(title);
    }
}

void BlacklistEditState::remove_from_blacklist()
{
    const int selected           = m_blacklistMenu->get_selected();
    const uint64_t applicationID = m_blacklist[selected];
    config::add_remove_blacklist(applicationID);

    data::UserList userList{};
    data::get_users(userList);
    for (data::User *user : userList) { user->load_user_data(); }

    MainMenuState::refresh_view_states();
    BlacklistEditState::refresh_menu();
}

void BlacklistEditState::deactivate_state()
{
    sm_slidePanel->clear_elements();
    sm_slidePanel->reset();
    BaseState::deactivate();
}
