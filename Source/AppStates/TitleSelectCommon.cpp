#include "AppStates/TitleSelectCommon.hpp"
#include "Colors.hpp"
#include "SDL.hpp"
#include "Strings.hpp"

TitleSelectCommon::TitleSelectCommon(void)
{
    if (m_TitleControlsX == 0)
    {
        m_TitleControlsX = 1220 - SDL::Text::GetWidth(22, Strings::GetByName(Strings::Names::ControlGuides, 1));
    }
}

void TitleSelectCommon::RenderControlGuide(void)
{
    if (AppState::HasFocus())
    {
        SDL::Text::Render(NULL,
                          m_TitleControlsX,
                          673,
                          22,
                          SDL::Text::NO_TEXT_WRAP,
                          Colors::White,
                          Strings::GetByName(Strings::Names::ControlGuides, 1));
    }
}
