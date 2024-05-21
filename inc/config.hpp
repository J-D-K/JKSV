#pragma once
#include <cstdint>
#include <string>

// Names of values to avoid typos
#define CONFIG_WORKING_DIRECTORY "workingDirectory"
#define CONFIG_INLCUDE_DEVICE_SAVES "includeDeviceSaves"
#define CONFIG_AUTO_BACKUP_SAVES "autoBackupSaves"
#define CONFIG_OVERCLOCK_FOR_ZIP "overClockForZIP"
#define CONFIG_HOLD_TO_DELETE "holdToDelete"
#define CONFIG_HOLD_TO_RESTORE "holdToRestore"
#define CONFIG_HOLD_TO_OVERWRITE "holdToOverwrite"
#define CONFIG_ONLY_LIST_MOUNTABLE "onlyListMountable"
#define CONFIG_LIST_ACCOUNT_SYSTEM_SAVES "listAccountSystemSaves"
#define CONFIG_ALLOW_SYSTEM_SAVE_WRITING "allowSystemSave"
#define CONFIG_USE_ZIP "useZipFiles"
#define CONFIG_OVERRIDE_LANGUAGE "languageOverride"
#define CONFIG_USE_TRASH_FOLDER "useTrashFolder"
#define CONFIG_AUTONAME_BACKUPS "autoNameBackups"
#define CONFIG_TITLE_SORT_TYPE "titleSortType"
#define CONFIG_FAVORITE_TITLE "favorite"
#define CONFIG_BLACKLIST_TITLE "blacklist"
#define CONFIG_AUTO_UPLOAD_TO_DRIVE "autoUploadToDrive"
#define CONFIG_UI_ANIMATION_SCALING "uiAnimationScale"
#define CONFIG_DRIVE_REFRESH_TOKEN "driveRefreshToken"

namespace config
{
    typedef enum
    {
        SORT_TYPE_ALPHA,
        SORT_TYPE_MOST_PLAYED,
        SORT_TYPE_LAST_PLAYED
    } titleSortTypes;

    // Inits static config class inside config.cpp
    void init(void);
    // Basically just saves config before exiting
    void exit(void);
    // Resets config to default
    void reset(void);

    // Returns the working directory
    std::string getWorkingDirectory(void);
    // Gets the config value by string
    uint8_t getByKey(const std::string &key);
    // Returns the float to use for animations
    float getAnimationScaling(void);
    // Returns if titleID is found in favorites vector
    bool titleIsFavorite(const uint64_t &titleID);
    // Returns if titleID is found in blacklist vector
    bool titleIsBlacklisted(const uint64_t &titleID);

    // Adds or removes titleID from favorites vector
    void addRemoveTitleToFavorites(const uint64_t &titleID);
    // Adds or removes title to blacklist vector
    void addRemoveTitleToBlacklist(const uint64_t &titleID);
}