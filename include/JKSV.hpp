#pragma once
#include "appstates/BaseState.hpp"
#include "sdl.hpp"

#include <atomic>
#include <memory>
#include <vector>

/// @brief Main application class.
class JKSV
{
    public:
        /// @brief Initializes JKSV. Initializes services.
        JKSV();

        /// @brief Exits services.
        ~JKSV();

        /// @brief Returns if initializing was successful and JKSV is running.
        /// @return True or false.
        bool is_running() const noexcept;

        /// @brief Runs JKSV's update routine.
        void update();

        /// @brief Runs JKSV's render routine.
        void render();

        /// @brief Function to allow tasks to tell JKSV to exit.
        static void request_quit() noexcept;

    private:
        /// @brief SDL2 instance.
        sdl2::SDL2 m_sdl2{};

        /// @brief SDL2 Window.
        sdl2::Window m_window{};

        /// @brief SDL2 Renderer.
        sdl2::Renderer m_renderer{};

        /// @brief SDL2 Audio.
        sdl2::Audio m_audio{};

        /// @brief Not really a part of SDL2, but...
        sdl2::Input m_input{};

        /// @brief Whether or not initialization was successful and JKSV is still running.
        static inline std::atomic_bool sm_isRunning{};

        /// @brief Whether or not to print the translation credits.
        bool m_showTranslationInfo{};

        /// @brief JKSV icon in upper left corner.
        sdl2::SharedTexture m_headerIcon{};

        /// @brief This is used for rendering JKSV.
        sdl2::SharedFont m_titleFont{};

        /// @brief This is used to render the build date.
        sdl2::SharedFont m_buildFont{};

        /// @brief Stores the translation string.
        std::string m_translationInfo{};

        /// @brief Stores the build string.
        std::string m_buildString{};

        /// @brief Sets the system to enable boost mode.
        void set_boost_mode();

        /// @brief Initializes fslib and takes care of a few other things.
        bool initialize_filesystem();

        /// @brief Initializes the services JKSV uses.
        bool initialize_services();

        /// @brief Initializes SDL and loads the header icon.
        bool initialize_sdl();

        // Creates the needed directories on SD.
        bool create_directories();

        /// @brief Retrieves the strings from the map and sets them up for printing.
        void setup_translation_info_strings();

        /// @brief Pushed the applet mode warning pop-up if needed.
        void applet_mode_warning() noexcept;

        /// @brief Renders the base UI.
        void render_base();

        /// @brief Exits all services.
        void exit_services();
};
