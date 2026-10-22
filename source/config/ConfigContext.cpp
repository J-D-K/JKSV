#include "config/ConfigContext.hpp"

#include "config/keys.hpp"
#include "error.hpp"
#include "json.hpp"
#include "logging/logger.hpp"
#include "stringutil.hpp"

#include <algorithm>
#include <cstring>

namespace
{
    constexpr std::string_view PATH_DEFAULT_WORK_DIR = "sdmc:/JKSV";
    constexpr std::string_view PATH_CONFIG_FOLDER    = "sdmc:/config/JKSV";
    constexpr std::string_view PATH_CONFIG_FILE      = "sdmc:/config/JKSV/JKSV.json";
    constexpr std::string_view PATH_PATHS_FILE       = "sdmc:/config/JKSV/Paths.json";

    constexpr double DEFAULT_SCALING = 2.5f;

    constexpr const char *APP_ID_HEX_FORMAT = "%016llX";
}

//                      ---- Construction ----

config::ConfigContext::ConfigContext()
{
    // This will default value everything.
    ConfigContext::initialize();
}

//                      ---- Public functions ----

void config::ConfigContext::create_directory()
{
    const fslib::Path configDir{PATH_CONFIG_FOLDER};
    const bool exists = fslib::directory_exists(configDir);
    if (exists) { return; }

    error::fslib(fslib::create_directories_recursively(configDir));
}

void config::ConfigContext::initialize()
{
    m_workingDirectory                                 = PATH_DEFAULT_WORK_DIR;
    m_configMap[config::keys::INCLUDE_DEVICE_SAVES]    = 0;
    m_configMap[config::keys::AUTO_BACKUP_ON_RESTORE]  = 1;
    m_configMap[config::keys::AUTO_NAME_BACKUPS]       = 0;
    m_configMap[config::keys::AUTO_UPLOAD]             = 0;
    m_configMap[config::keys::KEEP_LOCAL_BACKUPS]      = 0;
    m_configMap[config::keys::USE_TITLE_IDS]           = 0;
    m_configMap[config::keys::ENGLISH_SAFE_TITLES]     = 0;
    m_configMap[config::keys::HOLD_FOR_DELETION]       = 1;
    m_configMap[config::keys::HOLD_FOR_RESTORATION]    = 1;
    m_configMap[config::keys::HOLD_FOR_OVERWRITE]      = 1;
    m_configMap[config::keys::ONLY_LIST_MOUNTABLE]     = 1;
    m_configMap[config::keys::LIST_ACCOUNT_SYS_SAVES]  = 0;
    m_configMap[config::keys::ALLOW_WRITING_TO_SYSTEM] = 0;
    m_configMap[config::keys::EXPORT_TO_ZIP]           = 1;
    m_configMap[config::keys::ZIP_COMPRESSION_LEVEL]   = 6;
    m_configMap[config::keys::TITLE_SORT_TYPE]         = 0;
    m_configMap[config::keys::JKSM_TEXT_MODE]          = 0;
    m_configMap[config::keys::FORCE_ENGLISH]           = 0;
    m_configMap[config::keys::SHOW_DEVICE_USER]        = 1;
    m_configMap[config::keys::SHOW_BCAT_USER]          = 0;
    m_configMap[config::keys::SHOW_CACHE_USER]         = 0;
    m_configMap[config::keys::SHOW_SYSTEM_USER]        = 0;
    m_configMap[config::keys::ENABLE_TRASH_BIN]        = 0;
    m_animationScaling                                 = DEFAULT_SCALING;
    m_peerLastIp.clear();
}

bool config::ConfigContext::load()
{
    ConfigContext::load_custom_paths();
    return ConfigContext::load_config_file();
}

void config::ConfigContext::save() { ConfigContext::save_config_file(); }

uint8_t config::ConfigContext::get_by_key(std::string_view key) const noexcept
{
    const auto findKey = m_configMap.find(key);
    if (findKey == m_configMap.end()) { return 0; }
    return findKey->second;
}

void config::ConfigContext::toggle_by_key(std::string_view key) noexcept
{
    auto findKey = m_configMap.find(key);
    if (findKey == m_configMap.end()) { return; }

    const uint8_t value = findKey->second;
    findKey->second     = value ? 0 : 1;
}

void config::ConfigContext::set_by_key(std::string_view key, uint8_t value) noexcept
{
    auto findKey = m_configMap.find(key);
    if (findKey == m_configMap.end()) { return; }

    findKey->second = value;
}

