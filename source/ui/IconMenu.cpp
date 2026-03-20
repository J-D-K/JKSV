#include "ui/IconMenu.hpp"

#include "graphics/ScopedRender.hpp"
#include "graphics/colors.hpp"

namespace
{
    constexpr int ICON_RENDER_WIDTH  = 128;
    constexpr int ICON_RENDER_HEIGHT = 128;

    constexpr int BOUND_WIDTH  = 152;
    constexpr int BOUND_HEIGHT = 146;
}

//                      ---- Construction ----

ui::IconMenu::IconMenu(int x, int y, int renderTargetHeight)
    : Menu(x, y, 152, 84, renderTargetHeight)
{
    // This needs to be overriden from the base state.
    m_boundingBox->set_width(152);
    m_boundingBox->set_height(146);
    m_optionHeight = 144;
}

//                      ---- Public functions ----

void ui::IconMenu::update(const sdl2::Input &input, bool hasFocus) { Menu::update(input, hasFocus); }

void ui::IconMenu::render(sdl2::Renderer &renderer, bool hasFocus)
{
    const int optionCount = m_options.size();
    const int y           = m_transition.get_y();
    for (int i = 0, tempY = y; i < optionCount; i++, tempY += m_optionHeight)
    {
        // Set target, clear.
        {
            graphics::ScopedRender optionScope{renderer, m_optionTarget};
            renderer.frame_begin(colors::TRANSPARENT);
            if (i == m_selected)
            {
                if (hasFocus)
                {
                    m_boundingBox->set_x(m_x - 8);
                    m_boundingBox->set_y(tempY - 8);
                    m_boundingBox->render(renderer, hasFocus);
                }
                // This is always rendered.
                renderer.render_rectangle(0, 0, 4, 130, colors::BLUE_GREEN);
            }

            m_options[i]->render_stretched(8, 1, ICON_RENDER_WIDTH, ICON_RENDER_HEIGHT);
        }

        m_optionTarget->render(m_x, tempY);
    }
}

void ui::IconMenu::add_option(sdl2::SharedTexture newOption)
{
    Menu::add_option("ICON"); // Parent class needs text for this to work correctly.
    m_options.push_back(newOption);
}
