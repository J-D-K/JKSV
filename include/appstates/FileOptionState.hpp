#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "appstates/FileModeState.hpp"
#include "fslib.hpp"
#include "sys/sys.hpp"
#include "ui/ui.hpp"

#include <atomic>

class FileOptionState final : public BaseState
{
    public:
        /// @brief FileOptionState.
        /// @param spawningState Pointer to spawning state to grab its goodies.
        FileOptionState(FileModeState *spawningState);

        /// @brief Inline creation function.
        static inline std::shared_ptr<FileOptionState> create(FileModeState *spawningState)
        { return std::make_shared<FileOptionState>(spawningState); }

        /// @brief Same as above. Pushes state before returning it.
        static inline std::shared_ptr<FileOptionState> create_and_push(FileModeState *spawningState)
        {
            auto newState = FileOptionState::create(spawningState);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Update routine.
        void update(const sdl2::Input &input) override;

        /// @brief Render routine.
        void render(sdl2::Renderer &renderer) override;

        /// @brief Signals to this state to update the source/target menu on the next update() call.
        void update_source();

        /// @brief Signals to this state to update the destination. Very rarely used.
        void update_destination();

        // clang-format off
        struct DataStruct : sys::Task::DataStruct
        {
            fslib::Path sourcePath{};
            fslib::Path destPath{};
            int64_t journalSize{};
            FileOptionState *spawningState{};
        };
        // clang-format on

    private:
        /// @brief States this state can be in.
        enum class State : uint8_t
        {
            Opening,
            Opened,
            Closing
        };

        /// @brief Pointer to spawning FileMode state.
        FileModeState *m_spawningState{};

        /// @brief Stores the target for easier access.
        FileModeState::Target m_target{};

        /// @brief Current state of the state.
        FileOptionState::State m_state{};

        /// @brief Transition.
        ui::Transition m_transition{};

        /// @brief Stores whether or not an update is needed on the next update().
        std::atomic<bool> m_updateSource{};
        std::atomic<bool> m_updateDest{};

        /// @brief This is the data struct passed to tasks.
        std::shared_ptr<FileOptionState::DataStruct> m_dataStruct{};

        /// @brief This is shared by all instances.
        static inline std::shared_ptr<ui::Menu> sm_copyMenu{};

        /// @brief This is shared by all instances.
        static inline std::shared_ptr<ui::DialogBox> sm_dialog{};

        /// @brief Ensures static members of all instances are allocated.
        void initialize_static_members();

        /// @brief Assigns the pointer to this.
        void initialize_data_struct();

        /// @brief Updates the dimensions of the dialog.
        void update_dimensions() noexcept;

        /// @brief Updates and handles input.
        void update_handle_input(const sdl2::Input &input) noexcept;

        /// @brief Updates the FileModeState's source data.
        void update_filemode_source();

        /// @brief Updates the FileModeState's destination data.
        void update_filemode_dest();

        /// @brief Sets up and begins the copy task.
        void copy_target();

        /// @brief Sets up and begins the delete task.
        void delete_target();

        /// @brief Attempts to rename the target.
        void rename_target();

        /// @brief Attempts to create a new directory.
        void create_directory();

        /// @brief Gets the properties of a file/folder.
        void get_show_target_properties();

        /// @brief Closes the dialog.
        void close_dialog() noexcept;

        /// @brief Gets info and creates a MessageState displaying the properties of the target directory.
        void get_show_directory_properties(const fslib::Path &path);

        /// @brief Gets info and creates a message displaying the properties of the target file.
        /// @param path
        void get_show_file_properties(const fslib::Path &path);

        /// @brief Pops the error writing to system message.
        void pop_system_error();

        /// @brief Sets the menu index back to 0 and deactivates the state.
        void deactivate_state();

        /// @brief Returns if the copy/from is allowed before continuing.
        inline bool system_write_check()
        {
            const FileModeState::Target target = m_spawningState->m_target;
            const bool isSystem                = m_spawningState->m_isSystem;
            const bool allowSystem             = m_spawningState->m_allowSystem;

            return target == FileModeState::Target::MountB && isSystem && !allowSystem;
        }

        /// @brief Returns if the operation is allowed.
        inline bool system_operation_check()
        {
            const FileModeState::Target target = m_spawningState->m_target;
            const bool isSystem                = m_spawningState->m_isSystem;
            const bool allowSystem             = m_spawningState->m_allowSystem;

            return target == FileModeState::Target::MountA && isSystem && !allowSystem;
        }
};