fslib::Path config::ConfigContext::get_working_directory() const noexcept { return m_workingDirectory; }

bool config::ConfigContext::set_working_directory(const fslib::Path &workDir) noexcept
{
    if (!workDir.is_valid()) { return false; }

    m_workingDirectory = workDir;
    return true;
}

double config::ConfigContext::get_animation_scaling() const noexcept { return m_animationScaling; }

void config::ConfigContext::set_animation_scaling(double scaling) noexcept { m_animationScaling = scaling; }

void config::ConfigContext::add_favorite(uint64_t applicationID)
{
    const auto findID = m_favorites.find(applicationID);
    if (findID != m_favorites.end()) { return; }

    m_favorites.insert(applicationID);
}

void config::ConfigContext::remove_favorite(uint64_t applicationID) noexcept
{
    const auto findID = m_favorites.find(applicationID);
    if (findID == m_favorites.end()) { return; }

    m_favorites.erase(findID);
}

bool config::ConfigContext::is_favorite(uint64_t applicationID) const noexcept
{
    return m_favorites.find(applicationID) != m_favorites.end();
}

void config::ConfigContext::add_to_blacklist(uint64_t applicationID)
{
    auto findID = m_blacklist.find(applicationID);
    if (findID != m_blacklist.end()) { return; }

    m_blacklist.insert(applicationID);
}

void config::ConfigContext::remove_from_blacklist(uint64_t applicationID) noexcept
{
    const auto findID = m_blacklist.find(applicationID);
    if (findID == m_blacklist.end()) { return; }

    m_blacklist.erase(findID);
}

void config::ConfigContext::get_blacklist(std::vector<uint64_t> &listOut)
{
    listOut.assign(m_blacklist.begin(), m_blacklist.end());
}

bool config::ConfigContext::is_blacklisted(uint64_t applicationID) const noexcept
{
    return m_blacklist.find(applicationID) != m_blacklist.end();
}

bool config::ConfigContext::blacklist_empty() const noexcept { return m_blacklist.empty(); }

bool config::ConfigContext::has_custom_path(uint64_t applicationID) const noexcept
{
    return m_paths.find(applicationID) != m_paths.end();
}

void config::ConfigContext::add_custom_path(uint64_t applicationID, std::string_view newPath)
{
    m_paths[applicationID] = newPath;
    ConfigContext::save_custom_paths();
}

void config::ConfigContext::get_custom_path(uint64_t applicationID, char *buffer, size_t bufferSize)
{
    const auto findPath = m_paths.find(applicationID);
    if (findPath == m_paths.end()) { return; }

    const std::string &path = findPath->second;
    if (path.length() >= bufferSize) { return; }

    std::memset(buffer, 0x00, bufferSize);
    std::memcpy(buffer, path.c_str(), path.length());
}

const std::string &config::ConfigContext::get_peer_last_ip() const noexcept { return m_peerLastIp; }

void config::ConfigContext::set_peer_last_ip(std::string_view ip)
{
    m_peerLastIp = ip;
    ConfigContext::save_config_file();
}

//                      ---- Private functions ----

bool config::ConfigContext::load_config_file()
{
    const fslib::Path configPath{PATH_CONFIG_FILE};
    const bool exists = fslib::file_exists(configPath);
    if (!exists) { return false; }

    json::Object configJSON = json::new_object(json_object_from_file, PATH_CONFIG_FILE.data());
    if (!configJSON) { return false; }

    json_object_iterator configIter = json::iter_begin(configJSON);
    json_object_iterator configEnd  = json::iter_end(configJSON);
    while (!json_object_iter_equal(&configIter, &configEnd))
    {
        const char *key    = json_object_iter_peek_name(&configIter);
        json_object *value = json_object_iter_peek_value(&configIter);

        // These keys require special handling.
        const bool workDir    = key == config::keys::WORKING_DIRECTORY;
        const bool scaling    = key == config::keys::UI_ANIMATION_SCALE;
        const bool favorites  = key == config::keys::FAVORITES;
        const bool blacklist  = key == config::keys::BLACKLIST;
        const bool peerLastIp = key == config::keys::PEER_LAST_IP;

        if (workDir) { m_workingDirectory = json_object_get_string(value); }
        else if (scaling) { m_animationScaling = json_object_get_double(value); }
        else if (favorites) { ConfigContext::read_array_to_set(m_favorites, value); }
        else if (blacklist) { ConfigContext::read_array_to_set(m_blacklist, value); }
        else if (peerLastIp) { m_peerLastIp = json_object_get_string(value); }
        else { m_configMap[key] = json_object_get_uint64(value); }

        json_object_iter_next(&configIter);
    }

    return true;
}

