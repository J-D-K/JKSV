#include <algorithm>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "filesystem/fileParser.hpp"
#include "config.hpp"
#include "log.hpp"

namespace
{
    // This is where most config values live
    std::map<std::string, uint8_t> s_ConfigValues;
    std::string s_WorkingDirectory;
    std::string s_DriveRefreshToken;
    float s_UIAnimationScaling;
    std::vector<uint64_t> s_Favorites;
    std::vector<uint64_t> s_Blacklist;

    std::map<std::string, int> s_ConfigKeys =
    {
        {CONFIG_WORKING_DIRECTORY, 0}, {CONFIG_INLCUDE_DEVICE_SAVES, 1}, {CONFIG_AUTO_BACKUP_SAVES, 2}, {CONFIG_OVERCLOCK_FOR_ZIP, 3}, 
        {CONFIG_HOLD_TO_DELETE, 4}, {CONFIG_HOLD_TO_RESTORE, 5}, {CONFIG_HOLD_TO_OVERWRITE, 6}, {CONFIG_ONLY_LIST_MOUNTABLE, 7}, 
        {CONFIG_LIST_ACCOUNT_SYSTEM_SAVES, 8}, {CONFIG_ALLOW_SYSTEM_SAVE_WRITING, 9}, {CONFIG_USE_ZIP, 10}, {CONFIG_OVERRIDE_LANGUAGE, 11}, 
        {CONFIG_USE_TRASH_FOLDER, 12}, {CONFIG_TITLE_SORT_TYPE, 13}, {CONFIG_UI_ANIMATION_SCALING, 14}, {CONFIG_FAVORITE_TITLE, 15}, 
        {CONFIG_BLACKLIST_TITLE, 16}, {CONFIG_AUTONAME_BACKUPS, 16}, {CONFIG_DRIVE_REFRESH_TOKEN, 17}
    };
}

// For switch case
enum
{
    CASE_WORKING_DIR,
    CASE_INCLUDE_DEVICE,
    CASE_AUTO_BACKUP,
    CASE_OVERCLOCK_ZIP,
    CASE_HOLD_DELETE,
    CASE_HOLD_RESTORE,
    CASE_HOLD_OVERWRITE,
    CASE_ONLY_MOUNTABLE,
    CASE_LIST_ACCOUNT_SYSTEM,
    CASE_ALLOW_SYSTEM_SAVE,
    CASE_USE_ZIP,
    CASE_OVERRIDE_LANG,
    CASE_USE_TRASH,
    CASE_SORT_TYPE,
    CASE_UI_ANIM_SCALE,
    CASE_FAVORITE,
    CASE_BLACKLIST,
    CASE_AUTONAME,
    CASE_DRIVE_REFRESH
};

