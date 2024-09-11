#include "appStates/taskState.hpp"

#include "graphics/graphics.hpp"
#include "ui/ui.hpp"
#include "jksv.hpp"
#include "log.hpp"

namespace
{
    // Y position of thread status text
    static const int STATUS_TEXT_Y = 348;
    static const int STATUS_TEXT_FONT_SIZE = 24;
}

taskState::taskState(sys::taskFunction threadFunction, sys::sharedTaskData sharedData) : m_Task(std::make_unique<sys::task>(threadFunction, sharedData)) {}

taskState::~taskState() {}

void taskState::update(void)
{
    if (m_Task->isRunning() == false)
    {
        appState::deactivateState();
    }
}

void taskState::render(void)
{
    // Get thread status and calculate centered text
    std::string taskStatus = m_Task->getThreadStatus();
    int taskStatusX = 640 - (graphics::systemFont::getTextWidth(taskStatus, STATUS_TEXT_FONT_SIZE) / 2);

    // Render glyph in bottom left corner
    ui::renderLoadingGlyph();

    // Dim background and render text to screen
    graphics::renderRect(NULL, 0, 0, 1280, 720, COLOR_DIM_BACKGROUND);
    graphics::systemFont::renderText(taskStatus, NULL, taskStatusX, STATUS_TEXT_Y, STATUS_TEXT_FONT_SIZE, COLOR_WHITE);
}
