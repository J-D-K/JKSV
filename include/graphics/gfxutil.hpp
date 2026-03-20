#pragma once
#include "sdl.hpp"

#include <string_view>
// This file contains basic graphics functions various parts of JKSV use.

namespace gfxutil
{
    /// @brief Generates a generic icon for saves and titles that lack one.
    /// @param text Text to be centered and rendered to the icon.
    /// @param fontSize Size of the font to use.
    /// @param background Background color to use.
    /// @param foreground Color to use to render the text.
    /// @return sdl::SharedTexture of the icon.
    sdl2::SharedTexture create_generic_icon(sdl2::Renderer &renderer,
                                            std::string_view text,
                                            int fontSize,
                                            SDL_Color background,
                                            SDL_Color textColor);
} // namespace gfxutil
