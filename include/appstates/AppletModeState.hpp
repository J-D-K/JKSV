#pragma once

#include "appstates/BaseState.hpp"

#include <memory>

/// @brief This is the state that's pushed when applet mode is detected because I'm tired of people complaining.
class AppletModeState final : public BaseState
{
    public:
        /// @brief Constructor.
        AppletModeState();

        /// @brief Inline factory function.
        static inline std::shared_ptr<AppletModeState> create() { return std::make_shared<AppletModeState>(); }

        /// @brief Runs the update routine. Basically does nothing.
        void update(const sdl2::Input &input) override;

        /// @brief Renders the message to the screen.
        void render(sdl2::Renderer &renderer) override;

    private:
        /// @brief Pointer to the string rendered to the screen.
        const char *m_appletModeString{};

        /// @brief Fetches the string to render to the screen.
        void get_applet_mode_string() noexcept;
};