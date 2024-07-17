#include "system/progressTask.hpp"
#include "log.hpp"

sys::progressTask::progressTask(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args) :
task::task(threadFunction, this, args) { }

sys::progressTask::progressTask(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args, uint64_t maxValue) :
task::task(threadFunction, this, args),
m_MaxValue(maxValue) { }

sys::progressTask::~progressTask() { }

double sys::progressTask::getMax(void)
{
    return m_MaxValue;
}

void sys::progressTask::setMax(uint64_t newMax)
{
    m_MaxValueMutex.lock();
    m_MaxValue = newMax;
    m_MaxValueMutex.unlock();
}

double sys::progressTask::getProgress(void)
{
    return m_Progress;
}

void sys::progressTask::updateProgress(uint64_t newProgress)
{
    m_ProgressMutex.lock();
    m_Progress = newProgress;
    m_ProgressMutex.unlock();
}

double sys::progressTask::getTotalProgress(void)
{
    return m_Progress / m_MaxValue;
}

void sys::progressTask::reset(void)
{
    m_Progress = 0;
}