#include "AppStates/ProgressState.hpp"
#include "Colors.hpp"
#include "SDL.hpp"
#include "StringUtil.hpp"
#include <cmath>

void ProgressState::Update(void)
{
    m_ProgressBarWidth = std::ceil(720.0f * m_Task.GetCurrentProgress());
    m_Progress = std::ceil(m_Task.GetCurrentProgress() * 100);
    m_PercentageString = StringUtil::GetFormattedString("%u", m_Progress);
    m_PerentageX = 640 - (SDL::Text::GetWidth(18, m_PercentageString.c_str()));
}

void ProgressState::Render(void)
{
    SDL::RenderRectFill(NULL, 0, 0, 1280, 720, Colors::BackgroundDim);
}
