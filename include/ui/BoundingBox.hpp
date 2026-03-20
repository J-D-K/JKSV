#pragma once
#include "sdl.hpp"
#include "ui/ColorMod.hpp"
#include "ui/Element.hpp"

#include <memory>

namespace ui
{
    class BoundingBox final : public ui::Element
    {
        public:
            /// @brief Constructs a new bounding box instance.
            /// @param x X coord to render to.
            /// @param y Y coord to render to.
            /// @param width Width of the box in pixels.
            /// @param height Height of the box in pixels.
            BoundingBox(int x, int y, int width, int height);

            /// @brief Creates a returns a new BoundingBox. See constructor.
            static inline std::shared_ptr<ui::BoundingBox> create(int x, int y, int width, int height)
            { return std::make_shared<ui::BoundingBox>(x, y, width, height); }

            /// @brief Update override.
            void update(const sdl2::Input &input, bool hasFocus) override;

            /// @brief Render override.
            void render(sdl2::Renderer &renderer, bool hasFocus) override;

            /// @brief Sets the X coord.
            void set_x(int x) noexcept;

            /// @brief Sets the Y coord.
            void set_y(int y) noexcept;

            /// @brief Sets the width.
            void set_width(int width) noexcept;

            /// @brief Sets the height.
            void set_height(int height) noexcept;

        private:
            /// @brief X coord to render to.
            int m_x{};

            /// @brief Y coord to rend to.
            int m_y{};

            /// @brief Width of the box.
            int m_width{};

            /// @brief Height of the box.
            int m_height{};

            /// @brief Color for the bounding box.
            ui::ColorMod m_colorMod{};

            /// @brief This is shared by all instances.
            static inline sdl2::SharedTexture sm_corners{};

            /// @brief Loads ^
            void initialize_static_members();
    };
}
