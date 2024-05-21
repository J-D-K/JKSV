#include "system/task.hpp"

#define THREAD_STACK_SIZE 0x400000
#define THREAD_PRIORITY 0x2B
#define THREAD_CPU_ID 1

sys::task::task(taskFunction function, std::shared_ptr<taskArgs> args)
{
    m_Thread = std::make_unique<std::thread>(function, this, args);
}

sys::task::~task()
{
    m_Thread->join();
}

std::string sys::task::getThreadStatus(void)
{
    // Like this to prevent corruption
    std::string status;
    m_StatusMutex.lock();
    status.assign(m_ThreadStatus);
    m_StatusMutex.unlock();
    return status;
}

void sys::task::setThreadStatus(const std::string &newStatus)
{
    m_StatusMutex.lock();
    m_ThreadStatus.assign(newStatus);
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
    bool running = false;
    m_RunningMutex.lock();
    running = m_IsRunning;
    m_RunningMutex.unlock();
    return running;
}