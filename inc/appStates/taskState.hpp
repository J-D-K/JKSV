#pragma once
#include <memory>
#include <switch.h>
#include "appStates/appState.hpp"
#include "system/timer.hpp"
#include "system/task.hpp"

class taskState : public appState
{
    public:
        taskState(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args);
        ~taskState();

        void update(void);
        void render(void);

    protected:
        // Task that is created and runs
        std::unique_ptr<sys::task> m_Task;
};

void createAndPushNewTask(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args);