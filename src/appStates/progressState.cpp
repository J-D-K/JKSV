#include "appStates/progressState.hpp"
#include "graphics/graphics.hpp"
#include "ui/ui.hpp"
#include "stringUtil.hpp"
#include "jksv.hpp"

namespace
{
    // These are the coordinates and dimensions of the dialog boxrendered to screen
    const int PROG_DIALOG_X = 280;
    const int PROG_DIALOG_Y = 262;
    const int PROG_DIALOG_WIDTH = 720;
    const int PROG_DIALOG_HEIGHT = 256;
    // These are for the actual bar inside the box
    const double PROG_BAR_WIDTH = 648.0f;
    const int PROG_BAR_X = 312;
    const int PROG_BAR_Y = 471;
    //static const int PROG_BAR_WIDTH = 484; Where'd this come from? Try to figure it out later...
    const int PROG_BAR_HEIGHT = 32;
    // These are for the status string
    const int STATUS_TEXT_X = PROG_DIALOG_X + 16;
    const int STATUS_TEXT_Y = PROG_DIALOG_Y + 16;
    const int STATUS_TEXT_MAX_WIDTH = 478;
    const int STATUS_TEXT_FONT_SIZE = 18;
    // For the [X]MB/[X]MB
    const int PROGRESS_STATUS_Y = 478;
    const int PROGRESS_STATUS_FONT_SIZE = 18;
}

progressState::progressState(sys::taskFunction threadFunction, sys::sharedTaskData sharedData) : 
m_Task(std::make_unique<sys::progressTask>(threadFunction, sharedData)) { }

progressState::~progressState() { }

void progressState::update(void)
{
    if(m_Task->isRunning() == false)
    {
        appState::deactivateState();
    }

    // Update the width of the bar
    m_BarWidth = m_Task->getTotalProgress() * PROG_BAR_WIDTH;

    // Update the string in the center of the bar. Measured in MB's
    double taskMax = m_Task->getMax() / 1024 / 1024;
    double taskProgress = m_Task->getProgress() / 1024 / 1024;
    m_ProgressStatus = stringUtil::getFormattedString("%.2fMB / %.2fMB", taskProgress, taskMax);
    // Calculate X of m_ProgressStatus
    m_ProgressStatusX = 640 - (graphics::systemFont::getTextWidth(m_ProgressStatus, PROGRESS_STATUS_FONT_SIZE) / 2);

    // Grab current task status
    m_TaskStatus = m_Task->getThreadStatus();
}

void progressState::render()
{
    // Dim underlying states
    graphics::renderRect(NULL, 0, 0, 1280, 720, COLOR_DIM_BACKGROUND);

    // Render the dialog box, bar and status to screen
    ui::renderDialogBox(NULL, PROG_DIALOG_X, PROG_DIALOG_Y, PROG_DIALOG_WIDTH, PROG_DIALOG_HEIGHT);
    graphics::renderRect(NULL, PROG_BAR_X, PROG_BAR_Y, PROG_BAR_WIDTH, PROG_BAR_HEIGHT, COLOR_BLACK);
    graphics::renderRect(NULL, PROG_BAR_X, PROG_BAR_Y, m_BarWidth, PROG_BAR_HEIGHT, COLOR_GREEN);
    graphics::systemFont::renderTextWrap(m_TaskStatus, NULL, STATUS_TEXT_X, STATUS_TEXT_Y, STATUS_TEXT_FONT_SIZE, STATUS_TEXT_MAX_WIDTH, COLOR_WHITE); // Status
    graphics::systemFont::renderText(m_ProgressStatus, NULL, m_ProgressStatusX, PROGRESS_STATUS_Y, PROGRESS_STATUS_FONT_SIZE, COLOR_WHITE); // Actual progress
}

void createAndPushNewProgressState(sys::taskFunction threadFunction, sys::sharedTaskData sharedData)
{
    std::unique_ptr<appState> newProgressState = std::make_unique<progressState>(threadFunction, sharedData);
    jksv::pushNewState(newProgressState);
}