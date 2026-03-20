#include "ui/TitleTile.hpp"

#include "graphics/colors.hpp"
#include "logging/logger.hpp"

namespace
{
    constexpr int UNSELECTED_WIDTH_HEIGHT = 128;
    constexpr int SELECTED_WIDTH_HEIGHT   = 176;
}

//                      ---- Construction ----

ui::TitleTile::TitleTile(bool isFavorite, int index, sdl2::SharedTexture &icon)
    : m_transition(0,
                   0,
                   UNSELECTED_WIDTH_HEIGHT,
                   UNSELECTED_WIDTH_HEIGHT,
                   0,
                   0,
                   UNSELECTED_WIDTH_HEIGHT,
                   UNSELECTED_WIDTH_HEIGHT,
                   m_transition.DEFAULT_THRESHOLD)
    , m_isFavorite(isFavorite)
    , m_icon(icon) {};

//                      ---- Public functions ----

void ui::TitleTile::update(bool isSelected)
{
    if (isSelected)
    {
        m_transition.set_target_width(SELECTED_WIDTH_HEIGHT);
        m_transition.set_target_height(SELECTED_WIDTH_HEIGHT);
    }
    else
    {
        m_transition.set_target_width(UNSELECTED_WIDTH_HEIGHT);
        m_transition.set_target_height(UNSELECTED_WIDTH_HEIGHT);
    }

    m_transition.update_width_height();
}

void ui::TitleTile::render(int x, int y)
{
    static constexpr std::string_view HEART_CHAR = "\uE017";

    const int width   = m_transition.get_width();
    const int height  = m_transition.get_height();
    const int renderX = x - ((width - 128) / 2);
    const int renderY = y - ((width - 128) / 2);

    m_icon->render_stretched(renderX, renderY, width, height);

    if (m_isFavorite) { sm_heartFont->render_text(renderX + 2, renderY + 2, colors::PINK, HEART_CHAR); }
}

void ui::TitleTile::reset() noexcept
{
    m_transition.set_width(UNSELECTED_WIDTH_HEIGHT);
    m_transition.set_height(UNSELECTED_WIDTH_HEIGHT);
    m_transition.set_target_width(UNSELECTED_WIDTH_HEIGHT);
    m_transition.set_target_height(UNSELECTED_WIDTH_HEIGHT);
}

int ui::TitleTile::get_width() const noexcept { return m_transition.get_width(); }

int ui::TitleTile::get_height() const noexcept { return m_transition.get_height(); }

//                          ---- Private Functions ----

void ui::TitleTile::initialize_static_members()
{
    // Font name for the manager.
    static constexpr std::string_view FONT_NAME = "HeartFont";
    static constexpr int FONT_SIZE              = 28;

    if (sm_heartFont) { return; }

    sm_heartFont = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(FONT_NAME, FONT_SIZE);
}