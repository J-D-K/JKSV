#include "appstates/UserSelectState.hpp"

#include "graphics/colors.hpp"
#include "input.hpp"
#include "strings/strings.hpp"

#include <string_view>

namespace
{
constexpr std::string_view PICKER_TARGET = "UserSelectTarget";

constexpr int TARGET_WIDTH  = 1080;
constexpr int TARGET_HEIGHT = 555;
constexpr int TARGET_X      = 201;
constexpr int TARGET_Y      = 91;

const char *picker_string(int index, const char *fallback)
{
    const char *string = strings::get_by_name(strings::names::PEER, index);
    return string ? string : fallback;
}
} // namespace

// User picker used by the post-receive flow. When only one compatible user
// exists the picker resolves immediately, so callers must handle both paths.
//                      ---- Construction ----

UserSelectState::UserSelectState(const std::vector<data::User *> &users, Callback callback)
    : m_users(users)
    , m_callback(std::move(callback))
    , m_renderTarget(sdl::TextureManager::load(PICKER_TARGET, TARGET_WIDTH, TARGET_HEIGHT, SDL_TEXTUREACCESS_TARGET))
{
    m_controlGuide = ui::ControlGuide::create(picker_string(41, "A: Select  B: Back"));
    m_menu         = ui::Menu::create(32, 40, 1000, 23, TARGET_HEIGHT);

    for (data::User *user : m_users)
    {
        if (user) { m_menu->add_option(user->get_nickname()); }
    }
}

//                      ---- Public functions ----

bool UserSelectState::create_and_push_if_needed(const std::vector<data::User *> &users, Callback callback)
{
    // A single compatible user is selected without showing a picker.
    if (users.size() == 1)
    {
        if (callback) { callback(users.front()); }
        return false;
    }

    if (users.size() > 1)
    {
        StateManager::push_state(UserSelectState::create(users, std::move(callback)));
        return true;
    }

    return false;
}

void UserSelectState::update()
{
    const bool hasFocus = BaseState::has_focus();

    m_menu->update(hasFocus);
    m_controlGuide->update(hasFocus);

    if (input::button_pressed(HidNpadButton_A))
    {
        const int selected = m_menu->get_selected();
        if (selected >= 0 && selected < static_cast<int>(m_users.size())) { UserSelectState::finish(m_users[selected]); }
    }
    else if (input::button_pressed(HidNpadButton_B)) { UserSelectState::finish(nullptr); }
}

void UserSelectState::sub_update() { m_controlGuide->sub_update(); }

void UserSelectState::render()
{
    const bool hasFocus = BaseState::has_focus();
    const char *title   = picker_string(40, "Select a user to apply the backup to");

    m_renderTarget->clear(colors::CLEAR_COLOR);
    sdl::text::render(m_renderTarget, 32, 10, 24, sdl::text::NO_WRAP, colors::WHITE, title);
    m_menu->render(m_renderTarget, hasFocus);
    m_renderTarget->render(sdl::Texture::Null, TARGET_X, TARGET_Y);
    m_controlGuide->render(sdl::Texture::Null, hasFocus);
}

//                      ---- Private functions ----

void UserSelectState::finish(data::User *user)
{
    if (m_finished) { return; }
    m_finished = true;

    auto callback = std::move(m_callback);
    BaseState::deactivate();
    if (callback) { callback(user); }
}
