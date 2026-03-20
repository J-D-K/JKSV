#pragma once
#include "sdl.hpp"
#include "ui/Element.hpp"
#include "ui/Transition.hpp"

#include <string>

namespace ui
{
    class DialogBox final : public ui::Element
    {
        public:
            enum class Type
            {
                Light,
                Dark
            };

            /// @brief Creates a new dialog box instance.
            /// @param width Width of the dialog box in pixels.
            /// @param height Height of the dialog box in pixels.
            /// @param text Text to display on the dialog box.
            /// @param type Optional. The type of box. Default is dark since JKSV rewrite doesn't do theme detection.
            DialogBox(int x, int y, int width, int height, DialogBox::Type type = DialogBox::Type::Dark);

            /// @brief Creates and returns a new DialogBox instance. See constructor.
            static inline std::shared_ptr<ui::DialogBox> create(int x,
                                                                int y,
                                                                int width,
                                                                int height,
                                                                DialogBox::Type type = DialogBox::Type::Dark)
            { return std::make_shared<ui::DialogBox>(x, y, width, height, type); }

            /// @brief Update override. This does NOTHING!
            void update(const sdl2::Input &input, bool hasFocus) override {};

            /// @brief Renders the dialog box to screen.
            /// @param target Render target to render to.
            /// @param hasFocus This is ignored.
            void render(sdl2::Renderer &renderer, bool hasFocus) override;

            /// @brief Sets the X render coord.
            void set_x(int x) noexcept;

            /// @brief Sets the X render coord.
            void set_y(int y) noexcept;

            /// @brief Sets the width.
            void set_width(int width) noexcept;

            /// @brief Sets the height.
            void set_height(int height) noexcept;

            /// @brief Uses the transition passed to position the dialog.
            /// @param transition Transition to use.
            /// @param centered Whether or not the center the dialog.
            void set_from_transition(ui::Transition &transition, bool centered = false);

        private:
            /// @brief X render coord.
            int m_x{};

            /// @brief Y render coord.
            int m_y{};

            /// @brief Width of the dialog.
            int m_width{};

            /// @brief Height of the dialog.
            int m_height{};

            /// @brief Stores which type and corners should be used.
            DialogBox::Type m_type{};

            /// @brief All instances shared this and the other.
            static inline sdl2::SharedTexture sm_darkCorners{};

            /// @brief This is the light cer
            static inline sdl2::SharedTexture sm_lightCorners{};

            /// @brief Initializes and loads the corners texture.
            void initialize_static_members();
    };
}
