#pragma once
#include <switch.h>
#include "appStates/appState.hpp"
#include "system/task.hpp"

class taskState : public appState
{
    public:
        taskState(sys::taskFunction function, std::shared_ptr<sys::taskArgs> args);
        ~taskState();

        void update(void);
        void render(void);

    protected:
        std::unique_ptr<sys::task> m_Task;
};
