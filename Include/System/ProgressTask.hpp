#pragma once
#include "System/Task.hpp"

namespace System
{
    class ProgressTask : public System::Task
    {
        public:
            template <typename... Args>
            ProgressTask(void (*Function)(System::ProgressTask *, Args...), Args... Arguments)
                : System::Task(Function, this, std::forward<Args>(Arguments)...){};

            // Resets current down to 0 and sets goal
            void Reset(double Goal);
            // Sets current value
            void UpdateCurrent(double Current);

            // Returns progress
            double GetCurrentProgress(void) const;

        private:
            // Current value and goal
            double m_Current, m_Goal;
    };
} // namespace System
