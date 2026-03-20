#include "appstates/MessageState.hpp"

#include "StateManager.hpp"
#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
#include "graphics/fonts.hpp"
#include "graphics/screen.hpp"
#include "mathutil.hpp"
#include "strings/strings.hpp"

namespace
{
    // Initial coordinates.
    constexpr int INITIAL_WIDTH_HEIGHT = 32;

    // Target
    constexpr int TARGET_WIDTH  = 720;
    constexpr int TARGET_HEIGHT = 256;
}

//                      ---- Construction ----

MessageState::MessageState(std::string_view message)
    : m_message(message)
    , m_transition(0,
                   0,
                   INITIAL_WIDTH_HEIGHT,
                   INITIAL_WIDTH_HEIGHT,
                   0,
                   0,
                   TARGET_WIDTH,
                   TARGET_HEIGHT,
                   ui::Transition::DEFAULT_THRESHOLD)
    , m_state(State::Opening)
{
    MessageState::initialize_static_members();
    sm_dialogPop->play();
}

MessageState::MessageState(std::string &message)
    : m_message(std::move(message))
    , m_transition(0,
                   0,
                   INITIAL_WIDTH_HEIGHT,
                   INITIAL_WIDTH_HEIGHT,
                   0,
                   0,
                   TARGET_WIDTH,
                   TARGET_HEIGHT,
                   ui::Transition::DEFAULT_THRESHOLD)
    , m_state(State::Opening)
{
    MessageState::initialize_static_members();
    sm_dialogPop->play();
}

//                      ---- Public functions ----

void MessageState::update(const sdl2::Input &input)
{
    switch (m_state)
    {
        case State::Opening:
        case State::Closing:    MessageState::update_dimensions(); break;
        case State::Displaying: MessageState::update_handle_input(input); break;
    }
}

void MessageState::render(sdl2::Renderer &renderer)
{
    // This is were to render
    static constexpr int y = 229;

    const bool hasFocus = BaseState::has_focus();

    // Dim the background and render the dialog. Only continue if we're fully open.
    renderer.render_rectangle(0, 0, graphics::SCREEN_WIDTH, graphics::SCREEN_HEIGHT, colors::DIM_BACKGROUND);
    sm_dialog->render(renderer, hasFocus);
    if (!m_transition.in_place()) { return; }

    sm_textFont->render_text_wrapped(312, y + 24, colors::WHITE, 656, m_message);
    renderer.render_line(280, y + 192, 999, y + 192, colors::DIV_COLOR);
    sm_optionFont->render_text(sm_okX, y + 214, colors::WHITE, sm_okText);
}

//                      ---- Private functions ----

void MessageState::initialize_static_members()
{
    static constexpr std::string_view CONFIRM_POP = "romfs:/Sound/ConfirmPop.wav";

    if (sm_okText && sm_dialog && sm_dialogPop && sm_textFont && sm_optionFont) { return; }

    // This is needed later, so it comes first.
    sm_textFont   = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::TWENTY_PIXEL,
                                                                              graphics::fonts::sizes::TWENTY_PIXEL);
    sm_optionFont = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::THIRTY_TWO_PIXEL,
                                                                              graphics::fonts::sizes::TWENTY_TWO_PIXEL);

    // OK text and X coord.
    sm_okText = strings::get_by_name(strings::names::YES_NO_OK, 2);
    sm_okX    = math::Util<int>::center_within(graphics::SCREEN_WIDTH, sm_textFont->get_text_width(sm_okText));

    // Dialog.
    sm_dialog = ui::DialogBox::create(0, 0, 0, 0);
    sm_dialog->set_from_transition(m_transition, true);

    // Sound to play.
    sm_dialogPop = sdl2::SoundManager::create_load_resource(CONFIRM_POP, CONFIRM_POP);
}

void MessageState::update_dimensions() noexcept
{
    // Update the dialog.
    m_transition.update();
    sm_dialog->set_from_transition(m_transition, true);

    // Conditions for state shifting.
    const bool opened = m_state == State::Opening && m_transition.in_place();
    const bool closed = m_state == State::Closing && m_transition.in_place();
    if (opened) { m_state = State::Displaying; }
    else if (closed) { MessageState::deactivate_state(); }
}

void MessageState::update_handle_input(const sdl2::Input &input) noexcept
{
    // Input bools.
    const bool aPressed = input.button_pressed(HidNpadButton_A);

    // Handle the triggerguard.
    m_triggerGuard = m_triggerGuard || (aPressed && !m_triggerGuard);

    // Conditions.
    const bool finished = m_triggerGuard && aPressed;
    if (finished) { MessageState::close_dialog(); }
}

void MessageState::close_dialog()
{
    m_state = State::Closing;
    m_transition.set_target_width(32);
    m_transition.set_target_height(32);
}

void MessageState::deactivate_state()
{
    FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_END, colors::ALPHA_FADE_BEGIN, nullptr);
    BaseState::deactivate();
}
