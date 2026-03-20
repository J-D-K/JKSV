#include "appstates/BaseTask.hpp"

#include "graphics/colors.hpp"
#include "graphics/fonts.hpp"
#include "strings/strings.hpp"
#include "ui/PopMessageManager.hpp"

namespace
{
    /// @brief This is the time in milliseconds between changing glyphs.
    constexpr uint64_t TICKS_GLYPH_TRIGGER = 50;
} // namespace

//                      ---- Construction ----
BaseTask::BaseTask()
    : BaseState(false)
    , m_frameTimer(TICKS_GLYPH_TRIGGER)
    , m_popUnableExit(strings::get_by_name(strings::names::GENERAL_POPS, 0))
{ BaseTask::initialize_static_members(); }

//                      ---- Public functions ----

void BaseTask::update_loading_glyph()
{
    m_colorMod.update();

    if (!m_frameTimer.is_triggered()) { return; }
    else if (++m_currentFrame % 8 == 0) { m_currentFrame = 0; }
}

void BaseTask::pop_on_plus(const sdl2::Input &input)
{
    const bool plusPressed = input.button_pressed(HidNpadButton_Plus);

    if (plusPressed) { ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS, m_popUnableExit); }
}

void BaseTask::render_loading_glyph()
{
    // Render coords.
    static constexpr int RENDER_X = 56;
    static constexpr int RENDER_Y = 673;

    sm_font->render_text(RENDER_X, RENDER_Y, colors::WHITE, sm_glyphArray[m_currentFrame]);
}

//                      ---- Private Functions ----

void BaseTask::initialize_static_members()
{
    if (sm_font) { return; }

    sm_font = sdl2::FontManager::create_load_resource<sdl2::SystemFont>(graphics::fonts::names::THIRTY_TWO_PIXEL,
                                                                        graphics::fonts::sizes::THIRTY_TWO_PIXEL);
}