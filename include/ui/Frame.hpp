#pragma once
#include "sdl.hpp"
#include "ui/Element.hpp"
#include "ui/Transition.hpp"

#include <memory>

namespace ui
{
    class Frame final : public ui::Element
    {
        public:
            /// @brief Constructs a new frame.
            Frame(int x, int y, int width, int height);

            /// @brief Inline function to make constructing nicer.
            static inline std::shared_ptr<ui::Frame> create(int x, int y, int width, int height)
            { return std::make_shared<ui::Frame>(x, y, width, height); }

            /// @brief Doesn't need to do anything for this.
            void update(const sdl2::Input &input, bool hasFocus) override {};

            /// @brief Renders the frame to the target passed.
            void render(sdl2::Renderer &renderer, bool hasFocus) override;

            /// @brief Sets the X coord.
            void set_x(int x) noexcept;

            /// @brief Sets the Y coord.
            void set_y(int y) noexcept;

            /// @brief Sets the width of the frame.
            void set_width(int width) noexcept;

            /// @brief Sets the height of the frame.
            void set_height(int height) noexcept;

            /// @brief Sets the dimensions and coords of the the frame according to the transition passed.
            void set_from_transition(const ui::Transition &transition, bool centered = false) noexcept;

        private:
            /// @brief X rendering coord.
            int m_x{};

            /// @brief Y rendering coord.
            int m_y{};

            /// @brief Rendering width.
            int m_width{};

            /// @brief Rendering height.
            int m_height{};

            /// @brief This texture is shared by all instances.
            static inline sdl2::SharedTexture sm_frameCorners{};

            /// @brief Ensures the texture is loading if it hasn't been.
            void initialize_static_members();
    };
}