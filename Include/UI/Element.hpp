#pragma once
#include "SDL.hpp"

namespace UI
{
    class Element
    {
        public:
            Element(void) = default;
            virtual ~Element() {};

            virtual void Update(void) = 0;
            virtual void Render(SDL_Texture *Target);
    };
} // namespace UI
