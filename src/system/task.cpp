#include "system/task.hpp"

#include "log.hpp"

sys::task::task(sys::taskFunction threadFunction, sys::sharedTaskData sharedData) :
m_Thread(std::make_unique<std::thread>(threadFunction, this, sharedData)) { }

sys::task::task(sys::taskFunction threadFunction, sys::task *childTask, sys::sharedTaskData sharedData) :
m_Thread(std::make_unique<std::thread>(threadFunction, childTask, sharedData)) { }

sys::task::~task()
{
    // Wait for thread to finish.
    m_Thread->join();
}

std::string sys::task::getThreadStatus(void)
{
    // Like this to prevent corruption
    std::string threadStatus;

    m_StatusMutex.lock();
    threadStatus = m_ThreadStatus;
    m_StatusMutex.unlock();

    return threadStatus;
}

void sys::task::setThreadStatus(const std::string &newStatus)
{
    m_StatusMutex.lock();
    m_ThreadStatus = newStatus;
    m_StatusMutex.unlock();
}

void sys::task::finished(void)
{
    m_RunningMutex.lock();
    m_IsRunning = false;
    m_RunningMutex.unlock();
}

bool sys::task::isRunning(void)
{
    bool threadRunning = false;

    m_RunningMutex.lock();
    threadRunning = m_IsRunning;
    m_RunningMutex.unlock();

    return threadRunning;
}