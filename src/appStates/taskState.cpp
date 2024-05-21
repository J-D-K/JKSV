#include "appStates/taskState.hpp"
#include "graphics/graphics.hpp"

taskState::taskState(sys::taskFunction function, std::shared_ptr<sys::taskArgs> args)
{
    m_Task = std::make_unique<sys::task>(function, args);
}

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
    
    // Dim background and render text to screen
    graphics::renderRect(NULL, 0, 0, 1280, 720, COLOR_DIM_BACKGROUND);
    graphics::systemFont::renderText(taskStatus, NULL, taskStatusX, 348, 24, COLOR_WHITE);
}