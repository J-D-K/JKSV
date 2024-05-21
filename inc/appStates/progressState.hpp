#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include "appStates/appState.hpp"
#include "ui/progressBar.hpp"
#include "system/task.hpp"
#include "system/progressArgs.hpp"

class progressState : public appState
{
    public:
        progressState(sys::taskFunction function, std::shared_ptr<sys::progressArgs> args, const uint64_t &maxValue);
        ~progressState();
        void update(void);
        void render(void);

    private:
        // Task to be run
        std::unique_ptr<sys::task> m_Task;
        // This is the actual progress bar
        std::unique_ptr<ui::progressBar> m_ProgressBar;
        // Args sent to task and this
        std::shared_ptr<sys::progressArgs> m_Args;
};