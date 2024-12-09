#pragma once
#include "FsLib.hpp"
#include <string_view>

namespace Config
{
    // Attempts to load config from file. If that fails, initializes to default.
    void Initialize(void);
    // Resets config to default settings.
    void ResetToDefault(void);
    // Saves config.
    void Save(void);
    // Retrieves a value with its key.
    uint8_t GetByKey(std::string_view Key);
    // Retrieves value by index.
    uint8_t GetByIndex(int Index);
    // Gets the working directory.
    FsLib::Path GetWorkingDirectory(void);
    // Gets the UI's transition scaling
    double GetAnimationScaling(void);

    // Adds or removes title from favorites.
    void AddRemoveFavorite(uint64_t TitleID);
    // Returns if TitleID is in favorites list.
    bool IsFavorite(uint64_t TitleID);

    // Adds or removes a title from the blacklist
    void AddRemoveBlacklist(uint64_t TitleID);
    // Returns if TitleID is in blacklist
    bool IsBlacklisted(uint64_t TitleID);

    // Names of keys. Note: Not all of these are retrievable with GetByKey. Some of these are purely for config reading and writing.
    namespace Keys
    {
        static constexpr std::string_view WorkingDirectory = "WorkingDirectory";
        static constexpr std::string_view IncludeDeviceSaves = "IncludeDeviceSaves";
        static constexpr std::string_view AutoBackupOnRestore = "AutoBackupOnRestore";
        static constexpr std::string_view AutoNameBackups = "AutoNameBackups";
        static constexpr std::string_view AutoUpload = "AutoUploadToRemote";
        static constexpr std::string_view HoldForDeletion = "HoldForDeletion";
        static constexpr std::string_view HoldForRestoration = "HoldForRestoration";
        static constexpr std::string_view HoldForOverwrite = "HoldForOverWrite";
        static constexpr std::string_view OnlyListMountable = "OnlyListMountable";
        static constexpr std::string_view ListAccountSystemSaves = "ListAccountSystemSaves";
        static constexpr std::string_view AllowSystemSaveWriting = "AllowSystemSaveWriting";
        static constexpr std::string_view ExportToZip = "ExportToZip";
        static constexpr std::string_view TitleSortType = "TitleSortType";
        static constexpr std::string_view JKSMTextMode = "JKSMTextMode";
        static constexpr std::string_view ForceEnglish = "ForceEnglish";
        static constexpr std::string_view EnableTrashBin = "EnableTrash";
        static constexpr std::string_view UIAnimationScaling = "UIAnimationScaling";
        static constexpr std::string_view Favorites = "Favorites";
        static constexpr std::string_view BlackList = "BlackList";
    } // namespace Keys
} // namespace Config
