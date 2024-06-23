#include "appStates/taskState.hpp"
#include "graphics/graphics.hpp"
#include "jksv.hpp"
#include "log.hpp"

// Time in ticks before changing glyph frames
static const int GLYPH_CHANGE_TICKS = 50;

// These are the actual glyphs
static const std::string s_LoadGlyphArray[8] = 
{
    "\ue020", "\ue021", "\ue022", "\ue023",
    "\ue024", "\ue025", "\ue026", "\ue027"
};

taskState::taskState(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args) :
m_Task(std::make_unique<sys::task>(threadFunction, args)),
m_LoadingGlyphTimer(std::make_unique<sys::timer>(GLYPH_CHANGE_TICKS)) { }

taskState::~taskState() { }

void taskState::update(void)
{
    if(m_Task->isRunning() == false)
    {
        appState::deactivateState();
    }
}

void taskState::render(void)
{
    // Get thread status and calculate centered text
    std::string taskStatus = m_Task->getThreadStatus();
    int taskStatusX = 640 - (graphics::systemFont::getTextWidth(taskStatus, 24) / 2);

    // Render glyph in bottom left corner
    renderLoadingGlyph();
    
    // Dim background and render text to screen
    graphics::renderRect(NULL, 0, 0, 1280, 720, COLOR_DIM_BACKGROUND);
    graphics::systemFont::renderText(taskStatus, NULL, taskStatusX, 348, 24, COLOR_WHITE);
}

void taskState::renderLoadingGlyph(void)
{
    // If timer is triggered, update glyph frame
    if(m_LoadingGlyphTimer->triggered() && ++m_GlyphFrame == 8)
    {
        m_GlyphFrame = 0;
    }

    // If true, we're adding, false subtracting
    if(m_ColorModifier && (m_LoadingGlyphColorMod += 6) >= 0x72)
    {
        m_ColorModifier = false;
    }
    else if(m_ColorModifier && (m_LoadingGlyphColorMod -= 3) <= 0x00)
    {
        m_ColorModifier = true;
    }

    graphics::systemFont::renderText(s_LoadGlyphArray[m_GlyphFrame], NULL, 56, 673, 32, createColor(0x00, (0x88 + m_LoadingGlyphColorMod), (0xC5 + (m_LoadingGlyphColorMod / 2)), 0xFF));
}

void createAndPushNewTask(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args)
{
    std::unique_ptr<appState> newTaskState = std::make_unique<taskState>(threadFunction, args);
    jksv::pushNewState(newTaskState);
}