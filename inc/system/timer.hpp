#pragma once
#include <cstdint>

namespace sys
{
    class timer
    {
        public:
            // Inits timer with current ticks
            timer(const uint32_t &triggerTicks);
            // Returns if triggerTicks has been reached.
            bool triggered(void);

        private:
            // Starting tick count
            uint32_t m_StartingTicks = 0;
            // Trigger tick count
            uint32_t m_TriggerTicks = 0;
    };
}