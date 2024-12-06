#include "AppStates/TaskState.hpp"
#include "Colors.hpp"
#include "SDL.hpp"

void TaskState::Update(void)
{
    if (!m_Task.IsRunning())
    {
        AppState::Deactivate();
    }
}

void TaskState::Render(void)
{
    // Grab task string.
    std::string Status = m_Task.GetStatus();
    // Center so it looks perty
    int StatusX = 640 - (SDL::Text::GetWidth(18, Status.c_str()) / 2);
    // Dim the background states.
    SDL::RenderRectFill(NULL, 0, 0, 1280, 720, Colors::BackgroundDim);
    // Render the status.
    SDL::Text::Render(NULL, StatusX, 351, 18, SDL::Text::NO_TEXT_WRAP, Colors::White, Status.c_str());
}
