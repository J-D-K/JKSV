#pragma once
#include "sdl.hpp"

#include <stack>

namespace graphics
{
    /// @brief This is a workaround class because of SDLLib changes and I'm not willing to sink a ton of time into JKSV anymore.
    class ScopedRender final
    {
        public:
            /// @brief Constructs a new scoped rendering sequence.
            /// @param renderer Reference to the renderer.
            /// @param target Render target.
            ScopedRender(sdl2::Renderer &renderer, sdl2::SharedTexture target)
                : m_renderer{renderer}
            {
                // Push the target.
                sm_renderTargets.push(target);

                // Set the target.
                renderer.set_render_target(sm_renderTargets.top());
            }

            /// @brief Ends the ScopedRender'ing.
            ~ScopedRender()
            {
                // Pop.
                sm_renderTargets.pop();

                // Set.
                m_renderer.set_render_target(sm_renderTargets.top());
            }

        private:
            /// @brief Reference to the renderer.
            sdl2::Renderer &m_renderer;

            /// @brief Stack of render targets. Init'd with null as the base.
            static inline std::stack<sdl2::SharedTexture> sm_renderTargets =
                std::stack<sdl2::SharedTexture>({sdl2::Texture::null});
    };
}