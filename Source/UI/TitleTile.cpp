#include "UI/TitleTile.hpp"
#include "Colors.hpp"

UI::TitleTile::TitleTile(bool IsFavorite, SDL::SharedTexture Icon) : m_IsFavorite(IsFavorite), m_Icon(Icon) {};

void UI::TitleTile::Update(bool IsSelected)
{
    if (IsSelected && m_RenderWidth < 164)
    {
        // I think it's safe to assume both are too small.
        m_RenderWidth += 18;
        m_RenderHeight += 18;
    }
    else if (!IsSelected && m_RenderWidth > 128)
    {
        m_RenderWidth -= 9;
        m_RenderHeight -= 9;
    }
}

void UI::TitleTile::Render(SDL_Texture *Target, int X, int Y)
{
    int RenderX = X - ((m_RenderWidth - 128) / 2);
    int RenderY = Y - ((m_RenderHeight - 128) / 2);

    m_Icon->RenderStretched(Target, RenderX, RenderY, m_RenderWidth, m_RenderHeight);
    if (m_IsFavorite)
    {
        SDL::Text::Render(Target, RenderX + 4, RenderY + 2, 28, SDL::Text::NO_TEXT_WRAP, Colors::Pink, "\uE017");
    }
}

void UI::TitleTile::Reset(void)
{
    m_RenderWidth = 128;
    m_RenderHeight = 128;
}

int UI::TitleTile::GetWidth(void) const
{
    return m_RenderWidth;
}

int UI::TitleTile::GetHeight(void) const
{
    return m_RenderHeight;
}
