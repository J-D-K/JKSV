#include "StringUtil.hpp"
#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstring>
#include <switch.h>

namespace
{
    // Size limit for formatted strings.
    constexpr size_t VA_BUFFER_SIZE = 0x1000;
    // These characters get replaced by spaces when path is sanitized.
    constexpr std::array<uint32_t, 13> FORBIDDEN_PATH_CHARACTERS =
        {L',', L'/', L'\\', L'<', L'>', L':', L'"', L'|', L'?', L'*', L'™', L'©', L'®'};
} // namespace

std::string StringUtil::GetFormattedString(const char *Format, ...)
{
    char VaBuffer[VA_BUFFER_SIZE];

    std::va_list VaList;
    va_start(VaList, Format);
    vsnprintf(VaBuffer, VA_BUFFER_SIZE, Format, VaList);
    va_end(VaList);

    return std::string(VaBuffer);
}

void StringUtil::ReplaceInString(std::string &Target, std::string_view Find, std::string_view Replace)
{
    size_t StringPosition = 0;
    while ((StringPosition = Target.find(Find, StringPosition)) != Target.npos)
    {
        Target.replace(StringPosition, Find.length(), Replace);
    }
}

bool StringUtil::SanitizeStringForPath(const char *StringIn, char *StringOut, size_t StringOutSize)
{
    uint32_t Codepoint = 0;
    size_t StringLength = std::strlen(StringIn);
    for (size_t i = 0, StringOutOffset = 0; i < StringLength;)
    {
        ssize_t UnitCount = decode_utf8(&Codepoint, reinterpret_cast<const uint8_t *>(&StringIn[i]));
        if (UnitCount <= 0 || i + UnitCount >= StringOutSize)
        {
            break;
        }

        if (Codepoint < 0x20 || Codepoint > 0x7E)
        {
            // Don't even bother. It's not possible.
            return false;
        }

        // Replace forbidden with spaces.
        if (std::find(FORBIDDEN_PATH_CHARACTERS.begin(), FORBIDDEN_PATH_CHARACTERS.end(), Codepoint) != FORBIDDEN_PATH_CHARACTERS.end())
        {
            StringOut[StringOutOffset++] = 0x20;
        }
        else if (Codepoint == L'é')
        {
            StringOut[StringOutOffset++] = 'e';
        }
        else
        {
            // Just memcpy it over. This is a safety thing to be honest. Since it's only Ascii allowed, unitcount should only be 1.
            std::memcpy(&StringOut[StringOutOffset], &StringIn[i], static_cast<size_t>(UnitCount));
            StringOutOffset += UnitCount;
        }
        i += UnitCount;
    }

    // Loop backwards and trim off spaces and periods.
    size_t StringOutLength = std::strlen(StringOut);
    while (StringOut[StringOutLength - 1] == ' ' || StringOut[StringOutLength - 1] == '.')
    {
        StringOut[--StringOutLength] = 0x00;
    }
    return true;
}
