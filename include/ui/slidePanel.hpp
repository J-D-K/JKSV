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
            slidePanel(const std::string &panelName, int panelWidth, ui::slidePanelSide panelSide);
            ~slidePanel();

            void update(void);
            // Panels are always rendered to framebuffer
            void render(void);

            // Returns render target for rendering in state
            graphics::sdlTexture getPanelRenderTarget(void);
            // Hides panel, but doesn't close it.
            void hidePanel(void);
            // Unhides panel
            void unhidePanel(void);
            // Closes panel
            void closePanel(void);
            // Returns if panel is fully closed
            bool isClosed(void);
            // Returns if panel is hidden.
            bool isHidden(void);

        private:
            // Whether panel is opened. Always true by default
            bool m_IsOpen = true;
            // Whether panel should be hidden
            bool m_HidePanel = false;
            // Width of panel. Height is always 720
            int m_PanelWidth;
            // Current X
            int m_X;
            // Which side the panel slides from
            ui::slidePanelSide m_PanelSide;
            // Render target of panel
            graphics::sdlTexture m_RenderTarget;
    };
}
