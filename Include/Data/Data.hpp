#pragma once
#include "Data/AccountUID.hpp"
#include "Data/TitleInfo.hpp"
#include "Data/User.hpp"
#include <vector>

namespace Data
{
    // Loads data from system.
    bool Initialize(void);
    // Gets a vector of pointers to users.
    void GetUsers(std::vector<Data::User *> &VectorOut);
    // Gets the TitleInfo mapped to ApplicationID.
    Data::TitleInfo *GetTitleInfoByID(uint64_t ApplicationID);
} // namespace Data
