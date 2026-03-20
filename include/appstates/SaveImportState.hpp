#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "fslib.hpp"
#include "sys/sys.hpp"
#include "ui/ui.hpp"

#include <memory>

class SaveImportState final : public BaseState
{
    public:
        /// @brief Constructs a new SaveImportState.
        SaveImportState(data::User *user);

        static inline std::shared_ptr<SaveImportState> create(data::User *user)
        { return std::make_shared<SaveImportState>(user); }

        static inline std::shared_ptr<SaveImportState> create_and_push(data::User *user)
        {
            auto newState = SaveImportState::create(user);
            StateManager::push_state(newState);
            return newState;
        }

        void update(const sdl2::Input &input) override;

        void render(sdl2::Renderer &renderer) override;

        // clang-format off
        // Struct used to pass data to the task.
        struct DataStruct : sys::Task::DataStruct
        {
            data::User *user{};
            fslib::Path path{};
        };
        // clang-format on

        /// @brief Makes things easier to read and type.
        using TaskData = std::shared_ptr<SaveImportState::DataStruct>;

    private:
        /// @brief Pointer to the target user.
        data::User *m_user{};

        /// @brief Directory listing of the saves directory.
        fslib::Directory m_saveDir{};

        /// @brief This is used to pass data to the task function.
        static inline SaveImportState::TaskData sm_taskData{};

        /// @brief Menu shared by all instances.
        static inline std::shared_ptr<ui::Menu> sm_saveMenu{};

        /// @brief Panel shared by all instances.
        static inline std::shared_ptr<ui::SlideOutPanel> sm_slidePanel{};

        /// @brief Ensures the menu and panel are initialized correctly.
        void initialize_static_members();

        /// @brief Opens and
        void initialize_directory_menu();

        /// @brief Imports a backup selected from the menu.
        void import_backup();

        /// @brief Cleans up and deactivates the state.
        void deactivate_state();
};
