#pragma once
#include "AppStates/AppState.hpp"
#include "System/Task.hpp"

class TaskState : public AppState
{
    public:
        template <typename... Args>
        TaskState(void (*Function)(Args...), Args... Arguments) : m_Task(Function, std::forward<Args>(Arguments)...){};
        ~TaskState() {};

        void Update(void);
        void Render(void);

    private:
        // Underlying task.
        System::Task m_Task;
};
