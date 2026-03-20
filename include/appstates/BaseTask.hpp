#pragma once
#include "appstates/BaseState.hpp"
#include "sdl.hpp"
#include "sys/sys.hpp"
#include "ui/ColorMod.hpp"

#include <array>
#include <memory>

/// @brief Normally, I wouldn't do this, but this holds a single function both TaskState and ProgressState share...
class BaseTask : public BaseState
{
    public:
        /// @brief Constructor. Starts the glyph timer and sets AppState to not allow closing.
        BaseTask();

        /// @brief Runs the update routine for rendering the loading glyph animation.
        /// @param
        virtual void update(const sdl2::Input &input) = 0;

        /// @brief Virtual render function.
        virtual void render(sdl2::Renderer &renderer) = 0;

    protected:
        /// @brief Underlying system task. This needs to be allocated by the derived classes.
        std::unique_ptr<sys::Task> m_task{};

        /// @brief Updates the loading glyph animation.
        void update_loading_glyph();

        /// @brief Displays the "can't quit JKSV" when plus is pressed.
        void pop_on_plus(const sdl2::Input &input);

        /// @brief This function renders the loading glyph in the bottom left corner.
        /// @note This is mostly just so users don't think JKSV has frozen when operations take a long time.
        void render_loading_glyph();

    private:
        /// @brief This is the current frame of the loading glyph animation.
        int m_currentFrame{};

        /// @brief This is the timer used for changing the current glyph/frame of the animation.
        sys::Timer m_frameTimer{};

        /// @brief This is used to give the animation its pulsing color.
        ui::ColorMod m_colorMod{};

        /// @brief This is a pointer to the string popped when plus is pressed.
        const char *m_popUnableExit{};

        /// @brief This array holds the glyphs of the loading sequence. I think it's from the Wii?
        static inline constexpr std::array<std::string_view, 8> sm_glyphArray =
            {"\ue020", "\ue021", "\ue022", "\ue023", "\ue024", "\ue025", "\ue026", "\ue027"};

        /// @brief Font for rendering the glyph.
        static inline sdl2::SharedFont sm_font{};

        /// @brief Ensures the font is loaded.
        void initialize_static_members();
};
