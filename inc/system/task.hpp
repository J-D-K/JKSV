#pragma once
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <switch.h>
#include "system/taskArgs.hpp"

namespace sys
{
    class task;

    using taskFunction = std::function<void(task *, std::shared_ptr<sys::taskArgs>)>;

    class task
    {
        public:
            // taskFunction is 
            task(taskFunction function, std::shared_ptr<taskArgs> args);
            ~task();
            // Returns status string of task. 
            std::string getThreadStatus(void);
            // Sets status of task
            void setThreadStatus(const std::string &newStatus);
            // Sets m_IsRunning to false
            void finished(void);
            // Returns if task
            bool isRunning(void);
            
        private:
            // Thread
            std::unique_ptr<std::thread> m_Thread;
            // Status string
            std::string m_ThreadStatus;
            // Mutexes to protect status and running bool
            std::mutex m_StatusMutex, m_RunningMutex;
            // Whether task still running
            bool m_IsRunning = false;
    };
}