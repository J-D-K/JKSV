#pragma once
#include "sdl.hpp"
#include "ui/Transition.hpp"

namespace ui
{
    /// @brief Tile for Titleview.
    class TitleTile final
    {
        public:
            /// @brief Constructor.
            /// @param isFavorite Whether the title is a favorite and should have the little heart rendered.
            /// @param icon Shared texture pointer to the icon.
            TitleTile(bool isFavorite, int index, sdl2::SharedTexture &icon);

            /// @brief Runs the update routine.
            /// @param isSelected Whether or not the tile is selected and needs to expand.
            void update(bool isSelected);

            /// @brief Runs the render routine.
            /// @param target Target to render to.
            /// @param x X coordinate to render to.
            /// @param y Y coordinate to render to.
            void render(int x, int y);

            /// @brief Resets the width and height of the tile.
            void reset() noexcept;

            /// @brief Returns the render width in pixels.
            /// @return Render width.
            int get_width() const noexcept;

            /// @brief Returns the render height in pixels.
            /// @return Render height.
            int get_height() const noexcept;

        private:
            /// @brief Transition for the tile select/deselect.
            ui::Transition m_transition{};

            /// @brief Whether or not the title is a favorite.
            bool m_isFavorite{};

            /// @brief Title's icon texture.
            sdl2::SharedTexture m_icon{};

            /// @brief Font used for rendering the heart over the icon.
            static inline sdl2::SharedFont sm_heartFont{};

            /// @brief Ensures the font is loaded.,
            void initialize_static_members();
    };
} // namespace ui
