#pragma once
#include <memory>
#include <unordered_map>

#include "data/user.hpp"
#include "data/titleInfo.hpp"

#include "system/system.hpp"

namespace data
{
    // Makes stuff easier to type.
    using titleMap = std::unordered_map<uint64_t, data::titleInfo>;
    
    // Loads users and titles from switch records. This is the initial loading routine and can run as a task.
    bool init(void);
    // Loads save data for users
    void loadUserSaveInfo(void);
    // Sorts titles according to config
    void sortUserSaveInfo(void);
    // Returns total users
    int getTotalUsers(void);
    // Gets user by AccountUid. Returns false if not possible
    data::user *getUserByAccountID(u128 accountID);
    // Gets user instead by position in vector
    data::user *getUserAtPosition(int position);
    // Returns reference to title map. Used exclusively for save creation menu
    data::titleMap &getTitleMap(void);
    // Returns total titles in map
    int getTotalTitleCount(void);
    // Gets pointer to titleInfo struct with titleID
    data::titleInfo *getTitleInfoByTitleID(uint64_t titleID);
}