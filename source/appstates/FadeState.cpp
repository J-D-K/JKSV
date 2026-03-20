#include "appstates/FadeState.hpp"

#include "StateManager.hpp"
#include "graphics/screen.hpp"
#include "logging/logger.hpp"
#include "mathutil.hpp"
#include "sdl.hpp"

namespace
{
    constexpr uint64_t TICKS_TIMER_TRIGGER = 1;
}

//                          ---- Construction ----
FadeState::FadeState(SDL_Color baseColor, uint8_t startAlpha, uint8_t endAlpha, std::shared_ptr<BaseState> nextState)
    : m_baseColor(baseColor)
    , m_alpha(startAlpha)
    , m_endAlpha(endAlpha)
    , m_direction(m_endAlpha < m_alpha ? FadeState::Direction::In : FadeState::Direction::Out)
    , m_nextState(nextState)
{
    FadeState::find_divisor();
    m_fadeTimer.start(TICKS_TIMER_TRIGGER);
}

void FadeState::update(const sdl2::Input &input)
{
    if (m_alpha == m_endAlpha)
    {
        FadeState::completed();
        return;
    }

    switch (m_direction)
    {
        case Direction::In:  FadeState::decrease_alpha(); break;
        case Direction::Out: FadeState::increase_alpha(); break;
    }
}

void FadeState::render(sdl2::Renderer &renderer)
{
    // Update alpha.
    m_baseColor.a = m_alpha;

    // Render overtop of everything.
    renderer.render_rectangle(0, 0, graphics::SCREEN_WIDTH, graphics::SCREEN_HEIGHT, m_baseColor);
}

//                      ---- Private functions ----

void FadeState::find_divisor()
{
    // Get the distance between where we start and need to end.
    m_divisor = math::Util<uint8_t>::absolute_distance(m_alpha, m_endAlpha);

    // Going to loop and try to find the highest divisible number. To do: Maybe not brute force this?
    for (uint8_t i = 48; i > 0; i--)
    {
        if (m_divisor % i == 0)
        {
            m_divisor = i;
            break;
        }
    }
}

void FadeState::decrease_alpha() { m_alpha -= m_divisor; }

void FadeState::increase_alpha() { m_alpha += m_divisor; }

void FadeState::completed()
{
    if (m_nextState) { StateManager::push_state(m_nextState); }
    BaseState::deactivate();
}
