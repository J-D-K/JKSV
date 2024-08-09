#include <cmath>
#include "config.hpp"
#include "ui/slidePanel.hpp"

ui::slidePanel::slidePanel(const std::string &panelName, int panelWidth, ui::slidePanelSide panelSide) : 
m_PanelWidth(panelWidth), 
m_PanelSide(panelSide),
m_RenderTarget(graphics::textureManager::createTexture(panelName, m_PanelWidth, 720, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET))
{
    if(m_PanelSide == ui::PANEL_SIDE_LEFT)
    {
        m_X = -(m_PanelWidth);
    }
    else
    {
        m_X = 1280;
    }
}

ui::slidePanel::~slidePanel() { }

void ui::slidePanel::update(void)
{
    // Get animation scaling for calculations
    float uiAnimationScaling = config::getAnimationScaling();

    // If the panel is open and not hidden for 
    if(m_IsOpen == true && m_HidePanel == false && m_PanelSide == ui::PANEL_SIDE_LEFT && m_X < 0)
    {
        double increaseX = static_cast<double>(m_X) / uiAnimationScaling;
        m_X -= std::ceil(increaseX);
    }
    else if(m_IsOpen == true && m_HidePanel == false && m_PanelSide == ui::PANEL_SIDE_RIGHT && m_X > (1288 - m_PanelWidth))
    {
        double increaseX = ((1280.0f - static_cast<double>(m_PanelWidth)) - m_X) / uiAnimationScaling;
        m_X += std::ceil(increaseX);
    }
    else if((m_IsOpen == false || m_HidePanel == true) && m_PanelSide == ui::PANEL_SIDE_LEFT && m_X > -(m_PanelWidth))
    {
        double subtractX = (static_cast<double>(m_PanelWidth) - static_cast<double>(m_X)) / uiAnimationScaling;
        m_X -= std::ceil(subtractX);
    }
    else if((m_IsOpen == false || m_HidePanel == true) && m_PanelSide == ui::PANEL_SIDE_RIGHT && m_X < 1280)
    {
        double increaseX = (1280.0f - static_cast<double>(m_X)) / uiAnimationScaling;
        m_X += std::ceil(increaseX);
    }
}

void ui::slidePanel::render(void)
{
    graphics::textureRender(m_RenderTarget.get(), NULL, m_X, 0);
}

graphics::sdlTexture ui::slidePanel::getPanelRenderTarget(void)
{
    return m_RenderTarget;
}

void ui::slidePanel::hidePanel(void)
{
    m_HidePanel = true;
}

void ui::slidePanel::unhidePanel(void)
{
    m_HidePanel = false;
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

bool ui::slidePanel::isHidden(void)
{
    return m_HidePanel;
}