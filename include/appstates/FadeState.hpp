#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "sdl.hpp"
#include "sys/sys.hpp"

#include <memory>

class FadeState final : public BaseState
{
    public:
        /// @brief Direction (In/Out) of the fade. This is auto-determined at construction.
        enum class Direction : uint8_t
        {
            In,
            Out
        };

        /// @brief Creates a new fade in state.
        /// @param nextState The next state to push after the the fade is finished.
        FadeState(SDL_Color baseColor, uint8_t startAlpha, uint8_t endAlpha, std::shared_ptr<BaseState> nextState);

        /// @brief Returns a new fade in state. See constructor.
        static inline std::shared_ptr<FadeState> create(SDL_Color baseColor,
                                                        uint8_t startAlpha,
                                                        uint8_t endAlpha,
                                                        std::shared_ptr<BaseState> nextState)
        { return std::make_shared<FadeState>(baseColor, startAlpha, endAlpha, nextState); }

        /// @brief Creates, returns and pushes a new FadeInState to the statemanager.
        static std::shared_ptr<FadeState> create_and_push(SDL_Color baseColor,
                                                          uint8_t startAlpha,
                                                          uint8_t endAlpha,
                                                          std::shared_ptr<BaseState> nextState)
        {
            auto newState = FadeState::create(baseColor, startAlpha, endAlpha, nextState);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Update override.
        void update(const sdl2::Input &input) override;

        /// @brief Render override.
        void render(sdl2::Renderer &renderer) override;

    private:
        /// @brief Base color of the fade.
        SDL_Color m_baseColor{};

        /// @brief Alpha value.
        uint8_t m_alpha{};

        /// @brief Alpha value to destruct at.
        uint8_t m_endAlpha{};

        /// @brief Direction (in/out) of the fade. This is auto determined according to alpha values passed.
        FadeState::Direction m_direction{};

        /// @brief The divisor found to make sure alpha ends evenly.
        uint8_t m_divisor{};

        /// @brief Timer for fade.
        sys::Timer m_fadeTimer{};

        /// @brief Pointer to the next state to push.
        std::shared_ptr<BaseState> m_nextState{};

        /// @brief Finds the highest divisor for the fade to use.
        void find_divisor();

        /// @brief Decreases alpha by m_divisor.
        void decrease_alpha();

        /// @brief Increases alpha by m_divisor.
        void increase_alpha();

        /// @brief Completes the fade.
        void completed();
};
