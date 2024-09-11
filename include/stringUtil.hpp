#pragma once
#include <string>

namespace stringUtil
{
    // Not sure where else this could go
    typedef enum
    {
        DATE_FORMAT_YMD,
        DATE_FORMAT_YDM,
        DATE_FORMAT_ASC
    } dateFormats;

    // Returns va_list as C++ string
    std::string getFormattedString(const char *format, ...);
    // Returns a string safe to use in filepaths, or empty string if not possible
    std::string getPathSafeString(const std::string &str);
    // Erases character 'c' from string
    void eraseCharacterFromString(char c, std::string &str);
    // Replaces all 'find' in 'str' with 'replace'
    void replaceInString(std::string &str, const std::string &find, const std::string &replace);
    // Gets filename from string
    std::string getFilenameFromString(const std::string &path);
    // Gets extension from string
    std::string getExtensionFromString(const std::string &path);
    // This is just for now. Don't know where else it should go.
    std::string getTimeAndDateString(stringUtil::dateFormats dateFormat);
}