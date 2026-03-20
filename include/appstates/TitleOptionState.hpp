#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "appstates/TitleSelectCommon.hpp"
#include "data/data.hpp"
#include "ui/ui.hpp"

#include <memory>

class TitleOptionState final : public BaseState
{
    public:
        /// @brief Constructs a new title option state.
        /// @param user Target user.
        /// @param titleInfo Target title.
        TitleOptionState(data::User *user,
                         data::TitleInfo *titleInfo,
                         const FsSaveDataInfo *saveInfo,
                         TitleSelectCommon *titleSelect);

        /// @brief Returns a new TitleOptionState. See constructor.
        static inline std::shared_ptr<TitleOptionState> create(data::User *user,
                                                               data::TitleInfo *titleInfo,
                                                               const FsSaveDataInfo *saveInfo,
                                                               TitleSelectCommon *titleSelect)
        { return std::make_shared<TitleOptionState>(user, titleInfo, saveInfo, titleSelect); }

        /// @brief Creates, pushes, and returns a new TitleOptionState
        static std::shared_ptr<TitleOptionState> create_and_push(data::User *user,
                                                                 data::TitleInfo *titleInfo,
                                                                 const FsSaveDataInfo *saveInfo,
                                                                 TitleSelectCommon *titleSelect)
        {
            auto newState = TitleOptionState::create(user, titleInfo, saveInfo, titleSelect);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Runs update routine.
        void update(const sdl2::Input &input) override;

        /// @brief Handles hiding the panel.
        void sub_update() override;

        /// @brief Runs the render routine.
        void render(sdl2::Renderer &renderer) override;

        /// @brief This function allows tasks to signal to the spawning state to close itself on the next update() call.
        void close_on_update();

        /// @brief Signals to the main thread that a view refresh is required on the next update() call.
        void refresh_required();

        // clang-format off
       struct DataStruct : sys::Task::DataStruct
        {
            data::User *user{};
            data::TitleInfo *titleInfo{};
            const FsSaveDataInfo *saveInfo{};
            TitleOptionState *spawningState{};
            TitleSelectCommon *titleSelect{};
        };
        // clang-format on

    private:
        /// @brief This is just in case the option should only apply to the current user.
        data::User *m_user{};

        /// @brief This is the target title.
        data::TitleInfo *m_titleInfo{};

        /// @brief This makes tasks easier to read and understand.
        const FsSaveDataInfo *m_saveInfo{};

        /// @brief Pointer to the title selection being used for updating.
        TitleSelectCommon *m_titleSelect{};

        /// @brief The struct passed to functions.
        std::shared_ptr<TitleOptionState::DataStruct> m_dataStruct{};

        /// @brief This holds whether or not the state should deactivate itself on the next update loop.
        bool m_exitRequired{};

        /// @brief This stores whether or a not a refresh is required on the next update().
        bool m_refreshRequired{};

        /// @brief Menu used and shared by all instances.
        static inline std::shared_ptr<ui::Menu> sm_titleOptionMenu{};

        /// @brief This is shared by all instances of this class.
        static inline std::unique_ptr<ui::SlideOutPanel> sm_slidePanel{};

        /// @brief Initializes the static members all instances share.
        void initialize_static_members();

        /// @brief Initializes the values of the struct passed to tasks.
        void initialize_data_struct();

        /// @brief Creates and pushes the title information state.
        void create_push_info_state();

        /// @brief Adds the currently highlighted title to the blacklist.
        void add_to_blacklist();

        /// @brief Changes the current output directory for the title locally and remotely (if needed).
        void change_output_directory();

        /// @brief Creates and pushes a new instance of file mode (once I get around to it.).
        void create_push_file_mode();

        /// @brief Deletes all locally stored save backups for the current title.
        void delete_all_local_backups();

        /// @brief Deletes all backups on the remote server.
        void delete_all_remote_backups();

        /// @brief Resets the save data for the current title. Basically wipes it clean.
        void reset_save_data();

        /// @brief Deletes the filesystem from the Switch.
        void delete_save_from_system();

        /// @brief Extends the container to any size desired.
        void extend_save_container();

        /// @brief Exports an SVI file for the current title.
        void export_svi_file();

        /// @brief Performs some operations and marks the state for purging.
        void deactivate_state();
};
