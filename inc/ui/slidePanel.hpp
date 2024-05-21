#pragma once
#include <memory>
#include "graphics/graphics.hpp"

namespace ui
{
    typedef enum
    {
        PANEL_SIDE_LEFT,
        PANEL_SIDE_RIGHT
    } slidePanelSide;

    class slidePanel
    {
        public:
            slidePanel(const std::string &panelName, const int &panelWidth, const ui::slidePanelSide &panelSide);
            ~slidePanel();

            void update(void);
            // Panels are always rendered to framebuffer
            void render(void);
            // Returns render target for rendering in state
            SDL_Texture *getPanelRenderTarget(void);
            // Closes panel
            void closePanel(void);
            // Returns if panel is fully closed
            bool isClosed(void);

        private:
            // Whether panel is opened. Always true by default
            bool m_IsOpen = true;
            // Width of panel. Height is always 720
            int m_PanelWidth;
            // Current X
            int m_X;
            // Which side the panel slides from
            ui::slidePanelSide m_PanelSide;
            // Render target of panel
            SDL_Texture *m_RenderTarget;
    };
}