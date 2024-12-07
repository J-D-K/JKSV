#include "UI/ColorMod.hpp"

void UI::ColorMod::Update(void)
{
    if (m_Direction && (m_ColorMod += 6) >= 0x72)
    {
        m_Direction = false;
    }
    else if (!m_Direction && (m_ColorMod -= 3) <= 0x00)
    {
        m_Direction = true;
    }
}

UI::ColorMod::operator uint8_t(void) const
{
    return m_ColorMod;
}
