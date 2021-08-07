#pragma once

#include <string>
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

    bool isFavorite(const uint64_t& tid);
    void addTitleToFavorites(const uint64_t& tid);

    bool isDefined(const uint64_t& tid);
    void pathDefAdd(const uint64_t& tid, const std::string& newPath);
    std::string getPathDefinition(const uint64_t& tid);

    void addPathToFilter(const uint64_t& tid, const std::string& _p);

    extern std::unordered_map<std::string, bool> config;
    extern uint8_t sortType;
}
