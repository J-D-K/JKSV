#pragma once
#include <string_view>

namespace Strings
{
    // Attempts to load strings from file in RomFS.
    bool Initialize(void);
    // Returns string with name and index. Returns nullptr if string doesn't exist.
    const char *GetByName(std::string_view Name, int Index);
    // Names of strings to prevent typos.
    namespace Names
    {
        static constexpr std::string_view TranslationInfo = "TranslationInfo";
        static constexpr std::string_view ControlGuides = "ControlGuides";
        static constexpr std::string_view SaveDataTypes = "SaveDataTypes";
        static constexpr std::string_view MainMenuNames = "MainMenuNames";
        static constexpr std::string_view SettingsMenu = "SettingsMenu";
        static constexpr std::string_view OnOff = "OnOff";
    } // namespace Names
} // namespace Strings
