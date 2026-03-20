#include "graphics/gfxutil.hpp"

#include "graphics/ScopedRender.hpp"
#include "mathutil.hpp"
#include "stringutil.hpp"

sdl2::SharedTexture gfxutil::create_generic_icon(sdl2::Renderer &renderer,
                                                 std::string_view text,
                                                 int fontSize,
                                                 SDL_Color background,
                                                 SDL_Color textColor)
{
    // Icon dimensions.
    static constexpr int ICON_WIDTH  = 256;
    static constexpr int ICON_HEIGHT = 256;

    // Create a new font to render with.
    const std::string fontName = stringutil::get_formatted_string("IconFont%i", fontSize);
    sdl2::SharedFont font      = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(fontName, fontSize);

    // Center our text.
    const int textWidth = font->get_text_width(text);
    const int textX     = math::Util<int>::center_within(ICON_WIDTH, textWidth);
    const int textY     = math::Util<int>::center_within(ICON_HEIGHT, fontSize);

    // Create the icon.
    sdl2::SharedTexture icon =
        sdl2::TextureManager::create_load_resource(text, ICON_WIDTH, ICON_HEIGHT, SDL_TEXTUREACCESS_TARGET);

    {
        // Target the icon we just created.
        graphics::ScopedRender scopedRender(renderer, icon);

        // Render.
        renderer.frame_begin(background);
        font->render_text(textX, textY, textColor, text);
    }

    // Return.
    return icon;
}
