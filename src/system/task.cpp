#include "system/task.hpp"
#include "log.hpp"

static const size_t THREAD_STACK_SIZE = 0x80000;
static const int THREAD_PRIORITY = 0x2B;
static const int THREAD_CPU_ID = 1;

sys::task::task(sys::taskFunction threadFunction, std::shared_ptr<taskArgs> args)
{
    // Spawn thread
    m_Thread = std::make_unique<std::thread>(threadFunction, this, args);

    // Let's just hope it's running
    m_IsRunning = true;
}

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