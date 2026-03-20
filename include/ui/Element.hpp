#pragma once
#include "sdl.hpp"

namespace ui
{
    /// @brief Base class for ui elements.
    class Element
    {
        public:
            /// @brief Default constructor.
            Element() = default;

            /// @brief Virtual destructor.
            virtual ~Element() noexcept {};

            /// @brief Virtual update method. All derived classes must have this.
            /// @param HasFocus Whether or not the state containing the element currently has focus.
            virtual void update(const sdl2::Input &input, bool HasFocus) = 0;

            /// @brief Virtual render method. All derived classes must have this.
            /// @param target Target to render to.
            /// @param hasFocus Whether or not the containing state has focus.
            virtual void render(sdl2::Renderer &renderer, bool hasFocus) = 0;
    };
} // namespace ui
