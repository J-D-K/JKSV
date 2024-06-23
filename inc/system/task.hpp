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
    typedef enum
    {
        TASK_TYPE_TASK,
        TASK_TYPE_PROGRESS
    } taskTypes;

    class task;

    using taskFunction = std::function<void(sys::task *, std::shared_ptr<sys::taskArgs>)>;

    class task
    {
        public:
            // taskFunction is void that takes a sys::task pointer and shared taskArgs pointer
            task(sys::taskFunction threadFunction, std::shared_ptr<taskArgs> args);
            // This can pass through a child class of task. This is for progressTask mostly. Maybe more down the line.
            task(sys::taskFunction threadFunction, sys::task *childTask, std::shared_ptr<taskArgs> args);
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
            // Thread. Orignally used C++ threads, but without control they choked the main thread. LOL nevermind.
            std::unique_ptr<std::thread> m_Thread;
            // Status string
            std::string m_ThreadStatus;
            // Mutexes to protect status and running bool
            std::mutex m_StatusMutex, m_RunningMutex;
            // Whether task still running
            bool m_IsRunning = true;
    };
}