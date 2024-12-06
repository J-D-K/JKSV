#include "Logger.hpp"
#include "FsLib.hpp"
#include <cstdarg>

namespace
{
    // Path to log file.
    FsLib::Path s_LogFilePath;
    // Size of va buffer for log.
    constexpr size_t VA_BUFFER_SIZE = 0x1000;
} // namespace

void Logger::Initialize(void)
{
    // To do: Update this once config is implemented.
    s_LogFilePath = "sdmc:/JKSV/JKSV.log";
    FsLib::File LogFile(s_LogFilePath, FsOpenMode_Create | FsOpenMode_Write);
}

void Logger::Log(const char *Format, ...)
{
    char VaBuffer[VA_BUFFER_SIZE] = {0};

    std::va_list VaList;
    va_start(VaList, Format);
    vsnprintf(VaBuffer, VA_BUFFER_SIZE, Format, VaList);
    va_end(VaList);

    FsLib::File LogFile(s_LogFilePath, FsOpenMode_Append);
    LogFile << VaBuffer << "\n";
    LogFile.Flush();
}
