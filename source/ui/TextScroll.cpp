#include "ui/TextScroll.hpp"

#include "graphics/ScopedRender.hpp"
#include "sdl.hpp"
#include "stringutil.hpp"

namespace
{
    /// @brief This is the number of ticks needed before the text starts scrolling.
    constexpr uint64_t TICKS_SCROLL_TRIGGER = 3000;

    /// @brief This is the number of pixels between the two renderings of the text.
    constexpr int SIZE_TEXT_GAP = 0;
} // namespace

//                      ---- Construction ----

ui::TextScroll::TextScroll(std::string_view text,
                           int x,
                           int y,
                           int width,
                           int height,
                           int fontSize,
                           SDL_Color textColor,
                           SDL_Color clearColor,
                           bool center)
{ TextScroll::initialize(text, x, y, width, height, fontSize, textColor, clearColor, center); }

ui::TextScroll::TextScroll(std::string &text,
                           int x,
                           int y,
                           int width,
                           int height,
                           int fontSize,
                           SDL_Color textColor,
                           SDL_Color clearColor,
                           bool center)
{ TextScroll::initialize(text, x, y, width, height, fontSize, textColor, clearColor, center); }

//                      ---- Public functions ----

void ui::TextScroll::initialize(std::string_view text,
                                int x,
                                int y,
                                int width,
                                int height,
                                int fontSize,
                                SDL_Color textColor,
                                SDL_Color clearColor,
                                bool center)
{
    m_renderX   = x;
    m_renderY   = y;
    m_fontSize  = fontSize;
    m_font      = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(TextScroll::generate_font_name(fontSize), fontSize);
    m_textColor = textColor;
    m_clearColor   = clearColor;
    m_targetWidth  = width;
    m_targetHeight = height;
    m_textY        = (m_targetHeight / 2) - (m_fontSize / 2);
    m_scrollTimer.start(TICKS_SCROLL_TRIGGER);
    m_renderTarget = sdl2::TextureManager::create_load_resource(TextScroll::generate_target_name(),
                                                                m_targetWidth,
                                                                m_targetHeight,
                                                                SDL_TEXTUREACCESS_TARGET);

    TextScroll::set_text(text, center);
}

void ui::TextScroll::initialize(std::string &text,
                                int x,
                                int y,
                                int width,
                                int height,
                                int fontSize,
                                SDL_Color textColor,
                                SDL_Color clearColor,
                                bool center)
{
    m_renderX   = x;
    m_renderY   = y;
    m_fontSize  = fontSize;
    m_font      = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(TextScroll::generate_font_name(fontSize), fontSize);
    m_textColor = textColor;
    m_clearColor   = clearColor;
    m_targetWidth  = width;
    m_targetHeight = height;
    m_textY        = (m_targetHeight / 2) - (m_fontSize / 2);
    m_scrollTimer.start(TICKS_SCROLL_TRIGGER);
    m_renderTarget = sdl2::TextureManager::create_load_resource(TextScroll::generate_target_name(),
                                                                m_targetWidth,
                                                                m_targetHeight,
                                                                SDL_TEXTUREACCESS_TARGET);

    TextScroll::set_text(text, center);
}

std::string_view ui::TextScroll::get_text() const noexcept { return m_text; }

void ui::TextScroll::set_text(std::string_view text, bool center)
{
    m_text                = text;
    m_textWidth           = m_font->get_text_width(m_text);
    m_textScrollTriggered = false;

    if (m_textWidth > m_targetWidth - 16)
    {
        m_textX         = 8;
        m_textScrolling = true;
        m_scrollTimer.restart();
    }
    else if (center)
    {
        m_textX         = (m_targetWidth / 2) - (m_textWidth / 2);
        m_textScrolling = false;
    }
    else
    {
        m_textX         = 8;
        m_textScrolling = false;
    }
}

void ui::TextScroll::set_text(std::string &text, bool center)
{
    m_text                = std::move(text);
    m_textWidth           = m_font->get_text_width(m_text);
    m_textScrollTriggered = false;

    if (m_textWidth > m_targetWidth - 16)
    {
        m_textX         = 8;
        m_textScrolling = true;
        m_scrollTimer.restart();
    }
    else if (center)
    {
        m_textX         = (m_targetWidth / 2) - (m_textWidth / 2);
        m_textScrolling = false;
    }
    else
    {
        m_textX         = 8;
        m_textScrolling = false;
    }
}

void ui::TextScroll::update(const sdl2::Input &input, bool hasFocus)
{
    // I don't think this needs to care about having focus.
    const int invertedWidth    = -(m_textWidth + SIZE_TEXT_GAP);
    const bool scrollTriggered = hasFocus && m_textScrolling && m_scrollTimer.is_triggered();
    const bool keepScrolling   = m_textScrollTriggered && m_textX > invertedWidth;
    const bool scrollFinished  = m_textScrollTriggered && m_textX <= invertedWidth;

    if (scrollTriggered)
    {
        m_textX -= 2;
        m_textScrollTriggered = true;
    }
    else if (keepScrolling) { m_textX -= 2; }
    else if (scrollFinished)
    {
        // This will snap the text back. You can't see it though.
        m_textX               = 8;
        m_textScrollTriggered = false;
        m_scrollTimer.restart();
    }
}

void ui::TextScroll::render(sdl2::Renderer &renderer, bool hasFocus)
{
    // Gap between scrolling text rendering.
    static constexpr int TEXT_GAP = 8;

    {
        // Scoped render to the target.
        graphics::ScopedRender scopedRender{renderer, m_renderTarget};
        renderer.frame_begin(m_clearColor);

        if (!m_textScrolling) { m_font->render_text(m_textX, m_textY, m_textColor, m_text); }
        else
        {
            // We're going to render text twice so it looks like it's scrolling and doesn't end. Ever.
            m_font->render_text(m_textX, m_textY, m_textColor, m_text);
            m_font->render_text(m_textX + m_textWidth + TEXT_GAP, m_textY, m_textColor, m_text);
        }
    }
    m_renderTarget->render(m_renderX, m_renderY);
}

//                      ---- Private Functions ----

std::string ui::TextScroll::generate_target_name()
{
    static int targetID{};
    return stringutil::get_formatted_string("TextScroll_%i", targetID++);
}

std::string ui::TextScroll::generate_font_name(int fontSize)
{ return stringutil::get_formatted_string("TextScrollFont_%i", fontSize); }