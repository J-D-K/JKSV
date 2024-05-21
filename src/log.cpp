#include <cstdarg>
#include <mutex>
#include "log.hpp"

namespace
{
    std::ofstream s_LogFile;
    std::mutex s_LogMutex;
}

void logger::init(void)
{
    s_LogFile.open("sdmc:/JKSV/log.txt");
}

void logger::log(const char *format, ...)
{
    char vaBuffer[0x800];
    va_list args;
    va_start(args, format);
    vsnprintf(vaBuffer, 0x800, format, args);
    va_end(args);

    // JIC because of task threads
    s_LogMutex.lock();
    s_LogFile << vaBuffer << std::endl; // Using std::endl for flush for more reliable logging
    s_LogMutex.unlock();
}