#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "sdl.hpp"
#include "ui/ui.hpp"

#include <memory>
#include <vector>

class BlacklistEditState final : public BaseState
{
    public:
        /// @brief Constructor. Loads the blacklist and ensures the panel is allocated.S
        BlacklistEditState();

        /// @brief Creates and returns a new state.
        static inline std::shared_ptr<BlacklistEditState> create() { return std::make_shared<BlacklistEditState>(); }

        /// @brief Creates, pushes, then returns a new blacklist edit states.
        static inline std::shared_ptr<BlacklistEditState> create_and_push()
        {
            auto newState = BlacklistEditState::create();
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Update override.
        void update(const sdl2::Input &input) override;

        /// @brief Render override.
        void render(sdl2::Renderer &renderer) override;

    private:
        /// @brief Local copy of the blacklist.
        std::vector<uint64_t> m_blacklist{};

        /// @brief Menu with blacklisted titles.
        std::shared_ptr<ui::Menu> m_blacklistMenu{};

        /// @brief This panel is shared by all instances.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};

        /// @brief Inits ^.
        void initialize_static_members();

        /// @brief Loads the blacklist from config.
        void load_blacklist();

        /// @brief Fills the menu with the titles blacklisted.
        void initialize_menu();

        /// @brief Updates the menu.
        void refresh_menu();

        /// @brief Removes the selected title from the blacklist and refreshes everything.
        void remove_from_blacklist();

        /// @brief Performs some operations and marks the state for purging.
        void deactivate_state();
};
