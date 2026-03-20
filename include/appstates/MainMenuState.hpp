#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/ControlGuide.hpp"
#include "ui/IconMenu.hpp"

#include <memory>

/// @brief The main
class MainMenuState final : public BaseState
{
    public:
        /// @brief Creates and initializes the main menu.
        MainMenuState(sdl2::Renderer &renderer);

        /// @brief Returns a new MainMenuState
        static inline std::shared_ptr<MainMenuState> create(sdl2::Renderer &renderer)
        { return std::make_shared<MainMenuState>(renderer); }

        /// @brief Creates and returns a new MainMenuState. Pushes it automatically.
        static inline std::shared_ptr<MainMenuState> create_and_push(sdl2::Renderer &renderer)
        {
            auto newState = MainMenuState::create(renderer);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Runs update routine.
        void update(const sdl2::Input &input) override;

        /// @brief Runs the sub-update routine.
        void sub_update() override;

        /// @brief Renders menu to screen.
        void render(sdl2::Renderer &renderer) override;

        /// @brief Allows the update task to signal it found an update.
        void signal_update_found();

        /// @brief Signals to
        static void initialize_view_states();

        /// @brief Calls refresh on on view states in the vector.
        static void refresh_view_states();

        // clang-format off
        struct DataStruct : sys::Task::DataStruct
        {
            data::UserList userList;
            MainMenuState *spawningState{};
        };
        // clang-format on

    private:
        /// @brief Render target this state renders to.
        sdl2::SharedTexture m_renderTarget{};

        /// @brief The background gradient.
        sdl2::SharedTexture m_background{};

        /// @brief Icon for the settings option,
        sdl2::SharedTexture m_settingsIcon{};

        /// @brief Icon for the extras option.
        sdl2::SharedTexture m_extrasIcon{};

        /// @brief Special menu type that uses icons.
        std::shared_ptr<ui::IconMenu> m_mainMenu{};

        /// @brief Control guide in the bottom right.
        std::shared_ptr<ui::ControlGuide> m_controlGuide{};

        /// @brief This is the data struct passed to tasks.
        std::shared_ptr<MainMenuState::DataStruct> m_dataStruct{};

        /// @brief Allows the check_for_update task to signal that an update was found (to prevent corrupted textures!)
        std::atomic_bool m_updateFound{};

        /// @brief Records the size of the sm_users vector.
        static inline int sm_userCount{};

        /// @brief This is the list of user pointers from data.
        static inline data::UserList sm_users{};

        /// @brief This is the pointer to the settings state.
        static inline std::shared_ptr<BaseState> sm_settingsState{};

        /// @brief This is the pointer to the extras state.
        static inline std::shared_ptr<BaseState> sm_extrasState{};

        /// @brief This is the vector of title selection states.
        static inline std::vector<std::shared_ptr<BaseState>> sm_states{};

        /// @brief Creates the settings and extras.
        void initialize_settings_extras(sdl2::Renderer &renderer);

        /// @brief Pushes the icons to the main menu.
        void initialize_menu();

        /// @brief Initializes the data struct.
        void initialize_data_struct();

        /// @brief Silently checks for an update in the background.
        void check_for_update();

        /// @brief Pushes the target state to the vector.
        void push_target_state();

        /// @brief Creates the user option state.
        void create_user_options();

        /// @brief Backups up all save data for all users.
        void backup_all_for_all();

        /// @brief Pushes the update notice and allows the user to select whether or not they want to update JKSV.
        void confirm_update();
};
