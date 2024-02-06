#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace cfg
{
    typedef enum
    {
        ALPHA,
        MOST_PLAYED,
        LAST_PLAYED
    } sortTypes;

    void resetConfig();
    void loadConfig();
    void saveConfig();

    bool isBlacklisted(const uint64_t& tid);
    void addTitleToBlacklist(void *a);
    void removeTitleFromBlacklist(const uint64_t& tid);

    bool isFavorite(const uint64_t& tid);
    void addTitleToFavorites(const uint64_t& tid);

    bool isDefined(const uint64_t& tid);
    void pathDefAdd(const uint64_t& tid, const std::string& newPath);
    std::string getPathDefinition(const uint64_t& tid);

    void addPathToFilter(const uint64_t& tid, const std::string& _p);

    extern std::unordered_map<std::string, bool> config;
    extern std::vector<uint64_t> blacklist;
    extern std::vector<uint64_t> favorites;
    extern uint8_t sortType;
    extern std::string driveClientID, driveClientSecret, driveRefreshToken;
    extern std::string webdavOrigin, webdavBasePath, webdavUser, webdavPassword;
}
