#pragma once

#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "data/data.hpp"
#include "fslib.hpp"
#include "remote/remote.hpp"
#include "sdl.hpp"
#include "sys/sys.hpp"
#include "ui/ui.hpp"

#include <memory>
#include <mutex>

/// @brief This is the state where the user can backup and restore saves.
class BackupMenuState final : public BaseState
{
    public:
        /// @brief Creates a new backup selection state.
        /// @param user Pointer to currently selected user.
        /// @param titleInfo Pointer to titleInfo of selected title.
        /// @param saveInfo Pointer to the target save info.
        BackupMenuState(data::User *user, data::TitleInfo *titleInfo, const FsSaveDataInfo *saveInfo);

        /// @brief Creates and returns a new BackupMenuState.
        static inline std::shared_ptr<BackupMenuState> create(data::User *user,
                                                              data::TitleInfo *titleInfo,
                                                              const FsSaveDataInfo *saveInfo)
        { return std::make_shared<BackupMenuState>(user, titleInfo, saveInfo); }

        /// @brief Creates and pushes a new BackupMenuState to the vector.
        static inline std::shared_ptr<BackupMenuState> create_and_push(data::User *user,
                                                                       data::TitleInfo *titleInfo,
                                                                       const FsSaveDataInfo *saveInfo)
        {
            auto newState = BackupMenuState::create(user, titleInfo, saveInfo);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Required. Inherited virtual function from AppState.
        void update(const sdl2::Input &input) override;

        /// @brief Required. Inherited virtual function from AppState.
        void render(sdl2::Renderer &renderer) override;

        /// @brief Refreshes the directory listing and menu.
        void refresh();

        /// @brief Allows a spawned task to tell this class that it wrote save data to the system.
        void save_data_written();

        // clang-format off
        /// @brief Types of entries corresponding to the menu.
        enum class MenuEntryType : uint8_t
        {
            Null,
            Local,
            Remote
        };

        /// @brief The menu entry. Stores the type and the index in the respective target (remote/local)
        struct MenuEntry
        {
            MenuEntryType type{};
            int index{};
        };

        /// @brief Datastruct passed to tasks.
        struct DataStruct : sys::Task::DataStruct
        {
            /// @brief Pointer to the target user.
            data::User *user{};

            /// @brief Pointer to the target title.
            data::TitleInfo *titleInfo{};

            /// @brief Pointer to the data for the save.
            const FsSaveDataInfo *saveInfo{};

            /// @brief Pointer to the base path of the instance. Used for constructing auto backups (cause it's easier)
            fslib::Path *basePath{};

            /// @brief Path. This changes according to the context used.
            fslib::Path path{}; 

            /// @brief Name to use when the file is uploaded remotely.
            std::string remoteName{};

            /// @brief This is set and used when something is being targetted remotely.
            remote::Item *remoteItem{}; 

            /// @brief This is a pointer to the backup menu instance so refresh() and save_data_written() can be called.
            BackupMenuState *spawningState{};

            /// @brief Signals whether or not the backup fuctions should kill the task when finished. This is so they can be reused.
            bool killTask = false; 
        };
        // clang-format on

        /// @brief This makes this easier to work with and type.
        using TaskData = std::shared_ptr<BackupMenuState::DataStruct>;

    private:
        /// @brief Pointer to current user.
        data::User *m_user{};

        /// @brief Pointer to data for selected title.
        data::TitleInfo *m_titleInfo{};

        /// @brief Pointer to the save info. This cleans and simplifies code in other places.
        const FsSaveDataInfo *m_saveInfo{};

        /// @brief Save data type we're working with.
        FsSaveDataType m_saveType{};

        /// @brief Path to the target directory of the title.
        fslib::Path m_directoryPath{};

        /// @brief Directory listing of the above.
        fslib::Directory m_directoryListing{};

        /// @brief Remote storage listing of the current parent.
        remote::Storage::DirectoryListing m_remoteListing{};

        /// @brief This is the scrolling text at the top.
        std::shared_ptr<ui::TextScroll> m_titleScroll{};

        /// @brief Variable that saves whether or not the filesystem has data in it.
        bool m_saveHasData{};

        /// @brief Data struct passed to functions.
        std::shared_ptr<BackupMenuState::DataStruct> m_dataStruct{};

        /// @brief This keeps track of the properties of the entries in the menu.
        std::vector<BackupMenuState::MenuEntry> m_menuEntries{};

        /// @brief This is a pointer to the control guide string.
        const char *m_controlGuide{};

        /// @brief The width of the panels. This is set according to the control guide text.
        static inline int sm_panelWidth{};

        /// @brief The menu used by all instances of BackupMenuState.
        static inline std::shared_ptr<ui::Menu> sm_backupMenu{};

        /// @brief Prevents this and threads calling refresh() from causing races.
        static inline std::mutex sm_menuMutex{};

        /// @brief The slide out panel used by all instances of BackupMenuState.
        static inline std::shared_ptr<ui::SlideOutPanel> sm_slidePanel{};

        /// @brief Inner render target so the menu only renders to a certain area.
        static inline sdl2::SharedTexture sm_menuRenderTarget{};

        /// @brief Font used for rendering text.
        static inline sdl2::SharedFont sm_font{};

        /// @brief Initializes the static members all instances share if they haven't been already.
        void initialize_static_members();

        /// @brief Checks for and tries to create the target directory if it hasn't been already.
        void ensure_target_directory();

        /// @brief Initializes the struct passed to tasks.
        void initialize_task_data();

        /// @brief Init's the string at the top of the backupmenu.
        void initialize_info_string();

        /// @brief Checks to see if the save data is empty.
        void save_data_check();

        /// @brief Ensures the remote storage is initalized and pointing to the right place.
        void initialize_remote_storage();

        /// @brief This is the function called when New Backup is selected.
        void name_and_create_backup(const sdl2::Input &input);

        /// @brief This is the function called when a backup is selected to be overwritten.
        void confirm_overwrite();

        /// @brief This function is called to confirm restoring a backup.
        void confirm_restore();

        /// @brief Function called to confirm deleting a backup.
        void confirm_delete();

        /// @brief Uploads the currently selected backup to the remote storage.
        void upload_backup();

        /// @brief Just creates the pop-up that says Save is empty or w/e.
        void pop_save_empty();

        /// @brief Performs some operations and then marks the state for purging.
        void deactivate_state();

        /// @brief Inline. Returns if the user is "system" type.
        inline bool user_is_system()
        {
            const uint8_t saveType = m_user->get_account_save_type();
            return saveType == FsSaveDataType_System || saveType == FsSaveDataType_SystemBcat;
        }
};
