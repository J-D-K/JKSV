#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include "system/task.hpp"

// This is for threads and functions that need to report and show progress being m
namespace sys
{
    // This is like a task, but can keep track of progress of the tasks such as file copying and uploading.
    class progressTask : public sys::task
    {
        public:
            // Basically the same as a regular task. Second allows setting maximum immediately.
            progressTask(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args);
            progressTask(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args, const uint64_t &maxValue);
            ~progressTask();
            // Resets m_Progress back to 0.
            void reset(void);
            // Gets the current maximum/goal
            double getMax(void);
            // Sets the goal value to be reached.
            void setMax(const uint64_t &newMax);
            // Gets the current m_Progress itself
            double getProgress(void);
            // Updates the progress variable.
            void updateProgress(const uint64_t &newProgress);
            // Returns progress as a percentage
            double getTotalProgress(void);

        private:
            // Goal/Maximum value to reach
            double m_MaxValue = 0;
            // Total progress so far
            double m_Progress = 0;
            // Mutex for m_MaxValue
            std::mutex m_MaxValueMutex;
            // Progress mutex
            std::mutex m_ProgressMutex;
    };
}
