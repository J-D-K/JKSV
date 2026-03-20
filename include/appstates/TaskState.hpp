#pragma once
#include "StateManager.hpp"
#include "appstates/BaseTask.hpp"
#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
#include "sys/sys.hpp"

#include <switch.h>

/// @brief State that spawns a task and allows updates to be printed to screen.
class TaskState final : public BaseTask
{
    public:
        /// @brief Constructs a new TaskState.
        TaskState(sys::threadpool::JobFunction function, sys::Task::TaskData taskData);

        /// @brief Constructs and returns a TaskState.
        static inline std::shared_ptr<TaskState> create(sys::threadpool::JobFunction function, sys::Task::TaskData taskData)
        { return std::make_shared<TaskState>(function, taskData); }

        /// @brief Constructs, pushes, then returns a new TaskState.
        static inline std::shared_ptr<TaskState> create_and_push(sys::threadpool::JobFunction function,
                                                                 sys::Task::TaskData taskData)
        {
            auto newState = TaskState::create(function, taskData);
            StateManager::push_state(newState);
            return newState;
        }

        static inline std::shared_ptr<TaskState> create_push_fade(sys::threadpool::JobFunction function,
                                                                  sys::Task::TaskData taskData)
        {
            auto newState = TaskState::create(function, taskData);
            FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_BEGIN, colors::ALPHA_FADE_END, newState);
            return newState;
        }

        /// @brief Runs update routine. Waits for thread function to signal finish and deactivates.
        void update(const sdl2::Input &input) override;

        /// @brief Run render routine. Prints m_task's status string to screen, basically.
        /// @param
        void render(sdl2::Renderer &renderer) override;

    private:
        /// @brief Font used for rendering text.
        static inline sdl2::SharedFont sm_font{};

        /// @brief Ensures static members are initialized.
        void initialize_static_members();

        /// @brief Performs some operations and marks the state for deletion.
        void deactivate_state();
};