void config::ConfigContext::save_config_file()
{
    json::Object configJSON = json::new_object(json_object_new_object);
    if (!configJSON) { return; }

    const std::string workDirString = m_workingDirectory.string();
    json_object *workDir            = json_object_new_string(workDirString.c_str());
    json::add_object(configJSON, config::keys::WORKING_DIRECTORY, workDir);

    for (const auto &[key, value] : m_configMap)
    {
        json_object *jsonValue = json_object_new_uint64(value);
        json::add_object(configJSON, key, jsonValue);
    }

    json_object *scaling = json_object_new_double(m_animationScaling);
    json::add_object(configJSON, config::keys::UI_ANIMATION_SCALE, scaling);

    json_object *peerLastIp = json_object_new_string(m_peerLastIp.c_str());
    json::add_object(configJSON, config::keys::PEER_LAST_IP, peerLastIp);

    json_object *favoritesArray = json_object_new_array();
    for (const uint64_t applicationID : m_favorites)
    {
        const std::string appIDHex = stringutil::get_formatted_string(APP_ID_HEX_FORMAT, applicationID);
        json_object *jsonFavorite  = json_object_new_string(appIDHex.c_str());

        json_object_array_add(favoritesArray, jsonFavorite);
    }
    json::add_object(configJSON, config::keys::FAVORITES, favoritesArray);

    json_object *blacklistArray = json_object_new_array();
    for (const uint64_t applicationID : m_blacklist)
    {
        const std::string appIDHex = stringutil::get_formatted_string(APP_ID_HEX_FORMAT, applicationID);
        json_object *jsonBlacklist = json_object_new_string(appIDHex.c_str());

        json_object_array_add(blacklistArray, jsonBlacklist);
    }
    json::add_object(configJSON, config::keys::BLACKLIST, blacklistArray);

    const char *jsonString   = json::get_string(configJSON);
    const int64_t jsonLength = json::length(configJSON);
    fslib::File configFile{PATH_CONFIG_FILE, FsOpenMode_Create | FsOpenMode_Write, jsonLength};
    if (error::fslib(configFile.is_open())) { return; }
    configFile << jsonString;
}

void config::ConfigContext::read_array_to_set(std::unordered_set<uint64_t> &set, json_object *array)
{
    const size_t arrayLength = json_object_array_length(array);
    for (size_t i = 0; i < arrayLength; i++)
    {
        json_object *element         = json_object_array_get_idx(array, i);
        const char *idHex            = json_object_get_string(element);
        const uint64_t applicationID = std::strtoull(idHex, nullptr, 16);

        set.insert(applicationID);
    }
}

void config::ConfigContext::load_custom_paths()
{
    json::Object pathsJSON = json::new_object(json_object_from_file, PATH_PATHS_FILE.data());
    if (!pathsJSON) { return; }

    json_object_iterator pathsIter = json::iter_begin(pathsJSON);
    json_object_iterator pathsEnd  = json::iter_end(pathsJSON);
    while (!json_object_iter_equal(&pathsIter, &pathsEnd))
    {
        const char *appIDString = json_object_iter_peek_name(&pathsIter);
        json_object *pathObject = json_object_iter_peek_value(&pathsIter);

        const uint64_t applicationID = std::strtoull(appIDString, nullptr, 16);
        const char *path             = json_object_get_string(pathObject);
        m_paths.try_emplace(applicationID, path);

        json_object_iter_next(&pathsIter);
    }
}

void config::ConfigContext::save_custom_paths()
{
    json::Object pathsJSON = json::new_object(json_object_new_object);
    if (!pathsJSON) { return; }

    for (const auto &[applicationID, path] : m_paths)
    {
        const std::string key   = stringutil::get_formatted_string(APP_ID_HEX_FORMAT, applicationID);
        json_object *pathObject = json_object_new_string(path.c_str());

        json::add_object(pathsJSON, key, pathObject);
    }

    const char *jsonString   = json::get_string(pathsJSON);
    const int64_t jsonLength = json::length(pathsJSON);
    fslib::File pathsFile{PATH_PATHS_FILE, FsOpenMode_Create | FsOpenMode_Write, jsonLength};
    if (error::fslib(pathsFile.is_open())) { return; }
    pathsFile << jsonString;
}
