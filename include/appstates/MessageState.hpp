#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
#include "ui/DialogBox.hpp"
#include "ui/Transition.hpp"

#include <memory>
#include <string>

class MessageState final : public BaseState
{
    public:
        /// @brief Constructor.
        /// @param message Message to display.
        MessageState(std::string_view message);

        /// @brief Same as above. Moves message instead of copying.
        MessageState(std::string &message);

        /// @brief Creates and returns a new MessageState. See constructor.
        static inline std::shared_ptr<MessageState> create(std::string_view message)
        { return std::make_shared<MessageState>(message); }

        /// @brief Creates and returns a new MessageState. See constructor.
        static inline std::shared_ptr<MessageState> create(std::string &message)
        { return std::make_shared<MessageState>(message); }

        /// @brief Same as above, only pushed to the StateManager before return.
        static inline std::shared_ptr<MessageState> create_and_push(std::string_view message)
        {
            auto newState = MessageState::create(message);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Same as above, only pushed to the StateManager before return.
        static inline std::shared_ptr<MessageState> create_and_push(std::string &message)
        {
            auto newState = MessageState::create(message);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Same as above, but creates and pushes a transition fade in between.
        static inline std::shared_ptr<MessageState> create_push_fade(std::string_view message)
        {
            auto newState = MessageState::create(message);
            auto fadeState =
                FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_BEGIN, colors::ALPHA_FADE_END, newState);
            return newState;
        }

        /// @brief Same as above, but creates and pushes a transition fade in between.
        static inline std::shared_ptr<MessageState> create_push_fade(std::string &message)
        {
            auto newState = MessageState::create(message);
            auto fadeState =
                FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_BEGIN, colors::ALPHA_FADE_END, newState);
            return newState;
        }

        /// @brief Update override.
        void update(const sdl2::Input &input) override;

        /// @brief Render override
        void render(sdl2::Renderer &renderer) override;

    private:
        /// @brief States this state can be in.
        enum class State : uint8_t
        {
            Opening,
            Displaying,
            Closing
        };

        /// @brief Safe store of the message to display.
        std::string m_message{};

        /// @brief Prevents the state from auto triggering from a previous frame.
        bool m_triggerGuard{};

        /// @brief Transition.
        ui::Transition m_transition{};

        /// @brief Stores the current state of the current state.
        MessageState::State m_state{};

        /// @brief This is the OK string.
        static inline const char *sm_okText{};

        /// @brief The center X coord for the OK [A]
        static inline int sm_okX{};

        /// @brief All instances share this. There's no point in constantly allocating a new one.
        static inline std::shared_ptr<ui::DialogBox> sm_dialog{};

        /// @brief This is the same sound that the confirmation uses.
        static inline sdl2::SharedSound sm_dialogPop{};

        /// @brief Font used to render the message.
        static inline sdl2::SharedFont sm_textFont{};

        /// @brief Font used to render the OK.
        static inline sdl2::SharedFont sm_optionFont{};

        /// @brief Allocates and ensures ^
        void initialize_static_members();

        /// @brief Updates the dialog according to the transition.
        void update_dimensions() noexcept;

        /// @brief Updates and handles the input.
        void update_handle_input(const sdl2::Input &input) noexcept;

        /// @brief Closes and "hides" the dialog.
        void close_dialog();

        /// @brief Pushes and empty fade in and marks the stage for purging.
        void deactivate_state();
};
