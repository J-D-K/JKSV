#include "ui/progressBar.hpp"
#include "graphics/graphics.hpp"
#include "graphics/systemFont.hpp"
#include "ui/ui.hpp"
#include "stringUtil.hpp"

ui::progressBar::progressBar(const uint64_t &maxValue) : m_MaxValue(maxValue) { }

void ui::progressBar::update(const uint64_t &progress)
{
    m_Progress = progress;

    // Calculat bar width
    if(m_MaxValue > 0)
    {
        m_BarWidth = (static_cast<float>(m_Progress) / static_cast<float>(m_MaxValue)) * 648.0f;
    }

    // Get updated string
    m_ProgressString = stringUtil::getFormattedString("%.2fMB / %.2fMB", static_cast<float>(m_Progress) / 1024.0f / 1024.0f, static_cast<float>(m_MaxValue) / 1024.0f / 1024.0f);
}

void ui::progressBar::render(const std::string &text, SDL_Texture *target)
{
    // Calculate X position of progress string
    int progressX = 640 - (graphics::systemFont::getTextWidth(m_ProgressString, 18) / 2);

    ui::renderDialogBox(target, 280, 262, 720, 256);
    graphics::renderRect(target, 312, 471, 484, 32, COLOR_BLACK);
    graphics::renderRect(target, 312, 471, m_BarWidth, 32, COLOR_GREEN);
    graphics::systemFont::renderText(m_ProgressString, target, progressX, 478, 18, COLOR_WHITE);
}