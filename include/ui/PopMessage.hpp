#pragma once
#include "sys/Timer.hpp"
#include "ui/DialogBox.hpp"
#include "ui/Transition.hpp"

#include <cstdint>
#include <string>

namespace ui
{
    /// @brief Severity of a pop message. Neutral preserves the original look for existing callers.
    enum class PopKind : uint8_t
    {
        Neutral, ///< No severity accent.
        Info,    ///< Informational accent.
        Success, ///< Success accent.
        Warning, ///< Warning accent.
        Error    ///< Error accent.
    };

    class PopMessage
    {
        public:
            /// @brief PopMessage constructor.
            /// @param ticks Number of ticks to display the message.
            /// @param message Text to display.
            /// @param kind Severity used for the accent color.
            PopMessage(int ticks, std::string_view message, PopKind kind = PopKind::Neutral);

            /// @brief Same as above. Moves string instead of copying.
            /// @param ticks Number of ticks to display the message.
            /// @param message Text to display.
            /// @param kind Severity used for the accent color.
            PopMessage(int ticks, std::string &message, PopKind kind = PopKind::Neutral);

            /// @brief Updates the width, target, and typing.
            void update(double targetY);

            /// @brief Renders the dialog and text.
            void render();

            /// @brief Returns whether or not the message can be purged.
            bool finished() const noexcept;

            /// @brief Returns the text of the message.
            std::string_view get_message() const noexcept;

        private:
            /// @brief States the message will be in.
            enum class State : uint8_t
            {
                Rising,
                Opening,
                Displaying,
                Closing,
                Dropping,
                Finished
            };

            /// @brief Transition for the pop up/pop out thing.
            ui::Transition m_transition{};

            /// @brief Ticks to start timer with.
            int m_ticks{};

            /// @brief This stores the message for safe keeping.
            std::string m_message{};

            /// @brief Severity used for the color accent.
            PopKind m_kind{};

            /// @brief Current state the message is in.
            PopMessage::State m_state{};

            /// @brief Current X rendering coordinate for the text.
            int m_textX{};

            /// @brief The current offset of the substr.
            int m_substrOffset{};

            /// @brief Times when the message should be nuked.
            sys::Timer m_displayTimer{};

            /// @brief This times  updates the end of the substr printed.
            sys::Timer m_typeTimer{};

            /// @brief This is the texture used for the ends of the messages.
            static inline sdl::SharedTexture sm_endCaps{};

            /// @brief Ensures ^ is loaded and ready to go.
            void initialize_static_members();

            /// @brief Updates the Y Coord to match the target passed.
            void update_y() noexcept;

            /// @brief Updates the current end offset of the text.
            void update_text_offset();

            /// @brief Updates the width of the dialog.
            void update_width() noexcept;

            /// @brief Updates the display timer and begins the closing process of the message.
            void update_display_timer() noexcept;

            /// @brief Renders the container around the message.
            void render_container() noexcept;

            /// @brief Returns the accent color for the current severity.
            sdl::Color get_accent() const noexcept;
    };
}
