#pragma once
#include <mutex>
#include <string>
#include <thread>

namespace System
{
    class Task
    {
        public:
            template <typename... Args>
            Task(void (*Function)(System::Task *, Args...), Args... Arguments)
            {
                m_Thread = std::thread(Function, this, std::forward<Args>(Arguments)...);
            }

            // This is an alternate constructor that passes through a pointer to a derived class.
            template <typename... Args>
            Task(void (*Function)(System::Task *, Args...), System::Task *Task, Args... Arguments)
            {
                m_Thread = std::thread(Function, Task, std::forward<Args>(Arguments)...);
            }

            virtual ~Task();

            // Returns if thread is still running.
            bool IsRunning(void) const;
            // Signals thread is finished running.
            void Finished(void);

            // Sets status to display.
            void SetStatus(const char *Format, ...);
            // Returns status string.
            std::string GetStatus(void);

        private:
            // Whether task is still running.
            bool m_IsRunning = true;
            // Status string the thread can set that the main thread can display.
            std::string m_Status;
            // Mutex so that string doesn't get messed up.
            std::mutex m_StatusLock;
            // Thread
            std::thread m_Thread;
    };
} // namespace System
