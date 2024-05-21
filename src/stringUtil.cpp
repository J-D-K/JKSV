#include <cstdarg>
#include <cstdint>
#include <ctime>
#include <switch.h>
#include "stringUtil.hpp"

// Chars that shouldn't be in paths & function to test for them
static const uint32_t forbiddenPathChars[] = {L',', L'/', L'\\', L'<', L'>', L':', L'"', L'|', L'?', L'*', L'™', L'©', L'®'};

static bool characterIsForbidden(const uint32_t &codepoint)
{
    for (int i = 0; i < 13; i++)
    {
        if (codepoint == forbiddenPathChars[i])
        {
            return true;
        }
    }
    return false;
}

// Tests if a codepoint falls within ASCII range
static bool codepointIsASCII(const uint32_t &codepoint)
{
    return codepoint > 0x1E && codepoint < 0x7F;
}

std::string stringUtil::getFormattedString(const char *format, ...)
{
    char vaBuffer[0x800];
    va_list args;
    va_start(args, format);
    vsnprintf(vaBuffer, 0x800, format, args);
    va_end(args);
    return std::string(vaBuffer);
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

void stringUtil::eraseCharacterFromString(const char &c, std::string &str)
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

std::string stringUtil::getTimeAndDateString(const dateFormats &dateFormat)
{
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
            return stringUtil::getFormattedString("%04d.%02d.%02d @ %02d.%02d.%02d", currentYear, local->tm_mon + 1, local->tm_mday, local->tm_min, local->tm_sec);
        }
        break;

        case stringUtil::dateFormats::DATE_FORMAT_YDM:
        {
            return stringUtil::getFormattedString("%04d.%02d.%02d @ %02d.%02d.%02d", currentYear, local->tm_mday, local->tm_mon + 1, local->tm_hour, local->tm_min, local->tm_sec);
        }
        break;

        case stringUtil::dateFormats::DATE_FORMAT_ASC:
        {
            std::string dateString = stringUtil::getFormattedString("%04d%02d%02d_%02d%02d", currentYear, local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min);
            stringUtil::replaceInString(dateString, ":", "_");
            stringUtil::replaceInString(dateString, "\n", 0x00);
            return dateString;
        }
        break;
    }
    return std::string("");
}