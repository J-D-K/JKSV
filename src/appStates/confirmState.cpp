#include "appStates/confirmState.hpp"
#include "appStates/taskState.hpp"
#include "graphics/graphics.hpp"
#include "ui/ui.hpp"
#include "system/input.hpp"
#include "jksv.hpp"
#include "log.hpp"

static const int HOLD_TICKS = 1000;

confirmState::confirmState(const std::string &message, sys::taskFunction onConfirmation, std::shared_ptr<sys::taskArgs> args) : 
m_Message(message),
m_Yes(ui::strings::getString(LANG_DIALOG_YES, 0)),
m_No(ui::strings::getString(LANG_DIALOG_NO, 0)),
m_HoldTimer(std::make_unique<sys::timer>(HOLD_TICKS)), 
m_OnConfirmation(onConfirmation), 
m_Args(args) { }

confirmState::~confirmState() { }

// To do: I don't like the way this looks, but not sure if it can get much better
void confirmState::update(void)
{
    // If A is first pressed, and stage is 0, restart timer
    if(sys::input::buttonDown(HidNpadButton_A) && m_HoldStage == 0)
    {
        m_HoldTimer->restartTimer();
    }
    else if(sys::input::buttonHeld(HidNpadButton_A) && m_HoldStage < 3 && m_HoldTimer->triggered())
    {
        // Confirmed, create new task.
        if(++m_HoldStage == 3)
        {
            // Create new task
            std::unique_ptr<appState> confirmationTask = std::make_unique<taskState>(m_OnConfirmation, m_Args);
            jksv::pushNewState(confirmationTask);
            // Deactivate this one and pray JKSV catches it
            appState::deactivateState();
        }
        else
        {
            // Switch and update yes accordingly
            switch (m_HoldStage)
            {
                case 0:
                    {
                        m_Yes = ui::strings::getString(LANG_HOLDING_TEXT, 0);
                    }
                    break;

                case 1:
                    {
                        m_Yes = ui::strings::getString(LANG_HOLDING_TEXT, 1);
                    }
                    break;

                case 2:
                    {
                        m_Yes = ui::strings::getString(LANG_HOLDING_TEXT, 2);
                    }
                    break;
            }
        }
    }
    else if(sys::input::buttonReleased(HidNpadButton_A))
    {
        m_Yes = ui::strings::getString(LANG_DIALOG_YES, 0);
    }
    else if(sys::input::buttonDown(HidNpadButton_B))
    {
        // Party's over. Just stop
        appState::deactivateState();
    }
}

void confirmState::render(void)
{
    // Darken underlying
    graphics::renderRect(NULL, 0, 0, 1280, 720, COLOR_DIM_BACKGROUND);

    // Calculate X position of m_Yes because it changes on hold
    int yesXPosition = 458 - (graphics::systemFont::getTextWidth(m_Yes, 18) / 2);

    // Render dialog box
    ui::renderDialogBox(NULL, 280, 262, 720, 256);
    graphics::systemFont::renderTextWrap(m_Message, NULL, 312, 288, 18, 656, COLOR_WHITE);
    graphics::renderLine(NULL, 280, 454, 999, 454, COLOR_DARK_GRAY);
    graphics::renderLine(NULL, 640, 454, 640, 517, COLOR_DARK_GRAY);
    graphics::systemFont::renderText(m_Yes, NULL, yesXPosition, 478, 18, COLOR_WHITE);
    graphics::systemFont::renderText(m_No, NULL, 782, 478, 18, COLOR_WHITE);
}