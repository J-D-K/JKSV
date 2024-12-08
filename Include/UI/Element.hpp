#pragma once
#include "SDL.hpp"

namespace UI
{
    class Element
    {
        public:
            Element(void) = default;
            virtual ~Element() {};

            virtual void Update(bool HasFocus) = 0;
            virtual void Render(SDL_Texture *Target, bool HasFocus) = 0;
    };
} // namespace UI
