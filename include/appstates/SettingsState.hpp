#pragma once
#include "appstates/BaseState.hpp"
#include "sdl.hpp"
#include "ui/ControlGuide.hpp"
#include "ui/Menu.hpp"

/// @brief The state for settings.
class SettingsState final : public BaseState
{
    public:
        /// @brief Constructs a new settings state.
        SettingsState();

        /// @brief Returns a new SettingsState.
        static inline std::shared_ptr<SettingsState> create() { return std::make_shared<SettingsState>(); }

        /// @brief Runs the update routine.
        void update(const sdl2::Input &input) override;

        /// @brief Sub update routine.
        void sub_update() override;

        /// @brief Runs the render routine.
        void render(sdl2::Renderer &renderer) override;

    private:
        /// @brief Menu for selecting and toggling settings.
        std::shared_ptr<ui::Menu> m_settingsMenu{};

        // These are pointers to strings this state uses constantly.
        const char *m_onOff[2]{};
        const char *m_sortTypes[3]{};

        /// @brief Control guide.
        std::shared_ptr<ui::ControlGuide> m_controlGuide{};

        /// @brief Render target to render to.
        sdl2::SharedTexture m_renderTarget{};

        /// @brief Loads the settings menu strings.
        void load_settings_menu();

        /// @brief Loads the on/off and sort type strings.
        void load_extra_strings();

        /// @brief Runs a routine to update the menu strings for the menu.
        void update_menu_options();

        /// @brief Changes the current output directory of JKSV.
        void change_working_directory();

        /// @brief Creates and pushes the blacklist editing panel/state.
        void create_push_blacklist_edit();

        /// @brief Toggles or executes the code to changed the selected menu option.
        void toggle_options();

        /// @brief Calls config::reset_to_default and refreshes the menu.
        void reset_settings();

        /// @brief Creates and pushes the message with the description.
        void create_push_description_message();

        /// @brief Cycles and wraps the Zip compression level.
        void cycle_zip_level();

        /// @brief Cycles and wraps the title sort type.
        void cycle_sort_type();

        /// @brief Toggles JKSM mode and calls the main menu to reinitialize the views.
        void toggle_jksm_mode();

        /// @brief Toggles the trash folder and creates or deletes it if needed.
        void toggle_trash_folder();

        /// @brief Cycles and wraps the animation scaling (1.0 to 4.0);
        void cycle_anim_scaling();

        // Returns On/Off depending on the value passed.
        const char *get_status_text(uint8_t value);

        /// @brief Returns the sort type depending on the value passed.
        const char *get_sort_type_text(uint8_t value);
};
