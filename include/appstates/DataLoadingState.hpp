#pragma once
#include "StateManager.hpp"
#include "appstates/BaseTask.hpp"
#include "data/DataContext.hpp"
#include "sdl.hpp"
#include "sys/OpTimer.hpp"

#include <functional>
#include <vector>

class DataLoadingState final : public BaseTask
{
    public:
        /// @brief This is a definition for functions that are called at destruction.
        using DestructFunction = std::function<void()>;

        DataLoadingState(data::DataContext &context,
                         sdl2::Renderer &renderer,
                         DestructFunction destructFunction,
                         sys::threadpool::JobFunction function,
                         sys::Task::TaskData taskData);

        static inline std::shared_ptr<DataLoadingState> create(data::DataContext &context,
                                                               sdl2::Renderer &renderer,
                                                               DestructFunction destructFunction,
                                                               sys::threadpool::JobFunction function,
                                                               sys::Task::TaskData taskData)
        { return std::make_shared<DataLoadingState>(context, renderer, destructFunction, function, taskData); }

        static inline std::shared_ptr<DataLoadingState> create_and_push(data::DataContext &context,
                                                                        sdl2::Renderer &renderer,
                                                                        DestructFunction destructFunction,
                                                                        sys::threadpool::JobFunction function,
                                                                        sys::Task::TaskData taskData)
        {
            auto newState = DataLoadingState::create(context, renderer, destructFunction, function, taskData);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Update override.
        void update(const sdl2::Input &input) override;

        /// @brief Updates the loading glyph.
        void sub_update() override;

        /// @brief Render override.
        void render(sdl2::Renderer &renderer) override;

    private:
        /// @brief Reference to the data context to run post-init operations.
        data::DataContext &m_context;

        /// @brief Reference to SDL2 renderer for loading icons.
        sdl2::Renderer &m_renderer;

        /// @brief X coord of the status text.
        int m_statusX{};

        /// @brief The functions called upon destruction.
        DestructFunction m_destructFunction{};

        /// @brief Icon displayed in the center of the screen.
        static inline sdl2::SharedTexture sm_jksvIcon{};

        /// @brief Font used for rendering the status.
        static inline sdl2::SharedFont sm_font{};

        /// @brief Loads the icon if it hasn't been already.
        void initialize_static_members();

        /// @brief
        void deactivate_state();
};
