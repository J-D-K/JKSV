#include "ui/BoundingBox.hpp"

namespace
{
    constexpr int CORNER_WIDTH  = 8;
    constexpr int CORNER_HEIGHT = 8;

    constexpr int RECT_WIDTH  = 4;
    constexpr int RECT_HEIGHT = 4;
}

//                      ---- Construction ----

ui::BoundingBox::BoundingBox(int x, int y, int width, int height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
{ BoundingBox::initialize_static_members(); }

//                      ---- Public functions ----

void ui::BoundingBox::update(const sdl2::Input &input, bool hasFocus) { m_colorMod.update(); }

void ui::BoundingBox::render(sdl2::Renderer &renderer, bool hasFocus)
{
    // Sizes of the pieces.
    static constexpr int CORNER_WIDTH  = 8;
    static constexpr int CORNER_HEIGHT = 8;
    static constexpr int RECT_WIDTH    = 4;
    static constexpr int RECT_HEIGHT   = 4;

    // Set texture color modifier.
    sm_corners->set_color_mod(m_colorMod);

    // Calculating this all here maker the rest easier.
    const int rightX      = (m_x + m_width) - CORNER_WIDTH;
    const int rightRectX  = (m_x + m_width) - RECT_WIDTH;
    const int midX        = m_x + CORNER_WIDTH;
    const int midY        = m_y + CORNER_HEIGHT;
    const int midWidth    = m_width - (CORNER_WIDTH * 2);
    const int midHeight   = m_height - (CORNER_HEIGHT * 2);
    const int bottomY     = (m_y + m_height) - CORNER_HEIGHT;
    const int bottomRectY = (m_y + m_height) - RECT_HEIGHT;

    // Top
    sm_corners->render_part(m_x, m_y, 0, 0, CORNER_WIDTH, CORNER_HEIGHT);
    renderer.render_rectangle(midX, m_y, midWidth, RECT_HEIGHT, m_colorMod);
    sm_corners->render_part(rightX, m_y, CORNER_HEIGHT, 0, CORNER_WIDTH, CORNER_HEIGHT);

    // Middle
    renderer.render_rectangle(m_x, midY, RECT_WIDTH, midHeight, m_colorMod);
    renderer.render_rectangle(rightRectX, midY, RECT_WIDTH, midHeight, m_colorMod);

    // Bottom
    sm_corners->render_part(m_x, bottomY, 0, CORNER_HEIGHT, CORNER_WIDTH, CORNER_HEIGHT);
    renderer.render_rectangle(midX, bottomRectY, midWidth, RECT_HEIGHT, m_colorMod);
    sm_corners->render_part(rightX, bottomY, CORNER_WIDTH, CORNER_HEIGHT, CORNER_WIDTH, CORNER_HEIGHT);
}

void ui::BoundingBox::set_x(int x) noexcept { m_x = x; }

void ui::BoundingBox::set_y(int y) noexcept { m_y = y; }

void ui::BoundingBox::set_width(int width) noexcept { m_width = width; }

void ui::BoundingBox::set_height(int height) noexcept { m_height = height; }

//                      ---- Private functions ----

void ui::BoundingBox::initialize_static_members()
{
    // Path to load the corner texture from?
    static constexpr std::string_view CORNER_PATH = "romfs:/Textures/MenuBounding.png";

    if (sm_corners) { return; }
    sm_corners = sdl2::TextureManager::create_load_resource(CORNER_PATH, CORNER_PATH);
}
