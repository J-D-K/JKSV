#include <SDL2/SDL.h>
#include "system/timer.hpp"

sys::timer::timer(const uint32_t &triggerTicks) : m_TriggerTicks(triggerTicks)
{
    restartTimer();
}

void sys::timer::restartTimer(void)
{
    // Set starting ticks
    m_StartingTicks = SDL_GetTicks();
}

bool sys::timer::triggered(void)
{
    // Get current ticks
    uint32_t currentTicks = SDL_GetTicks();
    // If amount is greater or equal, triggered
    if(currentTicks - m_StartingTicks >= m_TriggerTicks)
    {
        m_StartingTicks = currentTicks;
        return true;
    }
    return false;
}