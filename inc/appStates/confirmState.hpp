#pragma once
#include <memory>
#include <string>
#include <switch.h>
#include "appStates/appState.hpp"
#include "system/timer.hpp"
#include "system/task.hpp"

class confirmState : public appState
{
    public:
        // Message is the question asked. onConfirmation is the function executed if user confirms. Args is the shared_ptr sent to onConfirmation
        confirmState(const std::string &message, sys::taskFunction onConfirmation, std::shared_ptr<sys::taskArgs> args, const sys::taskTypes &taskType);
        ~confirmState();

        void update(void);
        void render(void);

    private:
        // Message being displayed
        std::string m_Message;
        // Yes and no text
        std::string m_Yes;
        std::string m_No;
        // Timer for holding to confirm
        std::unique_ptr<sys::timer> m_HoldTimer;
        // Int for what stage for holding
        int8_t m_HoldStage = 0;
        // Function on confirm
        sys::taskFunction m_OnConfirmation;
        // Args to send m_OnConfirmation
        std::shared_ptr<sys::taskArgs> m_Args;
        // The type of task to be created on confirmation
        sys::taskTypes m_TaskType;
};

// Shortcut function
void confirmAction(const std::string &message, sys::taskFunction onConfirmation, std::shared_ptr<sys::taskArgs> args, const sys::taskTypes &taskType);