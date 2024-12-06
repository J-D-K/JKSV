#include "StringUtil.hpp"
#include <cstdarg>

namespace
{
    constexpr size_t VA_BUFFER_SIZE = 0x1000;
}

std::string StringUtil::GetFormattedString(const char *Format, ...)
{
    char VaBuffer[VA_BUFFER_SIZE];

    std::va_list VaList;
    va_start(VaList, Format);
    vsnprintf(VaBuffer, VA_BUFFER_SIZE, Format, VaList);
    va_end(VaList);

    return std::string(VaBuffer);
}
