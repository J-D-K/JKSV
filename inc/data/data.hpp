#pragma once
#include <memory>
#include "data/user.hpp"
#include "data/titleInfo.hpp"
#include "system/system.hpp"

namespace data
{
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
    // Gets pointer to titleInfo struct with titleID
    data::titleInfo *getTitleInfoByTitleID(uint64_t titleID);
}