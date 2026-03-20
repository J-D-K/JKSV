#include "ui/ColorMod.hpp"

//                      ---- Public functions ----

void ui::ColorMod::update() noexcept
{
    const bool changeDown = m_direction && ((m_colorMod += 6) >= 0x72);
    const bool changeUp   = !m_direction && ((m_colorMod -= 3) <= 0x00);
    if (changeDown) { m_direction = false; }
    else if (changeUp) { m_direction = true; }
}

ui::ColorMod::operator SDL_Color() const noexcept
{ return SDL_Color{0x00, static_cast<uint8_t>(0x88 + m_colorMod), static_cast<uint8_t>(0xC5 + m_colorMod * 0.5), 0xFF}; }
