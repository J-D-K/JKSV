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
        ExtrasMenuState(sdl2::Renderer &renderer);

        /// @brief Returns a new ExtrasMenuState
        static inline std::shared_ptr<ExtrasMenuState> create(sdl2::Renderer &renderer)
        { return std::make_shared<ExtrasMenuState>(renderer); }

        /// @brief Updates the menu.
        void update(const sdl2::Input &input) override;

        /// @brief Sub-update routine.
        void sub_update() override;

        /// @brief Renders the menu to screen.
        void render(sdl2::Renderer &renderer) override;

    private:
        /// @brief Reference to renderer for launching refresh.
        sdl2::Renderer &m_renderer;

        /// @brief Menu
        std::shared_ptr<ui::Menu> m_extrasMenu{};

        /// @brief Render target for menu.
        sdl2::SharedTexture m_renderTarget{};

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
