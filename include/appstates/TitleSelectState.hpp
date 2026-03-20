#pragma once
#include "StateManager.hpp"
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/TitleView.hpp"

/// @brief Title select state with icon tiles.
class TitleSelectState final : public TitleSelectCommon
{
    public:
        /// @brief Constructs new title select state.
        /// @param user User the state "belongs" to.
        TitleSelectState(data::User *user);

        /// @brief Returns a new TitleSelect state.
        static inline std::shared_ptr<TitleSelectState> create(data::User *user)
        { return std::make_shared<TitleSelectState>(user); }

        /// @brief Creates, pushes, and returns a new TitleSelectState.
        static inline std::shared_ptr<TitleSelectState> create_and_push(data::User *user)
        {
            auto newState = TitleSelectState::create(user);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Runs the update routine.
        void update(const sdl2::Input &input) override;

        /// @brief Runs the render routine.
        void render(sdl2::Renderer &renderer) override;

        /// @brief Refreshes the view.
        void refresh() override;

    private:
        /// @brief Pointer to the user the view belongs to.
        data::User *m_user{};

        /// @brief Target to render to.
        sdl2::SharedTexture m_renderTarget{};

        /// @brief Tiled title selection view.
        std::shared_ptr<ui::TitleView> m_titleView{};

        /// @brief Checks if the user still has any games left to list. This is a safety measure.
        bool title_count_check();

        /// @brief Creates and pushes the backup menu state.
        void create_backup_menu();

        /// @brief Creates and pushes the title option state.
        void create_title_option_menu();

        /// @brief Resets and marks the state for purging.
        void deactivate_state();

        /// @brief Adds or removes the currently highlighted game from the favorite vector.
        void add_remove_favorite();
};
