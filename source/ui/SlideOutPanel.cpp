#include "ui/SlideOutPanel.hpp"

#include "config/config.hpp"
#include "graphics/ScopedRender.hpp"
#include "graphics/colors.hpp"
#include "graphics/screen.hpp"
#include "logging/logger.hpp"
#include "mathutil.hpp"
#include "stringutil.hpp"

#include <cmath>
#include <utility>

//                      ---- Construction ----

ui::SlideOutPanel::SlideOutPanel(int width, Side side)
    : m_width(width)
    , m_targetX(side == Side::Left ? 0.0f : static_cast<double>(graphics::SCREEN_WIDTH) - m_width)
    , m_side(side)
    , m_transition(m_side == Side::Left ? -m_width : graphics::SCREEN_WIDTH,
                   0,
                   0,
                   0,
                   m_targetX,
                   0,
                   0,
                   0,
                   ui::Transition::DEFAULT_THRESHOLD)
    , m_state(State::Opening)
    , m_renderTarget(sdl2::TextureManager::create_load_resource(stringutil::get_formatted_string("PANEL_%i", sm_targetID++),
                                                                m_width,
                                                                graphics::SCREEN_HEIGHT,
                                                                SDL_TEXTUREACCESS_TARGET)) {};

//                      ---- Public functions ----

void ui::SlideOutPanel::update(const sdl2::Input &input, bool hasFocus)
{
    switch (m_state)
    {
        case State::Opening:
        case State::Closing:
        case State::Hiding:  SlideOutPanel::update_position_state(); break;
        case State::Opened:  SlideOutPanel::update_sub_elements(input, hasFocus); break;
        default:             return; // Nothing should take place in any other state.
    }
}

void ui::SlideOutPanel::sub_update() { m_transition.update(); }

void ui::SlideOutPanel::render(sdl2::Renderer &renderer, bool hasFocus)
{
    {
        // Set target.
        graphics::ScopedRender scopedRender{renderer, m_renderTarget};

        // Loop and render all sub-elements to the target.
        for (auto &currentElement : m_elements) { currentElement->render(renderer, hasFocus); }
    }

    // Render the main target to the target passed (most likely the screen);
    const int x = m_transition.get_x();
    m_renderTarget->render(x, 0);
}

void ui::SlideOutPanel::clear_target(sdl2::Renderer &renderer)
{
    graphics::ScopedRender scopedRender{renderer, m_renderTarget};
    renderer.frame_begin(colors::SLIDE_PANEL_CLEAR);
}

void ui::SlideOutPanel::reset() noexcept
{
    // Grab the X to close and to open.
    const int x       = SlideOutPanel::get_target_close_x();
    const int targetX = SlideOutPanel::get_target_open_x();

    // Since we're resetting, we just set this immediately to be 100% sure. There is no transition.
    m_transition.set_x(x);

    // This is reset for the next opening.
    m_transition.set_target_x(targetX);

    // Reset state to opening.
    m_state = State::Opening;
}

void ui::SlideOutPanel::close() noexcept
{
    // Grab target X to close the set the target X to that.
    const int targetX = SlideOutPanel::get_target_close_x();

    // Set the target to the closed X.
    m_transition.set_target_x(targetX);

    // Set the state to closing.
    m_state = State::Closing;
}

void ui::SlideOutPanel::hide() noexcept
{
    // Get close X, but set to hiding instead of closing.
    const int targetX = SlideOutPanel::get_target_close_x();
    m_transition.set_target_x(targetX);

    m_state = State::Hiding;
}

void ui::SlideOutPanel::unhide() noexcept
{
    // Get open X, set state to opening.
    const int targetX = SlideOutPanel::get_target_open_x();
    m_transition.set_target_x(targetX);

    m_state = State::Opening;
}

void ui::SlideOutPanel::unhide_on_focus(bool hasFocus) noexcept
{
    // Just bail if we don't even have focus.
    if (!hasFocus) { return; }

    // Need to know if the panel is hiding.
    const bool isHiding = m_state == State::Hiding || m_state == State::Hidden;
    if (!isHiding) { return; }

    // At this point, we need to unhide.
    // Grab the open x target and set the target.
    const int targetX = SlideOutPanel::get_target_open_x();
    m_transition.set_target_x(targetX);

    // Set the state to opening.
    m_state = State::Opening;
}

bool ui::SlideOutPanel::is_open() const noexcept { return m_state == State::Opened; }

bool ui::SlideOutPanel::is_closed() const noexcept { return m_state == State::Closed; }

bool ui::SlideOutPanel::is_hidden() const noexcept { return m_state == State::Hidden; }

void ui::SlideOutPanel::push_new_element(std::shared_ptr<ui::Element> newElement) { m_elements.push_back(newElement); }

void ui::SlideOutPanel::clear_elements() { m_elements.clear(); }

sdl2::SharedTexture &ui::SlideOutPanel::get_target() noexcept { return m_renderTarget; }

//                      ---- Private functions ----

void ui::SlideOutPanel::update_position_state() noexcept
{
    // Update transition.
    m_transition.update();

    // State shifting conditions.
    const bool opened = m_state == State::Opening && m_transition.in_place_xy();
    const bool closed = !opened && m_state == State::Closing && m_transition.in_place_xy();
    const bool hidden = !opened && !closed && m_state == State::Hiding && m_transition.in_place_xy();
    if (opened) { m_state = State::Opened; }
    else if (closed) { m_state = State::Closed; }
    else if (hidden) { m_state = State::Hidden; }
}

void ui::SlideOutPanel::update_sub_elements(const sdl2::Input &input, bool hasFocus) noexcept
{
    for (auto &element : m_elements) { element->update(input, hasFocus); }
}