#pragma once
#include "data/data.hpp"
#include "sdl.hpp"
#include "ui/BoundingBox.hpp"
#include "ui/ColorMod.hpp"
#include "ui/Element.hpp"
#include "ui/TitleTile.hpp"
#include "ui/Transition.hpp"

#include <vector>

namespace ui
{
    /// @brief Presents a grid of icons to select a title from.
    class TitleView final : public ui::Element
    {
        public:
            /// @brief Creates a title view using passed user pointer.
            /// @param user User to use.
            TitleView(data::User *user);

            static inline std::shared_ptr<ui::TitleView> create(data::User *user)
            { return std::make_shared<ui::TitleView>(user); }

            /// @brief Runs the update routine.
            /// @param hasFocus Whether the calling state has focus.
            void update(const sdl2::Input &input, bool hasFocus) override;

            /// @brief Runs the render routine.
            /// @param target Target to render to.
            /// @param hasFocus Whether or not the calling state has focus.
            void render(sdl2::Renderer &renderer, bool hasFocus) override;

            /// @brief Returns index of the currently selected tile.
            /// @return Index of currently selected tile.
            int get_selected() const noexcept;

            /// @brief Sets the currently selected item.
            void set_selected(int selected) noexcept;

            /// @brief Forces a refresh of the view.
            void refresh();

            /// @brief Resets the view to its default, empty state.
            void reset();

            /// @brief Plays the cursor sound.
            void play_sound() noexcept;

        private:
            /// @brief Pointer to user passed.
            data::User *m_user{};

            /// @brief Transition used for animating the scrolling.
            ui::Transition m_transition{};

            /// @brief Currently highlighted/selected title.
            int m_selected{};

            /// @brief X coordinate of the currently selected tile so it can be rendered over top of the rest.
            int m_selectedX{};

            /// @brief Y coordinate. Same as above.
            int m_selectedY{};

            /// @brief Color mod for bounding/selection pulse.
            ui::ColorMod m_colorMod{};

            /// @brief Vector of selection tiles.
            std::vector<ui::TitleTile> m_titleTiles{};

            /// @brief Bounding box rendered around the selected title.
            std::shared_ptr<ui::BoundingBox> m_bounding{};

            /// @brief Sound that is played when the selected title changes. This is shared with the menu code.
            static inline sdl2::SharedSound sm_cursor{};

            /// @brief Ensures static members are initialized properly.
            void initialize_static_members();

            /// @brief Performs the input routine.
            void handle_input(const sdl2::Input &input);

            /// @brief Performs the "scrolling" routine.
            void handle_scrolling();

            /// @brief Updates the tiles.
            void update_tiles();
    };
} // namespace ui
