#pragma once
#include <memory>
#include <string>
#include <cstdint>

#include "appStates/appState.hpp"
#include "appStates/taskState.hpp"

#include "system/progressTask.hpp"
#include "system/taskData.hpp"

class progressState : public appState
{
    public:
        // This is like taskState, but can display the progress of an operation. Only really used for copying and uploading
        progressState(sys::taskFunction threadFunction, sys::sharedTaskData sharedData);
        ~progressState();
        void update(void);
        void render(void);

    private:
        // Task to be run
        std::unique_ptr<sys::progressTask> m_Task;
        // Width of progress to be rendered
        double m_BarWidth = 0;
        // String to display current task status
        std::string m_TaskStatus;
        // String to display current progress of task
        std::string m_ProgressStatus;
        // X coordinate to render task status to
        int m_ProgressStatusX = 0;
};

void createAndPushNewProgressState(sys::taskFunction threadFunction, sys::sharedTaskData sharedData);