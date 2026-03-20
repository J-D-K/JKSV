#pragma once
#include "StateManager.hpp"
#include "appstates/BaseState.hpp"
#include "fslib.hpp"
#include "sdl.hpp"
#include "ui/ui.hpp"

#include <string_view>

class FileModeState final : public BaseState
{
    public:
        /// @brief Constructs a new FileModeState.
        FileModeState(std::string_view mountA, std::string_view mountB, int64_t journalSize = 0, bool isSystem = false);

        static inline std::shared_ptr<FileModeState> create(std::string_view mountA,
                                                            std::string_view mountB,
                                                            int64_t journalSize = 0,
                                                            bool isSystem       = false)
        { return std::make_shared<FileModeState>(mountA, mountB, journalSize, isSystem); }

        static inline std::shared_ptr<FileModeState> create_and_push(std::string_view mountA,
                                                                     std::string_view mountB,
                                                                     int64_t journalSize = 0,
                                                                     bool isSystem       = false)
        {
            auto newState = std::make_shared<FileModeState>(mountA, mountB, journalSize, isSystem);
            StateManager::push_state(newState);
            return newState;
        }

        /// @brief Update override.
        void update(const sdl2::Input &input) override;

        /// @brief Render override.
        void render(sdl2::Renderer &renderer) override;

        /// @brief This thing is a headache without this.
        friend class FileOptionState;

    private:
        /// @brief States the browser can be in.
        enum class State : uint8_t
        {
            Rising,
            Open,
            Dropping
        };

        /// @brief This is to make the target code easier to understand and read.
        // I didn't know what else to name these either...
        enum class Target : uint8_t
        {
            MountA,
            MountB
        };

        /// @brief These store the mount points to close the filesystems upon construction.
        std::string m_mountA{};
        std::string m_mountB{};

        /// @brief These are the actual target paths.
        fslib::Path m_pathA{};
        fslib::Path m_pathB{};

        /// @brief Directory listings for each respective path.
        fslib::Directory m_dirA{};
        fslib::Directory m_dirB{};

        /// @brief Menus for the directory listings.
        std::shared_ptr<ui::Menu> m_dirMenuA{};
        std::shared_ptr<ui::Menu> m_dirMenuB{};

        /// @brief Stores whether the instance is dealing with sensitive data.
        bool m_isSystem{};

        /// @brief Stores the config setting from config to allow writing to the sensitive parts of the system.
        bool m_allowSystem{};

        /// @brief Stores the size for committing data (if needed) to mountA.
        int64_t m_journalSize{};

        /// @brief Controls which menu/filesystem is currently targetted.
        FileModeState::Target m_target{};

        /// @brief Transition for the pop-up effect.
        ui::Transition m_transition{};

        /// @brief Stores the current state.
        FileModeState::State m_state{};

        /// @brief Frame shared by all instances.
        static inline std::shared_ptr<ui::Frame> sm_frame{};

        /// @brief This is the render target the browsers are rendered to.
        static inline sdl2::SharedTexture sm_renderTarget{};

        /// @brief Control guide shared by all instances.
        static inline std::shared_ptr<ui::ControlGuide> sm_controlGuide{};

        /// @brief Initializes the members shared by all instances of FileModeState.
        void initialize_static_members();

        /// @brief Creates valid paths with the mounts passed.
        void initialize_paths();

        /// @brief Ensures the menus are allocated and setup properly.
        void initialize_menus();

        /// @brief Loads the current directory listings and menus.
        void initialize_directory_menu(const fslib::Path &path, fslib::Directory &directory, ui::Menu &menu);

        /// @brief Updates the current Y coordinate of the dialog.
        void update_y() noexcept;

        /// @brief Handles input.
        void update_handle_input(const sdl2::Input &input) noexcept;

        /// @brief Handles changing the current directory or opening the options.
        void enter_selected(fslib::Path &path, fslib::Directory &directory, ui::Menu &menu);

        /// @brief Opens the little option pop-up thingy.
        void open_option_menu(fslib::Directory &directory, ui::Menu &menu);

        /// @brief Changes the current target/controllable menu.
        void change_target() noexcept;

        /// @brief Function executed when '..' is selected.
        void up_one_directory(fslib::Path &path, fslib::Directory &directory, ui::Menu &menu);

        /// @brief Appends the entry passed to the path passed and "enters" the directory.
        /// @param path Working path.
        /// @param directory Working directory.
        /// @param menu Working menu.
        /// @param entry Target entry.
        void enter_directory(fslib::Path &path,
                             fslib::Directory &directory,
                             ui::Menu &menu,
                             const fslib::DirectoryEntry &entry);

        /// @brief Returns a reference to the currently active menu.
        ui::Menu &get_source_menu() noexcept;

        /// @brief Returns a reference to the currently inactive menu.
        ui::Menu &get_destination_menu() noexcept;

        /// @brief Returns a reference to the current "active" path.
        fslib::Path &get_source_path() noexcept;

        /// @brief Returns a reference to the currently "inactive" path.
        fslib::Path &get_destination_path() noexcept;

        /// @brief Returns a reference to the "active" directory.
        fslib::Directory &get_source_directory() noexcept;

        /// @brief Returns a reference ot the currently "inactive" directory.
        fslib::Directory &get_destination_directory() noexcept;

        /// @brief Returns whether or not a path is at the root.
        inline bool path_is_root(fslib::Path &path) const { return std::char_traits<char>::length(path.get_path()) <= 1; }

        /// @brief Closes the filesystems passed and deactivates the state.
        void deactivate_state() noexcept;
};