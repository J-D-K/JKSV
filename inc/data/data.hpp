#pragma once
#include "data/user.hpp"
#include "data/titleInfo.hpp"

namespace data
{
    // Loads users and titles from switch records
    bool init(void);
    // Loads save data for users
    void loadUserSaveInfo(void);
    // Sorts titles according to config
    void sortUserSaveInfo(void);
    // Returns total users
    int getTotalUsers(void);
    // Gets user by AccountUid. Returns false if not possible
    data::user *getUserByAccountID(const u128 &accountID);
    // Gets user instead by position in vector
    data::user *getUserAtPosition(const int &position);
    // Gets pointer to titleInfo struct with titleID
    data::titleInfo *getTitleInfoByTitleID(const uint64_t &titleID);
}