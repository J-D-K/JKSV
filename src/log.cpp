#include <array>
#include <cstdarg>
#include <mutex>
#include "log.hpp"

// Log path
static const char *LOG_OUTPUT_PATH = "/JKSV/log.txt";

// Size for va 
static const int LOG_VA_BUFFER_SIZE = 0x1000;

namespace
{
    std::ofstream s_LogFile;
    std::mutex s_LogMutex;
}

void logger::init(void)
{
    // Open and close just to clear.
    s_LogFile.open(LOG_OUTPUT_PATH);
    s_LogFile.close();
}

void logger::log(const char *format, ...)
{
    // Open log for appending
    s_LogFile.open(LOG_OUTPUT_PATH, std::ios::app);

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

    // Close
    s_LogFile.close();
}