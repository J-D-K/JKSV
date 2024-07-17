#include <algorithm>
#include <array>
#include <string>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <ctime>
#include <switch.h>
#include "stringUtil.hpp"

namespace
{
    // Size for VA
    const int VA_BUFFER_SIZE = 0x1000;
    // Characters forbidden from file paths
    std::array<uint32_t, 13> FORBIDDEN_PATH_CHARACTERS = { L',', L'/', L'\\', L'<', L'>', L':', L'"', L'|', L'?', L'*', L'™', L'©', L'®' };
}

static bool characterIsForbidden(uint32_t codepoint)
{
    if(std::find(FORBIDDEN_PATH_CHARACTERS.begin(), FORBIDDEN_PATH_CHARACTERS.end(), codepoint) != FORBIDDEN_PATH_CHARACTERS.end())
    {
        return true;
    }
    return false;
}

// Tests if a codepoint falls within ASCII range
static bool codepointIsASCII(uint32_t codepoint)
{
    return codepoint > 0x1E && codepoint < 0x7F;
}

std::string stringUtil::getFormattedString(const char *format, ...)
{
    // Use C++ array for fun
    std::array<char, VA_BUFFER_SIZE> vaBuffer;
    // Use va to assemble string
    va_list args;
    va_start(args, format);
    vsnprintf(vaBuffer.data(), VA_BUFFER_SIZE, format, args);
    va_end(args);
    // Return array as C++ string
    return std::string(vaBuffer.data());
}

std::string stringUtil::getPathSafeString(const std::string &str)
{
    std::string returnString;
    for (unsigned int i = 0; i < str.length();)
    {
        uint32_t codepoint = 0x00;
        ssize_t unitCount = decode_utf8(&codepoint, reinterpret_cast<const uint8_t *>(&str.c_str()[i]));

        // This means it probably isn't possible without mangling the entire string
        if (!codepointIsASCII(codepoint))
        {
            return std::string("");
        }
        else if (codepoint == L'é') // Pokemon
        {
            returnString.append("e");
        }
        else if (characterIsForbidden(codepoint))
        {
            returnString.append(" ");
        }
        else
        {
            returnString.append(reinterpret_cast<const char *>(&codepoint));
        }
        i += unitCount;
    }

    // Loop and remove spaces at the end if there are any
    while(returnString.at(returnString.length() - 1) == ' ' || returnString.at(returnString.length() - 1) == '.')
    {
        returnString.erase(returnString.length() - 1, 1);
    }

    return returnString;
}

void stringUtil::eraseCharacterFromString(char c, std::string &str)
{
    size_t charPosition = 0;
    while((charPosition = str.find(c, charPosition)) != str.npos)
    {
        str.erase(charPosition, 1);
    }
}

void stringUtil::replaceInString(std::string &str, const std::string &find, const std::string &replace)
{
    size_t position = 0;
    while((position = str.find(find)) != str.npos)
    {
        str.replace(position, find.length(), replace);
    }
}

std::string stringUtil::getFilenameFromString(const std::string &path)
{
    size_t lastSlash = path.find_last_of('/');
    size_t extensionBegin = path.find_last_of('.');
    if(lastSlash != path.npos && extensionBegin != path.npos)
    {
        return path.substr(lastSlash + 1, extensionBegin);
    }
    return std::string("");
}

std::string stringUtil::getExtensionFromString(const std::string &path)
{
    size_t extensionBegin = path.find_last_of('.');
    if(extensionBegin != path.npos)
    {
        return path.substr(extensionBegin + 1, path.npos);
    }
    return std::string("");
}

std::string stringUtil::getTimeAndDateString(stringUtil::dateFormats dateFormat)
{
    // String to return
    std::string dateString;

    // Get local time from system
    std::time_t rawTime;
    std::time(&rawTime);
    std::tm *local = localtime(&rawTime);

    // So I don't have to constantly add 1900
    int currentYear = local->tm_year + 1900;

    switch(dateFormat)
    {
        case stringUtil::dateFormats::DATE_FORMAT_YMD:
        {
            dateString = stringUtil::getFormattedString("%04d.%02d.%02d @ %02d.%02d.%02d", currentYear, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
        }
        break;

        case stringUtil::dateFormats::DATE_FORMAT_YDM:
        {
            dateString = stringUtil::getFormattedString("%04d.%02d.%02d @ %02d.%02d.%02d", currentYear, local->tm_mday, local->tm_mon + 1, local->tm_hour, local->tm_min, local->tm_sec);
        }
        break;

        case stringUtil::dateFormats::DATE_FORMAT_ASC:
        {
            // Assign date
            dateString.assign(std::asctime(local));
            // These need to be removed
            stringUtil::replaceInString(dateString, ":", "_");
            stringUtil::replaceInString(dateString, "\n", 0x00);
        }
        break;
    }
    return dateString;
}