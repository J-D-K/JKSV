#pragma once
#include <string>

namespace StringUtil
{
    // Gets a string formatted with va args.
    std::string GetFormattedString(const char *Format, ...);
    // Replaces a sequence of characters in a string with another.
    void ReplaceInString(std::string &Target, std::string_view Find, std::string_view Replace);
    // Tries to make string path safe. Returns false if it's not possible.
    bool SanitizeStringForPath(const char *StringIn, char *StringOut, size_t StringOutSize);
} // namespace StringUtil
