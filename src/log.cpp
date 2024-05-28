#include <array>
#include <cstdarg>
#include <mutex>
#include "log.hpp"

static const int LOG_VA_BUFFER_SIZE = 0x1000;

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
    // Buffer for va
    std::array<char, LOG_VA_BUFFER_SIZE> vaBuffer;

    // VA
    std::va_list args;
    va_start(args, format);
    vsnprintf(vaBuffer.data(), LOG_VA_BUFFER_SIZE, format, args);
    va_end(args);

    // This is because of threading
    s_LogMutex.lock();
    s_LogFile << vaBuffer.data() << std::endl; // endl for buffer flushing
    s_LogMutex.unlock();
}