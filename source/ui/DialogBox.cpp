#include "ui/DialogBox.hpp"

#include "graphics/colors.hpp"

namespace
{
    constexpr int CORNER_WIDTH  = 16;
    constexpr int CORNER_HEIGHT = 16;
}

//                      ---- Construction ----

ui::DialogBox::DialogBox(int x, int y, int width, int height, ui::DialogBox::Type type)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_type(type)
{ ui::DialogBox::initialize_static_members(); }

//                      ---- Public functions ----

void ui::DialogBox::render(sdl2::Renderer &renderer, bool hasFocus)
{
    const bool darkDialog        = m_type == DialogBox::Type::Dark;
    sdl2::SharedTexture &corners = darkDialog ? sm_darkCorners : sm_lightCorners;
    const SDL_Color rectColor    = darkDialog ? colors::DIALOG_DARK : colors::DIALOG_LIGHT;

    // Top
    corners->render_part(m_x, m_y, 0, 0, CORNER_WIDTH, CORNER_HEIGHT);
    renderer.render_rectangle(m_x + CORNER_WIDTH, m_y, m_width - (CORNER_WIDTH * 2), CORNER_HEIGHT, rectColor);
    corners->render_part((m_x + m_width) - CORNER_WIDTH, m_y, CORNER_WIDTH, 0, CORNER_WIDTH, CORNER_HEIGHT);

    // Middle
    renderer.render_rectangle(m_x, m_y + CORNER_HEIGHT, m_width, m_height - (CORNER_HEIGHT * 2), rectColor);

    // Bottom
    corners->render_part(m_x, (m_y + m_height) - CORNER_HEIGHT, 0, CORNER_HEIGHT, CORNER_WIDTH, CORNER_HEIGHT);
    renderer.render_rectangle(m_x + CORNER_WIDTH,
                              (m_y + m_height) - CORNER_HEIGHT,
                              m_width - (CORNER_WIDTH * 2),
                              CORNER_HEIGHT,
                              rectColor);
    corners->render_part((m_x + m_width) - CORNER_WIDTH,
                         (m_y + m_height) - CORNER_HEIGHT,
                         CORNER_WIDTH,
                         CORNER_HEIGHT,
                         CORNER_WIDTH,
                         CORNER_HEIGHT);
}

void ui::DialogBox::set_x(int x) noexcept { m_x = x; }

void ui::DialogBox::set_y(int y) noexcept { m_y = y; }

void ui::DialogBox::set_width(int width) noexcept { m_width = width; }

void ui::DialogBox::set_height(int height) noexcept { m_height = height; }

void ui::DialogBox::set_from_transition(ui::Transition &transition, bool centered)
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

void ui::DialogBox::initialize_static_members()
{
    static constexpr std::string_view DARK_PATH  = "romfs:/Textures/DialogCornersDark.png";
    static constexpr std::string_view LIGHT_PATH = "romfs:/Textures/DialogCornersLight.png";

    if (sm_darkCorners && sm_lightCorners) { return; }

    sm_darkCorners  = sdl2::TextureManager::create_load_resource(DARK_PATH, DARK_PATH);
    sm_lightCorners = sdl2::TextureManager::create_load_resource(LIGHT_PATH, LIGHT_PATH);
}
