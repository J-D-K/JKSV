#pragma once
#include "SDL.hpp"
#include "UI/Element.hpp"

// This class serves no purpose other than to wrap a sub-render target in an element for slide out panels.
namespace UI
{
    class RenderTarget : UI::Element
    {
        public:
            RenderTarget(int X, int Y, int Width, int Height);
            ~RenderTarget() {};

            void Update(bool HasFocus) {};
            void Render(SDL_Texture *Target, bool HasFocus);

            SDL_Texture *Get(void);

        private:
            // Coordinates
            int m_X, m_Y;
            // Underlying SDL::SharedTexture
            SDL::SharedTexture m_RenderTarget;
    };
} // namespace UI
