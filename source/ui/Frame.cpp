#include "ui/Frame.hpp"

#include "graphics/colors.hpp"

//                      ---- Construction ----

ui::Frame::Frame(int x, int y, int width, int height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
{ Frame::initialize_static_members(); }

//                      ---- Public functions ----

void ui::Frame::render(sdl2::Renderer &renderer, bool hasFocus)
{
    // This is the size of one of the "tiles" of the frame.
    static constexpr int TILE = 16;

    const int midWidth      = m_width - (TILE * 2);
    const int midHeight     = m_height - (TILE * 2);
    const int rightEdge     = (m_x + m_width) - TILE;
    const int bottomEdge    = (m_y + m_height) - TILE;
    const int textureCorner = TILE * 2;

    // Top.
    sm_frameCorners->render_part(m_x, m_y, 0, 0, TILE, TILE);
    sm_frameCorners->render_part_stretched(TILE, 0, TILE, TILE, m_x + TILE, m_y, midWidth, TILE);
    sm_frameCorners->render_part(rightEdge, m_y, 32, 0, TILE, TILE);

    // Middle
    sm_frameCorners->render_part_stretched(0, TILE, TILE, TILE, m_x, m_y + TILE, TILE, midHeight);
    renderer.render_rectangle(m_x + TILE, m_y + TILE, midWidth, midHeight, colors::SLIDE_PANEL_CLEAR);
    sm_frameCorners->render_part_stretched(textureCorner, TILE, TILE, TILE, rightEdge, m_y + TILE, TILE, midHeight);

    // Bottom
    sm_frameCorners->render_part(m_x, bottomEdge, 0, textureCorner, TILE, TILE);
    sm_frameCorners->render_part_stretched(TILE, textureCorner, TILE, TILE, m_x + TILE, bottomEdge, midWidth, TILE);
    sm_frameCorners->render_part(rightEdge, bottomEdge, textureCorner, textureCorner, TILE, TILE);
}

void ui::Frame::set_x(int x) noexcept { m_x = x; }

void ui::Frame::set_y(int y) noexcept { m_y = y; }

void ui::Frame::set_width(int width) noexcept { m_width = width; }

void ui::Frame::set_height(int height) noexcept { m_height = height; }

void ui::Frame::set_from_transition(const ui::Transition &transition, bool centered) noexcept
{
    const int x      = centered ? transition.get_centered_x() : transition.get_x();
    const int y      = centered ? transition.get_centered_y() : transition.get_y();
    const int width  = transition.get_width();
    const int height = transition.get_height();

    m_x      = x;
    m_y      = y;
    m_width  = width;
    m_height = height;
}

//                      ---- Private functions ----

void ui::Frame::initialize_static_members()
{
    static constexpr std::string_view FRAME = "romfs:/Textures/Frame.png";

    if (sm_frameCorners) { return; }
    sm_frameCorners = sdl2::TextureManager::create_load_resource(FRAME, FRAME);
}
