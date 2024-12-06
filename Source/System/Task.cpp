#include "Task.hpp"
#include <cstdarg>

namespace
{
    constexpr size_t VA_BUFFER_SIZE = 0x1000;
}

System::Task::~Task()
{
    m_Thread.join();
}

bool System::Task::IsRunning(void) const
{
    return m_IsRunning;
}

void System::Task::Finished(void)
{
    m_IsRunning = false;
}

void System::Task::SetStatus(const char *Format, ...)
{
    char VaBuffer[VA_BUFFER_SIZE];

    std::va_list VaList;
    va_start(VaList, Format);
    vsnprintf(VaBuffer, VA_BUFFER_SIZE, Format, VaList);
    va_end(VaList);

    std::scoped_lock<std::mutex> StatusLock(m_StatusLock);
    m_Status = VaBuffer;
}

std::string System::Task::GetStatus(void)
{
    std::scoped_lock<std::mutex> StatusLock(m_StatusLock);
    return m_Status;
}