void config::init()
{
    // Make sure directory exists for sure
    std::filesystem::create_directories("sdmc:/config/JKSV");

    fs::fileParser configParser("sdmc:/config/JKSV/JKSV.cfg");
    if (!configParser.isOpen())
    {
        config::reset();
        return;
    }

    while (configParser.readLine())
    {
        // Get next line, skip if not a valid configKey
        std::string valueName = configParser.getCurrentItemName();
        if (s_ConfigKeys.find(valueName) == s_ConfigKeys.end())
        {
            logger::log("config: Key \"%s\" not found.", valueName);
            continue;
        }

        switch (s_ConfigKeys[valueName])
        {
            case CASE_WORKING_DIR:
            {
                s_WorkingDirectory = configParser.getNextValueAsString();
            }
            break;

            case CASE_INCLUDE_DEVICE:
            {
                s_ConfigValues[CONFIG_INLCUDE_DEVICE_SAVES] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_AUTO_BACKUP:
            {
                s_ConfigValues[CONFIG_AUTO_BACKUP_SAVES] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_OVERCLOCK_ZIP:
            {
                s_ConfigValues[CONFIG_OVERCLOCK_FOR_ZIP] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_HOLD_DELETE:
            {
                s_ConfigValues[CONFIG_HOLD_TO_DELETE] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_HOLD_RESTORE:
            {
                s_ConfigValues[CONFIG_HOLD_TO_RESTORE] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_HOLD_OVERWRITE:
            {
                s_ConfigValues[CONFIG_HOLD_TO_OVERWRITE] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_ONLY_MOUNTABLE:
            {
                s_ConfigValues[CONFIG_ONLY_LIST_MOUNTABLE] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_LIST_ACCOUNT_SYSTEM:
            {
                s_ConfigValues[CONFIG_LIST_ACCOUNT_SYSTEM_SAVES] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_ALLOW_SYSTEM_SAVE:
            {
                s_ConfigValues[CONFIG_ALLOW_SYSTEM_SAVE_WRITING] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_USE_ZIP:
            {
                s_ConfigValues[CONFIG_USE_ZIP] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_OVERRIDE_LANG:
            {
                s_ConfigValues[CONFIG_OVERRIDE_LANGUAGE] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_USE_TRASH:
            {
                s_ConfigValues[CONFIG_USE_TRASH_FOLDER] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_SORT_TYPE:
            {
                s_ConfigValues[CONFIG_TITLE_SORT_TYPE] = configParser.getNextValueAsInt();
            }
            break;

            case CASE_UI_ANIM_SCALE:
            {
                s_UIAnimationScaling = configParser.getNextValueAsFloat();
            }
            break;

            case CASE_FAVORITE:
            {
                s_Favorites.push_back(configParser.getNextValueAsUint64());
            }
            break;

            case CASE_BLACKLIST:
            {
                s_Blacklist.push_back(configParser.getNextValueAsUint64());
            }
            break;

            case CASE_AUTONAME:
            {
                s_ConfigValues[CONFIG_AUTONAME_BACKUPS] = configParser.getNextValueAsBool();
            }
            break;

            case CASE_DRIVE_REFRESH:
            {
                s_DriveRefreshToken = configParser.getNextValueAsString();
            }
            break;
        }
    }
    logger::log("config::init() succeeded.");
}

void config::exit()
{
    std::ofstream configOut("sdmc:/config/JKSV/JKSV.cfg");
    configOut << CONFIG_WORKING_DIRECTORY << " = \"" << s_WorkingDirectory << "\"\n";
    // Loop through map, write to configOut
    for (auto &c : s_ConfigValues)
    {
        configOut << c.first << " = " << (int)c.second << '\n';
    }
    configOut << CONFIG_UI_ANIMATION_SCALING << " = " << s_UIAnimationScaling << '\n';

    for (auto &f : s_Favorites)
    {
        configOut << CONFIG_FAVORITE_TITLE << " = " << f << '\n';
    }

    for (auto &b : s_Blacklist)
    {
        configOut << CONFIG_BLACKLIST_TITLE << " = " << b << '\n';
    }
}

void config::reset()
{
    s_WorkingDirectory = "sdmc:/JKSV/";
    s_UIAnimationScaling = 3.0f;
    s_ConfigValues[CONFIG_INLCUDE_DEVICE_SAVES] = 0;
    s_ConfigValues[CONFIG_AUTO_BACKUP_SAVES] = false;
    s_ConfigValues[CONFIG_OVERCLOCK_FOR_ZIP] = false;
    s_ConfigValues[CONFIG_HOLD_TO_DELETE] = false;
    s_ConfigValues[CONFIG_HOLD_TO_RESTORE] = false;
    s_ConfigValues[CONFIG_HOLD_TO_OVERWRITE] = false;
    s_ConfigValues[CONFIG_ONLY_LIST_MOUNTABLE] = true;
    s_ConfigValues[CONFIG_LIST_ACCOUNT_SYSTEM_SAVES] = false;
    s_ConfigValues[CONFIG_ALLOW_SYSTEM_SAVE_WRITING] = false;
    s_ConfigValues[CONFIG_USE_ZIP] = false;
    s_ConfigValues[CONFIG_OVERRIDE_LANGUAGE] = false;
    s_ConfigValues[CONFIG_USE_TRASH_FOLDER] = true;
    s_ConfigValues[CONFIG_AUTONAME_BACKUPS] = false;
    s_ConfigValues[CONFIG_AUTO_UPLOAD_TO_DRIVE] = false;
    s_ConfigValues[CONFIG_TITLE_SORT_TYPE] = SORT_TYPE_ALPHA;
}

std::string config::getWorkingDirectory()
{
    return s_WorkingDirectory;
}

uint8_t config::getByKey(const std::string &key)
{
    return s_ConfigValues[key];
}

float config::getAnimationScaling()
{
    return s_UIAnimationScaling;
}

bool config::titleIsFavorite(const uint64_t &titleID)
{
    if (std::find(s_Favorites.begin(), s_Favorites.end(), titleID) != s_Favorites.end())
    {
        return true;
    }

    return false;
}

bool config::titleIsBlacklisted(const uint64_t &titleID)
{
    if (std::find(s_Blacklist.begin(), s_Blacklist.end(), titleID) != s_Blacklist.end())
    {
        return true;
    }

    return false;
}

void config::addRemoveTitleToFavorites(const uint64_t &titleID)
{
    auto favoritePosition = std::find(s_Favorites.begin(), s_Favorites.end(), titleID);
    if (favoritePosition == s_Favorites.end())
    {
        s_Favorites.push_back(titleID);
    }
    else
    {
        s_Favorites.erase(favoritePosition);
    }
}

void config::addRemoveTitleToBlacklist(const uint64_t &titleID)
{
    auto blacklistPosition = std::find(s_Blacklist.begin(), s_Blacklist.end(), titleID);
    if (blacklistPosition == s_Blacklist.end())
    {
        s_Blacklist.push_back(titleID);
    }
    else
    {
        s_Blacklist.erase(blacklistPosition);
    }
}