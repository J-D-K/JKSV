#pragma once
#include "appstates/BaseState.hpp"
#include "sdl.hpp"

#include <concepts>
#include <memory>
#include <vector>

class StateManager
{
    public:
        // Singleton. No copying or constructing.
        StateManager(const StateManager &)            = delete;
        StateManager(StateManager &&)                 = delete;
        StateManager &operator=(const StateManager &) = delete;
        StateManager &operator=(StateManager &&)      = delete;

        /// @brief Runs the state update routine.
        static void update(const sdl2::Input &input);

        /// @brief Runs the state rendering routine(s);
        static void render(sdl2::Renderer &renderer) noexcept;

        /// @brief Returns whether the back of the vector is a closable state.
        static bool back_is_closable() noexcept;

        /// @brief Pushes a new state to the state vector.
        /// @param newState Shared_ptr to state to push.
        static void push_state(std::shared_ptr<BaseState> newState);

    private:
        /// @brief Private constructor so no constructing.
        StateManager() = default;

        /// @brief Returns a reference to the instance of StateManger.
        /// @return Reference to state manager.
        static StateManager &get_instance();

        /// @brief This is the vector that holds the pointers to the states.
        std::vector<std::shared_ptr<BaseState>> m_stateVector;
};
