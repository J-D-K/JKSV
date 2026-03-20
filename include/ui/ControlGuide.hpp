#pragma once
#include "sdl.hpp"
#include "ui/Element.hpp"
#include "ui/Transition.hpp"

namespace ui
{
    class ControlGuide final : public ui::Element
    {
        public:
            /// @brief Creates a new control guide.
            /// @param string Pointer to the control guide string to render.
            ControlGuide(const char *guide);

            /// @brief Factory function to return a control guide.
            static inline std::shared_ptr<ControlGuide> create(const char *guide)
            { return std::make_shared<ControlGuide>(guide); }

            /// @brief Update routine. Opens the guide.
            void update(const sdl2::Input &input, bool hasFocus) override;

            /// @brief Sub update routine. Handles hiding the control guide.
            void sub_update();

            /// @brief Renders the control guide. Both arguments are ignored in this case.
            void render(sdl2::Renderer &renderer, bool hasFocus) override;

            /// @brief This is a workaround for states where the guide can't really be hidden correctly. Ex: FileMode.
            void reset() noexcept;

        private:
            /// @brief States the guide can be in.
            enum class State : uint8_t
            {
                Opening,
                Opened,
                Hiding,
                Hidden
            };

            /// @brief Stores the pointer to the guide string.
            const char *m_guide{};

            /// @brief Width of the control guide text.
            int m_textWidth{};

            /// @brief This stores the target X coordinate of the guide.
            int m_targetX{};

            /// @brief Width of the guide.
            int m_guideWidth{};

            /// @brief Transition for the guide string.
            ui::Transition m_transition{};

            /// @brief Current state of the control guide.
            ControlGuide::State m_state{};

            /// @brief This is shared by all instances.
            static inline sdl2::SharedTexture sm_controlCap{};

            /// @brief Font used to render text.
            static inline sdl2::SharedFont sm_font{};

            /// @brief Ensures the control guide cap is loaded for all instances.
            void initialize_static_members();

            /// @brief Updates the position of the guide.
            void update_position_state() noexcept;

            /// @brief Updates the current state of the control guide.
            void update_state(bool hasFocus) noexcept;
    };
}