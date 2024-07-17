#include <cmath>
#include "config.hpp"
#include "ui/slidePanel.hpp"

ui::slidePanel::slidePanel(const std::string &panelName, int panelWidth, ui::slidePanelSide panelSide) : 
m_PanelWidth(panelWidth), 
m_PanelSide(panelSide)
{
    if(m_PanelSide == ui::PANEL_SIDE_LEFT)
    {
        m_X = -(m_PanelWidth);
    }
    else
    {
        m_X = 1280;
    }
    m_RenderTarget = graphics::textureCreate(panelName, m_PanelWidth, 720, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET);
}

ui::slidePanel::~slidePanel() { }

void ui::slidePanel::update(void)
{
    float uiAnimationScaling = config::getAnimationScaling();
    if(m_IsOpen && m_PanelSide == ui::PANEL_SIDE_LEFT && m_X < 0)
    {
        double increaseX = static_cast<double>(m_X) / uiAnimationScaling;
        m_X -= std::ceil(increaseX);
    }
    else if(m_IsOpen && m_PanelSide == ui::PANEL_SIDE_RIGHT && m_X > (1280 - m_PanelWidth))
    {
        double increaseX = ((1280.0f - static_cast<double>(m_PanelWidth)) - m_X) / uiAnimationScaling;
        m_X += std::ceil(increaseX);
    }
    else if(m_IsOpen == false && m_PanelSide == ui::PANEL_SIDE_LEFT && m_X > -(m_PanelWidth))
    {
        double subtractX = (static_cast<double>(m_PanelWidth) - static_cast<double>(m_X)) / uiAnimationScaling;
        m_X -= std::ceil(subtractX);
    }
    else if(m_IsOpen == false && m_PanelSide == ui::PANEL_SIDE_RIGHT && m_X < 1280)
    {
        double increaseX = (1280.0f - static_cast<double>(m_X)) / uiAnimationScaling;
        m_X += std::ceil(increaseX);
    }
}

void ui::slidePanel::render(void)
{
    graphics::textureRender(m_RenderTarget, NULL, m_X, 0);
}

SDL_Texture *ui::slidePanel::getPanelRenderTarget(void)
{
    return m_RenderTarget;
}

void ui::slidePanel::closePanel(void)
{
    m_IsOpen = false;
}

bool ui::slidePanel::isClosed(void)
{
    if(m_IsOpen == false && m_PanelSide == ui::PANEL_SIDE_LEFT && m_X <= 0)
    {
        return true;
    }
    else if(m_IsOpen == false && m_PanelSide == ui::PANEL_SIDE_RIGHT && m_X >= 1280)
    {
        return true;
    }
    return false;
}