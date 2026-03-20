#include "ui/PopMessage.hpp"

#include "config/config.hpp"
#include "graphics/colors.hpp"
#include "graphics/fonts.hpp"
#include "graphics/screen.hpp"
#include "mathutil.hpp"
#include "sdl.hpp"

namespace
{
    constexpr int START_X              = 624;
    constexpr int START_WIDTH          = 20;
    constexpr int PERMA_HEIGHT         = 48;
    constexpr int FONT_SIZE            = 22;
    constexpr int TRANSITION_THRESHOLD = 2; // This is used so the easing on opening doesn't look so odd.
}

//                      ---- Construction ----

ui::PopMessage::PopMessage(int ticks, std::string_view message)
    : m_transition(START_X, graphics::SCREEN_HEIGHT, START_WIDTH, PERMA_HEIGHT, 0, 0, 0, 0, TRANSITION_THRESHOLD)
    , m_ticks(ticks)
    , m_message(message)
    , m_state(State::Rising)
{
    // This will load the end cap graphics if they haven't been already.
    PopMessage::initialize_static_members();
}

ui::PopMessage::PopMessage(int ticks, std::string &message)
    : m_transition(START_X, graphics::SCREEN_HEIGHT, START_WIDTH, PERMA_HEIGHT, 0, 0, 0, 0, TRANSITION_THRESHOLD)
    , m_ticks(ticks)
    , m_message(std::move(message))
    , m_state(State::Rising)
{ PopMessage::initialize_static_members(); }

//                      ---- Public functions ----

void ui::PopMessage::update(double targetY)
{
    // Always update the targetY unless the message is dropping. This is so messages collapse properly.
    if (m_state != State::Dropping) { m_transition.set_target_y(targetY); }

    // Always run the update_y routine.
    PopMessage::update_y();

    // Update the current state.
    switch (m_state)
    {
        case State::Opening:
        {
            PopMessage::update_text_offset();
            PopMessage::update_width();
        }
        break;

        case State::Displaying: PopMessage::update_display_timer(); break;
        case State::Closing:    PopMessage::update_width(); break;
        default:                break;
    }
}

void ui::PopMessage::render(sdl2::Renderer &renderer)
{
    PopMessage::render_container(renderer);

    // Don't continue unless the message is in place or it's not in its closing state.
    if (m_state != State::Opening && m_state != State::Displaying) { return; }

    // This avoids allocating and returning another std::string.
    const int y = m_transition.get_y();
    const std::string_view message{m_message.c_str(), static_cast<size_t>(m_substrOffset)};
    sm_font->render_text(m_textX, y + 11, colors::BLACK, message);
}

bool ui::PopMessage::finished() const noexcept { return m_state == State::Finished; }

std::string_view ui::PopMessage::get_message() const noexcept { return m_message; }

//                      ---- Private functions ----

void ui::PopMessage::initialize_static_members()
{
    static constexpr std::string_view CAP_PATH = "romfs:/Textures/PopMessage.png";

    if (sm_endCaps && sm_font) { return; }

    sm_endCaps = sdl2::TextureManager::create_load_resource(CAP_PATH, CAP_PATH);
    sm_font    = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::TWENTY_TWO_PIXEL,
                                                                           graphics::fonts::sizes::TWENTY_TWO_PIXEL);
}

void ui::PopMessage::update_y() noexcept
{
    // This is the tick count for typing out the message.
    static constexpr uint64_t TYPE_TICK_COUNT = 5;

    // Update the transition's Y position.
    m_transition.update_xy();

    // Update the state if needed.
    const bool transitionUpFinished   = m_state == State::Rising && m_transition.in_place_xy();
    const bool transitionDownFinished = m_state == State::Dropping && m_transition.in_place_xy();
    if (transitionUpFinished)
    {
        m_state = State::Opening;
        m_typeTimer.start(TYPE_TICK_COUNT);
    }
    else if (transitionDownFinished) { m_state = State::Finished; }
}

void ui::PopMessage::update_text_offset()
{
    // This is the center of the screen for centering.
    static constexpr int SCREEN_CENTER     = 640;
    static constexpr int CONTAINER_PADDING = 32;

    // Get the entire length of the message and do not continue unless the timer is triggered and we still have ground to
    // cover.
    const int messageLength = m_message.length();
    if (!m_typeTimer.is_triggered() || m_substrOffset >= messageLength) { return; }

    // This needs to be slightly more technical since JKSV supports UTF-8.
    uint32_t codepoint{};
    const uint8_t *targetPoint = reinterpret_cast<const uint8_t *>(&m_message[m_substrOffset]);
    const ssize_t unitCount    = decode_utf8(&codepoint, targetPoint);
    if (unitCount <= 0) { return; }

    // Add the unit count to the current offset.
    m_substrOffset += unitCount;

    // Get the substring and calculate the updated width and X of the message.
    const std::string_view subString{m_message.c_str(), static_cast<size_t>(m_substrOffset)};
    const int subWidth       = sm_font->get_text_width(subString);
    const int containerWidth = subWidth + CONTAINER_PADDING;
    m_textX                  = SCREEN_CENTER - (subWidth / 2);

    // Update the target width of the transition.
    m_transition.set_target_width(containerWidth);

    // If we've hit the end of the message, start the time for displaying the message.
    if (m_substrOffset >= messageLength)
    {
        m_state = State::Displaying;
        m_displayTimer.start(m_ticks);
    }
}

void ui::PopMessage::update_width() noexcept
{
    // Update the width of the container.
    m_transition.update_width_height();

    // To do: Not sure if I like this here, but if we're in the closing state and the transition is in place width-wise, set the
    // target Y to the bottom of the screen.
    if (m_state == State::Closing && m_transition.in_place_width_height())
    {
        m_state = State::Dropping;
        m_transition.set_target_y(graphics::SCREEN_HEIGHT);
    }
}

void ui::PopMessage::update_display_timer() noexcept
{
    // If the timer is triggered, set the target width to the closing width and change the state to closing.
    if (m_displayTimer.is_triggered())
    {
        m_state = State::Closing;
        m_transition.set_target_width(START_WIDTH);
    }
}

void ui::PopMessage::render_container(sdl2::Renderer &renderer) noexcept
{
    // The width of the CAP graphics.
    constexpr int CAP_WIDTH = 48;
    constexpr int CAP_HALF  = CAP_WIDTH / 2;

    // Calc render coordinates.
    const int x     = m_transition.get_centered_x() - (CAP_HALF / 2);
    const int y     = m_transition.get_y();
    const int width = m_transition.get_width() + CAP_HALF;

    // Render the container.
    sm_endCaps->render_part(x, y, 0, 0, CAP_HALF, CAP_WIDTH);
    renderer.render_rectangle(x + CAP_HALF, y, width - CAP_WIDTH, 48, colors::DIALOG_LIGHT);
    sm_endCaps->render_part(x + (width - CAP_HALF), y, CAP_HALF, 0, CAP_HALF, 48);
}
