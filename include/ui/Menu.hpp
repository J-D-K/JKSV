#pragma once
#include "sdl.hpp"
#include "ui/BoundingBox.hpp"
#include "ui/ColorMod.hpp"
#include "ui/Element.hpp"
#include "ui/TextScroll.hpp"
#include "ui/Transition.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ui
{
    /// @brief Text based menu class.
    class Menu : public ui::Element
    {
        public:
            /// @brief Menu constructor.
            /// @param x X coordinate to render to.
            /// @param y Y coordinate to render to.
            /// @param width Width of the menu options in pixels.
            /// @param fontSize Size of the font in pixels to use.
            /// @param renderTargetHeight Height of the render target to calculate option height and scrolling.
            Menu(int x, int y, int width, int fontSize, int renderTargetHeight);

            /// @brief Creates and returns a new ui::Menu instance.
            static inline std::shared_ptr<ui::Menu> create(int x, int y, int width, int fontSize, int renderTargetHeight)
            { return std::make_shared<ui::Menu>(x, y, width, fontSize, renderTargetHeight); }

            /// @brief Runs the update routine.
            /// @param hasFocus Whether or not the calling state has focus.
            void update(const sdl2::Input &input, bool HasFocus) override;

            /// @brief Renders the menu.
            /// @param target Target to render to.
            /// @param hasFocus Whether or not the calling state has focus.
            void render(sdl2::Renderer &renderer, bool hasFocus) override;

            /// @brief Adds an option to the menu.
            /// @param newOption Option to add to menu.
            void add_option(std::string_view newOption);

            /// @brief Adds an option to the menu. Moves the string instead of creating a new one.
            void add_option(std::string &newOption);

            /// @brief Allows updating and editing the option.
            /// @param newOption Option to change text to.
            void edit_option(int index, std::string_view newOption);

            /// @brief Returns the index of the currently selected menu option.
            /// @return Index of currently selected option.
            int get_selected() const noexcept;

            /// @brief Sets the selected item.
            /// @param selected Value to set selected to.
            void set_selected(int selected);

            /// @brief Updates the X render coordinate.
            void set_x(int x) noexcept;

            /// @brief Updates the Y render coordinate.
            void set_y(int y) noexcept;

            /// @brief This is a workaround function until I find something better.
            /// @param width New width of the menu in pixels.
            void set_width(int width) noexcept;

            /// @brief Returns if the menu has no options.
            bool is_empty() const noexcept;

            /// @brief Plays the cursor sound effect since it can be conditional.
            void play_sound() noexcept;

            /// @brief Resets the menu and returns it to an empty, default state.
            void reset(bool full = true);

        protected:
            /// @brief X coordinate menu is rendered to.
            double m_x{};

            /// @brief Y coordinate menu is rendered to.
            double m_y{};

            /// @brief This is used for scrolling the menu.
            ui::Transition m_transition{};

            /// @brief Currently selected option.
            int m_selected{};

            /// @brief Height of options in pixels.
            int m_optionHeight{};

            /// @brief Target options are rendered to.
            sdl2::SharedTexture m_optionTarget{};

            /// @brief Bounding box for the selected option.
            std::shared_ptr<ui::BoundingBox> m_boundingBox{};

        private:
            /// @brief This to preserve the original Y coordinate passed.
            double m_originalY{};

            /// @brief Maximum number of display options render target can show.
            int m_maxDisplayOptions{};

            /// @brief How many options before scrolling happens.
            int m_scrollLength{};

            /// @brief Width of the menu in pixels.
            int m_width{};

            /// @brief Font size in pixels.
            int m_fontSize{};

            /// @brief The Y coord text is rendered to on that target.
            int m_textY{};

            /// @brief Vertical size of the destination render target in pixels.
            int m_renderTargetHeight{};

            /// @brief Vector of options.
            std::vector<std::string> m_options{};

            /// @brief Text scroll for when the current option is too long to on screen.
            std::shared_ptr<ui::TextScroll> m_optionScroll{};

            /// @brief Font used to render the text.
            sdl2::SharedFont m_font{};

            /// @brief The sound played when the selected/cursor moves.
            static inline sdl2::SharedSound sm_cursor{};

            /// @brief Calculates the alignment variables.
            void calculate_alignments() noexcept;

            /// @brief Initializes transition.
            void initialize_transition() noexcept;

            /// @brief Creates the option target for the menu.
            void initialize_option_target();

            /// @brief Initializes the UI elements used within the menu.
            void initialize_ui_elements();

            /// @brief Ensures the menu cursor sound is loaded.
            void initialize_sounds();

            /// @brief Updates the text scroll for the currently highlighted option.
            void update_scroll_text();

            /// @brief Updates the menu's vertical scrolling.
            void update_scrolling();

            /// @brief Handles the menu's input routine.
            void handle_input(const sdl2::Input &input);
    };
} // namespace ui
