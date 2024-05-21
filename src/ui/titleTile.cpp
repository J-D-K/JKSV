#include "ui/titleTile.hpp"
#include "graphics/systemFont.hpp"

ui::titleTile::titleTile(const int &width, const int &height, const bool &favorite, SDL_Texture *icon) : m_Width(width), m_Height(height), m_IsFavorite(favorite), m_Icon(icon)
{
    m_RenderWidth = m_Width;
    m_RenderHeight = m_Height;
    m_SelectedWidth = m_Width * 1.28f;
    m_SelectedHeight = m_Height * 1.28;
}

void ui::titleTile::update(const bool &isSelected)
{
    // Not sure if I should leave this like this. Feels kind of wrong
    if (isSelected && m_RenderWidth < m_SelectedWidth)
    {
        m_RenderWidth += 18;
    }

    if (isSelected && m_RenderHeight < m_SelectedHeight)
    {
        m_RenderHeight += 18;
    }

    if (!isSelected && m_RenderWidth > m_Width)
    {
        m_RenderWidth -= 9;
    }

    if (!isSelected && m_RenderHeight > m_Height)
    {
        m_RenderHeight -= 9;
    }
}

void ui::titleTile::render(SDL_Texture *target, const int &x, const int &y)
{
    m_RenderX = x - ((m_RenderWidth - m_Width) / 2);
    m_RenderY = y - ((m_RenderHeight - m_Height) / 2);
    graphics::textureRenderStretched(m_Icon, target, m_RenderX, m_RenderY, m_RenderWidth, m_RenderHeight);
    if (m_IsFavorite)
    {
        graphics::systemFont::renderText("♥", target, m_RenderX + 8, m_RenderY + 8, 20, COLOR_HEART);
    }
}