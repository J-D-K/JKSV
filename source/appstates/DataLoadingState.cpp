#include "appstates/DataLoadingState.hpp"

#include "graphics/colors.hpp"
#include "graphics/fonts.hpp"
#include "graphics/screen.hpp"
#include "logging/logger.hpp"
#include "mathutil.hpp"

//                      ---- Construction ----

DataLoadingState::DataLoadingState(data::DataContext &context,
                                   sdl2::Renderer &renderer,
                                   DestructFunction destructFunction,
                                   sys::threadpool::JobFunction function,
                                   sys::Task::TaskData taskData)
    : BaseTask()
    , m_context(context)
    , m_renderer(renderer)
    , m_destructFunction(destructFunction)
{
    DataLoadingState::initialize_static_members();
    m_task = std::make_unique<sys::Task>(function, taskData);
}

//                      ---- Public functions ----

void DataLoadingState::update(const sdl2::Input &input)
{
    BaseTask::update_loading_glyph();
    if (!m_task->is_running()) { DataLoadingState::deactivate_state(); }
    m_context.process_icon_queue(m_renderer);
}

void DataLoadingState::sub_update() { BaseTask::update_loading_glyph(); }

void DataLoadingState::render(sdl2::Renderer &renderer)
{
    // These are the dimensions of the icon loaded.
    static constexpr int ICON_WIDTH  = 256;
    static constexpr int ICON_HEIGHT = 259;
    static constexpr int ICON_X      = math::Util<int>::center_within(graphics::SCREEN_WIDTH, ICON_WIDTH);
    static constexpr int ICON_Y      = math::Util<int>::center_within(graphics::SCREEN_HEIGHT, ICON_HEIGHT);

    // Grab the status from the task and center it.
    const std::string status = m_task->get_status();
    const int statusWidth    = sm_font->get_text_width(status);
    m_statusX                = math::Util<int>::center_within(graphics::SCREEN_WIDTH, statusWidth);

    // This is to cover up the base rendering.
    renderer.render_rectangle(0, 0, graphics::SCREEN_WIDTH, graphics::SCREEN_HEIGHT, colors::CLEAR_COLOR);

    // Render the icon, status and loading glyph.
    sm_jksvIcon->render(ICON_X, ICON_Y);
    sm_font->render_text(m_statusX, 673, colors::WHITE, status);
    BaseTask::render_loading_glyph();
}

//                      ---- Private functions ----

void DataLoadingState::initialize_static_members()
{
    // Path for the loading icon.
    static constexpr std::string_view ICON_PATH = "romfs:/Textures/LoadingIcon.png";

    if (sm_jksvIcon && sm_font) { return; }

    // Load icon and font.
    sm_jksvIcon = sdl2::TextureManager::create_load_resource(ICON_PATH, ICON_PATH);
    sm_font     = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::TWENTY_PIXEL,
                                                                            graphics::fonts::sizes::TWENTY_PIXEL);
}

void DataLoadingState::deactivate_state()
{
    // This is to catch any stragglers.
    m_context.process_icon_queue(m_renderer);
    if (m_destructFunction) { m_destructFunction(); }
    BaseState::deactivate();
}
