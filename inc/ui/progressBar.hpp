#pragma once
#include <string>
#include <cstdint>
#include <SDL2/SDL.h>

namespace ui
{
    class progressBar
    {
        public:
            // Inits progress bar with maxValue as maximum
            progressBar(const uint64_t &maxValue);
            // Updates bar. Progress is the current number/max
            void update(const uint64_t &progress);
            // Renders bar to target
            void render(const std::string &text, SDL_Texture *target);

        private:
            // Maximum value
            uint64_t m_MaxValue;
            // Current progress
            uint64_t m_Progress = 0;
            // Width of bar to render
            uint32_t m_BarWidth;
            // String that displays XMB / XMB
            std::string m_ProgressString;
    };
}