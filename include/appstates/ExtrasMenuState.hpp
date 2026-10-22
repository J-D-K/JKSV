#pragma once
#include "appstates/BaseState.hpp"
#include "sdl.hpp"
#include "ui/ControlGuide.hpp"
#include "ui/Menu.hpp"

/// @brief Extras menu.
class ExtrasMenuState final : public BaseState
{
    public:
        /// @brief Constructor.
        ExtrasMenuState();

        /// @brief Returns a new ExtrasMenuState
        static inline std::shared_ptr<ExtrasMenuState> create() { return std::make_shared<ExtrasMenuState>(); }

        /// @brief Updates the menu.
        void update() override;

        /// @brief Sub-update routine.
        void sub_update() override;

        /// @brief Renders the menu to screen.
        void render() override;

    private:
        /// @brief Menu
        std::shared_ptr<ui::Menu> m_extrasMenu{};

        /// @brief Index of the "Peer transfer" entry appended to the extras menu.
        int m_peerIndex{7};

        /// @brief Render target for menu.
        sdl::SharedTexture m_renderTarget{};

        /// @brief Control guider for bottom right corner.
        std::shared_ptr<ui::ControlGuide> m_controlGuide{};

        /// @brief Creates and loads the menu strings.
        void initialize_menu();

        /// @brief This function is called when Reinitialize data is selected.
        void reinitialize_data();

        /// @brief Opens an SD to SD file browser.
        void sd_to_sd_browser();

        /// @brief Opens the prodinfo-f to sd.
        void prodinfof_to_sd();

        /// @brief Opens the safe partition to SD.
        void safe_to_sd();

        /// @brief Opens the system partition to SD.
        void system_to_sd();

        /// @brief Opens the user partition to SD.
        void user_to_sd();

        /// @brief Terminates a process.
        void terminate_process();
};
