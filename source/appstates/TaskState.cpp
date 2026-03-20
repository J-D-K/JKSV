#include "appstates/TaskState.hpp"

#include "appstates/FadeState.hpp"
#include "graphics/colors.hpp"
#include "graphics/fonts.hpp"
#include "graphics/screen.hpp"
#include "mathutil.hpp"
#include "sdl.hpp"
#include "strings/strings.hpp"
#include "ui/PopMessageManager.hpp"

//                      ---- Construction ----

TaskState::TaskState(sys::threadpool::JobFunction function, sys::Task::TaskData taskData)
{ m_task = std::make_unique<sys::Task>(function, taskData); }

//                      ---- Public functions ----

void TaskState::update(const sdl2::Input &input)
{
    BaseTask::pop_on_plus(input);
    BaseTask::update_loading_glyph();
    if (!m_task->is_running()) { TaskState::deactivate_state(); }
}

void TaskState::render(sdl2::Renderer &renderer)
{
    const std::string status = m_task->get_status();
    const int statusWidth    = sm_font->get_text_width(status);
    const int statusX        = math::Util<int>::center_within(graphics::SCREEN_WIDTH, statusWidth);

    renderer.render_rectangle(0, 0, graphics::SCREEN_WIDTH, graphics::SCREEN_HEIGHT, colors::DIM_BACKGROUND);
    sm_font->render_text(statusX, 351, colors::WHITE, status);

    BaseTask::render_loading_glyph();
}

void TaskState::deactivate_state()
{
    FadeState::create_and_push(colors::DIM_BACKGROUND, colors::ALPHA_FADE_END, colors::ALPHA_FADE_BEGIN, nullptr);
    BaseState::deactivate();
}

//                      ---- Private Functions ----

void TaskState::initialize_static_members()
{
    if (sm_font) { return; }

    sm_font = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::TWENTY_PIXEL,
                                                                        graphics::fonts::sizes::TWENTY_PIXEL);
}