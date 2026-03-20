#include "appstates/ProgressState.hpp"

#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
#include "graphics/fonts.hpp"
#include "graphics/screen.hpp"
#include "mathutil.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"

#include <cmath>

namespace
{
    // Initial coords.
    constexpr int INITIAL_WIDTH_HEIGHT = 32;

    // Target coords.
    constexpr int TARGET_WIDTH  = 720;
    constexpr int TARGET_HEIGHT = 256;

    constexpr int COORD_BAR_X = 296;
    constexpr int COORD_BAR_Y = 437;

    constexpr int COORD_TEXT_Y = 442;

    constexpr int COORD_DISPLAY_CENTER = 640;
    constexpr double SIZE_BAR_WIDTH    = 688.0f;
}

//                      ---- Construction ----

ProgressState::ProgressState(sys::threadpool::JobFunction function, sys::Task::TaskData taskData)
    : m_job(function)
    , m_taskData(taskData)
    , m_transition(0,
                   0,
                   INITIAL_WIDTH_HEIGHT,
                   INITIAL_WIDTH_HEIGHT,
                   0,
                   0,
                   TARGET_WIDTH,
                   TARGET_HEIGHT,
                   ui::Transition::DEFAULT_THRESHOLD)
{ initialize_static_members(); }

//                      ---- Public functions ----

void ProgressState::update(const sdl2::Input &input)
{
    // These are always updated and aren't conditional.
    BaseTask::update_loading_glyph();
    BaseTask::pop_on_plus(input);

    switch (m_state)
    {
        case State::Opening: ProgressState::update_dimensions(); break;
        case State::Closing: ProgressState::update_dimensions(); break;
        case State::Running: ProgressState::update_progress(); break;
    }
}

void ProgressState::render(sdl2::Renderer &renderer)
{
    static constexpr int RIGHT_EDGE_X = (COORD_BAR_X + SIZE_BAR_WIDTH) - 16;

    // Grab the focus since everything wants it.
    const bool hasFocus = BaseState::has_focus();

    // This will dim the background.
    renderer.render_rectangle(0, 0, graphics::SCREEN_WIDTH, graphics::SCREEN_HEIGHT, colors::DIM_BACKGROUND);

    // Render the glyph and dialog. Don't render anything else unless the task is running.
    BaseTask::render_loading_glyph();
    sm_dialog->render(renderer, hasFocus);
    if (m_state != State::Running) { return; }

    // Grab and render the status.
    const std::string status = m_task->get_status();
    sm_font->render_text(312, 255, colors::WHITE, status);

    // This is the divider line.
    renderer.render_line(280, 421, 999, 421, colors::DIV_COLOR);

    // Progress showing bar.
    renderer.render_rectangle(COORD_BAR_X, COORD_BAR_Y, static_cast<int>(SIZE_BAR_WIDTH), 32, colors::BLACK);
    renderer.render_rectangle(COORD_BAR_X, COORD_BAR_Y, m_progressBarWidth, 32, colors::BAR_GREEN);

    // These are the "caps" to round the edges of the bar.
    sm_barEdges->render_part(COORD_BAR_X, COORD_BAR_Y, 0, 0, 16, 32);
    sm_barEdges->render_part(RIGHT_EDGE_X, COORD_BAR_Y, 16, 0, 16, 32);

    // Progress string.
    sm_font->render_text(m_percentageX, COORD_TEXT_Y, colors::WHITE, m_percentageString);
}

//                      ---- Private functions ----

void ProgressState::initialize_static_members()
{
    static constexpr std::string_view BAR_EDGES = "romfs:/Textures/BarEdges/png";

    if (sm_dialog && sm_barEdges && sm_font) { return; }

    sm_dialog   = ui::DialogBox::create(0, 0, 0, 0);
    sm_barEdges = sdl2::TextureManager::create_load_resource(BAR_EDGES, BAR_EDGES);
    sm_font     = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::TWENTY_PIXEL,
                                                                            graphics::fonts::sizes::TWENTY_PIXEL);
    sm_dialog->set_from_transition(m_transition, true);
}

void ProgressState::update_dimensions() noexcept
{
    // Update the transition and dialog.
    m_transition.update();
    sm_dialog->set_from_transition(m_transition, true);

    // State shifting.
    const bool opened = m_state == State::Opening && m_transition.in_place();
    const bool closed = m_state == State::Closing && m_transition.in_place();
    if (opened)
    {
        // Gonna start the task here. Sometimes JKSV is too quick and it looks like nothing happened otherwise.
        m_task  = std::make_unique<sys::ProgressTask>(m_job, m_taskData);
        m_state = State::Running;
    }
    else if (closed) { ProgressState::deactivate_state(); }
}

void ProgressState::update_progress() noexcept
{
    // Cast pointer to the task. To do: Consider dynamic for this...
    auto *task = static_cast<sys::ProgressTask *>(m_task.get());

    // Current progress.
    const double current = task->get_progress();

    // Update the width and actual progress.
    m_progressBarWidth = std::round(SIZE_BAR_WIDTH * current);
    m_progress         = std::round(current * 100);

    // This is the actual string that's displayed.
    m_percentageString = stringutil::get_formatted_string("%u%%", m_progress);

    const int stringWidth = sm_font->get_text_width(m_percentageString);
    m_percentageX         = math::Util<int>::center_within(graphics::SCREEN_WIDTH, stringWidth);

    // Handle closing and updating.
    const bool taskRunning = m_task->is_running();
    if (!taskRunning && m_state != State::Closing) { ProgressState::close_dialog(); }
}

void ProgressState::close_dialog()
{
    m_state = State::Closing;
    m_transition.set_target_width(32);
    m_transition.set_target_height(32);
}

void ProgressState::deactivate_state()
{
    FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_END, colors::ALPHA_FADE_BEGIN, nullptr);
    BaseState::deactivate();
}
