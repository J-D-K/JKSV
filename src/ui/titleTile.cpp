#include "ui/titleTile.hpp"
#include "graphics/systemFont.hpp"

namespace
{
    double ICON_SIZE_MULTIPLIER = 1.28f;
    int ICON_EXPAND_SIZE = 18;
    int ICON_CONTRACT_SIZE = 9;
}

ui::titleTile::titleTile(int width, int height, bool favorite, SDL_Texture *icon) : 
m_Width(width), 
m_Height(height), 
m_IsFavorite(favorite), 
m_Icon(icon)
{
    m_RenderWidth = m_Width;
    m_RenderHeight = m_Height;
    m_SelectedWidth = m_Width * ICON_SIZE_MULTIPLIER;
    m_SelectedHeight = m_Height * ICON_SIZE_MULTIPLIER;
}

void ui::titleTile::update(bool isSelected)
{
    // Not sure if I should leave this like this. Feels kind of wrong
    if (isSelected && m_RenderWidth < m_SelectedWidth)
    {
        m_RenderWidth += ICON_EXPAND_SIZE;
    }

    if (isSelected && m_RenderHeight < m_SelectedHeight)
    {
        m_RenderHeight += ICON_EXPAND_SIZE;
    }

    if (!isSelected && m_RenderWidth > m_Width)
    {
        m_RenderWidth -= ICON_CONTRACT_SIZE;
    }

    if (!isSelected && m_RenderHeight > m_Height)
    {
        m_RenderHeight -= ICON_CONTRACT_SIZE;
    }
}

void ui::titleTile::render(SDL_Texture *target, int x, int y)
{
    m_RenderX = x - ((m_RenderWidth - m_Width) / 2);
    m_RenderY = y - ((m_RenderHeight - m_Height) / 2);
    graphics::textureRenderStretched(m_Icon, target, m_RenderX, m_RenderY, m_RenderWidth, m_RenderHeight);
    if (m_IsFavorite)
    {
        graphics::systemFont::renderText("♥", target, m_RenderX + 8, m_RenderY + 8, 20, COLOR_HEART);
    }
}