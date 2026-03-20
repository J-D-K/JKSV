#include "ui/ControlGuide.hpp"

#include "graphics/colors.hpp"
#include "graphics/fonts.hpp"
#include "graphics/screen.hpp"
#include "logging/logger.hpp"

namespace
{
    /// @brief This is the X coordinate used to calculate where the guide should be.
    constexpr int GUIDE_X_OFFSET = 1220;

    /// @brief This is the font size used for the guides.
    constexpr int GUIDE_TEXT_SIZE = 24;

    /// @brief This is the constant Y of the transition.
    constexpr int TRANS_Y = 662;

    /// @brief The padding for the control guide container.
    constexpr int CONTAINER_PADDING = 24;
}

//                      ---- Construction ----

ui::ControlGuide::ControlGuide(const char *guide)
    : m_guide(guide)
    , m_targetX(GUIDE_X_OFFSET - (m_textWidth + CONTAINER_PADDING))
    , m_guideWidth(graphics::SCREEN_WIDTH - m_targetX)
    , m_transition(graphics::SCREEN_WIDTH, TRANS_Y, 0, 0, m_targetX, TRANS_Y, 0, 0, ui::Transition::DEFAULT_THRESHOLD)
    , m_state(State::Hidden)
{
    // Init static.
    ui::ControlGuide::initialize_static_members();

    // Grab the width of the text.
    m_textWidth = sm_font->get_text_width(m_guide);
}

//                      ---- Public functions ----

void ui::ControlGuide::update(const sdl2::Input &input, bool hasFocus)
{
    switch (m_state)
    {
        case State::Opening:
        case State::Hiding:  ControlGuide::update_position_state(); break;
        default:             ControlGuide::update_state(hasFocus); break;
    }
}

void ui::ControlGuide::sub_update()
{
    // I don't like repeating, but it's the easiest way to adapt this for my SDL2 changes.
    switch (m_state)
    {
        case State::Opening:
        case State::Hiding:  ControlGuide::update_position_state(); break;
        default:             ControlGuide::update_state(false); break;
    }
}

void ui::ControlGuide::render(sdl2::Renderer &renderer, bool hasFocus)
{
    // These are for the rectangle that makes up the rest of the container.
    static constexpr int RECT_OFFSET_X = 16;
    static constexpr int RECT_HEIGHT   = 48;

    // This is where the text is rendered and its size.
    static constexpr int TEXT_OFFSET_X = 24;
    static constexpr int TEXT_OFFSET_Y = 10;

    // Grab the X and Y.
    const int guideX = m_transition.get_x();
    const int guideY = m_transition.get_y();

    // Render the cap and the rectangle.
    sm_controlCap->render(guideX, guideY);
    renderer.render_rectangle(guideX + RECT_OFFSET_X, guideY, m_guideWidth - RECT_OFFSET_X, RECT_HEIGHT, colors::GUIDE_COLOR);

    // Guide text.
    sm_font->render_text(guideX + TEXT_OFFSET_X, guideY + TEXT_OFFSET_Y, colors::WHITE, m_guide);
}

//                      ---- Private functions ----

void ui::ControlGuide::initialize_static_members()
{
    // Name for the texture manager.
    static constexpr std::string_view CAP_PATH = "romfs:/Textures/GuideCap.png";

    // If it's already loaded, return.
    if (sm_controlCap && sm_font) { return; }

    sm_controlCap = sdl2::TextureManager::create_load_resource(CAP_PATH, CAP_PATH);
    sm_font       = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::TWENTY_FOUR_PIXEL,
                                                                              graphics::fonts::sizes::TWENTY_FOUR_PIXEL);
}

void ui::ControlGuide::reset() noexcept
{
    // Set both to the width of the screen.
    m_transition.set_target_x(graphics::SCREEN_WIDTH);
    m_transition.set_x(graphics::SCREEN_WIDTH);

    // Set the state to Hidden.
    m_state = State::Hidden;
}

void ui::ControlGuide::update_position_state() noexcept
{
    // Update the transition.
    m_transition.update();

    // State conditions.
    const bool opened = m_state == State::Opening && m_transition.in_place_xy();
    const bool hidden = m_state == State::Hiding && m_transition.in_place_xy();
    if (opened) { m_state = State::Opened; }
    else if (hidden) { m_state = State::Hidden; }
}

void ui::ControlGuide::update_state(bool hasFocus) noexcept
{
    // Conditions.
    const bool hide   = (m_state == State::Opening || m_state == State::Opened) && !hasFocus;
    const bool unhide = (m_state == State::Hiding || m_state == State::Hidden) && hasFocus;
    if (hide)
    {
        // Set target to screen width.
        m_transition.set_target_x(graphics::SCREEN_WIDTH);
        m_state = State::Hiding;
    }
    else if (unhide)
    {
        // Set target to open.
        m_transition.set_target_x(m_targetX);
        m_state = State::Opening;
    }
}