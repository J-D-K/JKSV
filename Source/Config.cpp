#include "Config.hpp"
#include "JSON.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    // Config path(s)
    const char *CONFIG_FOLDER = "sdmc:/config/JKSV";
    const char *CONFIG_PATH = "sdmc:/config/JKSV/JKSV.json";
    // Map of config values
    std::unordered_map<std::string, uint8_t> s_ConfigMap;
    // Working directory
    FsLib::Path s_WorkingDirectory;
    // UI animation scaling.
    double s_UIAnimationScaling;
    // Vector of favorite title ids
    std::vector<uint64_t> s_Favorites;
    // Vector of titles to ignore.
    std::vector<uint64_t> s_Blacklist;
} // namespace

static void ReadArrayToVector(std::vector<uint64_t> &Vector, json_object *Array)
{
    // Just in case. Shouldn't happen though.
    Vector.clear();

    size_t ArrayLength = json_object_array_length(Array);
    for (size_t i = 0; i < ArrayLength; i++)
    {
        json_object *ArrayEntry = json_object_array_get_idx(Array, i);
        if (!ArrayEntry)
        {
            continue;
        }
        Vector.push_back(json_object_get_uint64(ArrayEntry));
    }
}

void Config::Initialize(void)
{
    if (!FsLib::DirectoryExists(CONFIG_FOLDER) && !FsLib::CreateDirectoriesRecursively(CONFIG_FOLDER))
    {
        Logger::Log("Error creating config folder: %s.", FsLib::GetErrorString());
        Config::ResetToDefault();
        return;
    }

    JSON::Object ConfigJSON = JSON::NewObject(json_object_from_file, CONFIG_PATH);
    if (!ConfigJSON)
    {
        Logger::Log("Error opening config for reading: %s", FsLib::GetErrorString());
        Config::ResetToDefault();
        return;
    }

    json_object_iterator ConfigIterator = json_object_iter_begin(ConfigJSON.get());
    json_object_iterator ConfigEnd = json_object_iter_end(ConfigJSON.get());
    while (!json_object_iter_equal(&ConfigIterator, &ConfigEnd))
    {
        const char *KeyName = json_object_iter_peek_name(&ConfigIterator);
        json_object *ConfigValue = json_object_iter_peek_value(&ConfigIterator);

        // These are exemptions.
        if (std::strcmp(KeyName, Config::Keys::WorkingDirectory.data()) == 0)
        {
            s_WorkingDirectory = json_object_get_string(ConfigValue);
        }
        else if (std::strcmp(KeyName, Config::Keys::UIAnimationScaling.data()) == 0)
        {
            s_UIAnimationScaling = json_object_get_double(ConfigValue);
        }
        else if (std::strcmp(KeyName, Config::Keys::Favorites.data()) == 0)
        {
            ReadArrayToVector(s_Favorites, ConfigValue);
        }
        else if (std::strcmp(KeyName, Config::Keys::BlackList.data()) == 0)
        {
            ReadArrayToVector(s_Blacklist, ConfigValue);
        }
        else
        {
            s_ConfigMap[KeyName] = json_object_get_uint64(ConfigValue);
        }
        json_object_iter_next(&ConfigIterator);
    }
}

void Config::ResetToDefault(void)
{
    s_WorkingDirectory = "sdmc:/JKSV";
    s_ConfigMap[Config::Keys::IncludeDeviceSaves.data()] = 1;
    s_ConfigMap[Config::Keys::AutoBackupOnRestore.data()] = 1;
    s_ConfigMap[Config::Keys::HoldForDeletion.data()] = 1;
    s_ConfigMap[Config::Keys::HoldForRestoration.data()] = 1;
    s_ConfigMap[Config::Keys::HoldForOverwrite.data()] = 1;
    s_ConfigMap[Config::Keys::OnlyListMountable.data()] = 1;
    s_ConfigMap[Config::Keys::ListAccountSystemSaves.data()] = 0;
    s_ConfigMap[Config::Keys::AllowSystemSaveWriting.data()] = 0;
    s_ConfigMap[Config::Keys::ExportToZip.data()] = 1;
    s_ConfigMap[Config::Keys::ForceEnglish.data()] = 0;
    s_ConfigMap[Config::Keys::EnableTrashBin.data()] = 1;
    s_ConfigMap[Config::Keys::AutoNameBackups.data()] = 0;
    s_ConfigMap[Config::Keys::TitleSortType.data()] = 0;
    s_ConfigMap[Config::Keys::AutoUpload.data()] = 0;
    s_UIAnimationScaling = 2.5f;
}

