#pragma once
#include <memory>
#include <switch.h>
#include "appStates/appState.hpp"
#include "system/timer.hpp"
#include "system/task.hpp"

class taskState : public appState
{
    public:
        taskState(sys::taskFunction threadFunction, sys::sharedTaskData sharedData);
        ~taskState();

        void update(void);
        void render(void);

    protected:
        // Task that is created and runs
        std::unique_ptr<sys::task> m_Task;
};

inline std::shared_ptr<taskState> createNewTask(sys::taskFunction threadFunction, sys::sharedTaskData sharedData)
{
    return std::make_shared<taskState>(threadFunction, sharedData);
}