void Config::Save(void)
{
    JSON::Object ConfigJSON = JSON::NewObject(json_object_new_object);

    // Add working directory first.
    json_object *WorkingDirectory = json_object_new_string(s_WorkingDirectory.CString());
    json_object_object_add(ConfigJSON.get(), Config::Keys::WorkingDirectory.data(), WorkingDirectory);

    // Loop through map and add it.
    for (auto &[Key, Value] : s_ConfigMap)
    {
        json_object *JsonValue = json_object_new_uint64(Value);
        json_object_object_add(ConfigJSON.get(), Key.c_str(), JsonValue);
    }

    // Add UI scaling.
    json_object *Scaling = json_object_new_double(s_UIAnimationScaling);
    json_object_object_add(ConfigJSON.get(), Config::Keys::UIAnimationScaling.data(), Scaling);

    // Favorites
    json_object *FavoritesArray = json_object_new_array();
    for (uint64_t &TitleID : s_Favorites)
    {
        json_object *NewFavorite = json_object_new_uint64(TitleID);
        json_object_array_add(FavoritesArray, NewFavorite);
    }
    json_object_object_add(ConfigJSON.get(), Config::Keys::Favorites.data(), FavoritesArray);

    // Same but blacklist
    json_object *BlacklistArray = json_object_new_array();
    for (uint64_t &TitleID : s_Blacklist)
    {
        json_object *NewBlacklist = json_object_new_uint64(TitleID);
        json_object_array_add(BlacklistArray, NewBlacklist);
    }
    json_object_object_add(ConfigJSON.get(), Config::Keys::BlackList.data(), BlacklistArray);

    // Write config file
    FsLib::File ConfigFile(CONFIG_PATH, FsOpenMode_Create | FsOpenMode_Write, std::strlen(json_object_get_string(ConfigJSON.get())));
    ConfigFile << json_object_get_string(ConfigJSON.get());
}

uint8_t Config::GetByKey(std::string_view Key)
{
    if (s_ConfigMap.find(Key.data()) == s_ConfigMap.end())
    {
        return 0;
    }
    return s_ConfigMap.at(Key.data());
}

FsLib::Path Config::GetWorkingDirectory(void)
{
    return s_WorkingDirectory;
}

double Config::GetAnimationScaling(void)
{
    return s_UIAnimationScaling;
}

void Config::AddRemoveFavorite(uint64_t TitleID)
{
    auto FindTitle = std::find(s_Favorites.begin(), s_Favorites.end(), TitleID);
    if (FindTitle == s_Favorites.end())
    {
        s_Favorites.push_back(TitleID);
    }
    else
    {
        s_Favorites.erase(FindTitle);
    }
}

bool Config::IsFavorite(uint64_t TitleID)
{
    if (std::find(s_Favorites.begin(), s_Favorites.end(), TitleID) == s_Favorites.end())
    {
        return false;
    }
    return true;
}

void Config::AddRemoveBlacklist(uint64_t TitleID)
{
    auto FindTitle = std::find(s_Blacklist.begin(), s_Blacklist.end(), TitleID);
    if (FindTitle == s_Blacklist.end())
    {
        s_Blacklist.push_back(TitleID);
    }
    else
    {
        s_Blacklist.erase(FindTitle);
    }
}

bool Config::IsBlacklisted(uint64_t TitleID)
{
    if (std::find(s_Blacklist.begin(), s_Blacklist.end(), TitleID) == s_Blacklist.end())
    {
        return false;
    }
    return true;
}
