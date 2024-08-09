#pragma once
#include <string>
#include <vector>

#include <switch.h>
#include <SDL2/SDL.h>

#include "data/userSaveInfo.hpp"
#include "graphics/graphics.hpp"

namespace data
{
    class user
    {
        public:
            //First is for accounts that actually exist, second is for fake system ones
            user(AccountUid accountID);
            user(AccountUid accountID, const std::string &username, const std::string &pathSafeUserName);

            // Pushes userSaveInfo struct to m_UserSaveInfo
            void addNewUserSaveInfo(uint64_t titleID, const FsSaveDataInfo &saveInfo, const PdmPlayStatistics &playStats);
            //Sorts m_UserSaveInfo according to config
            void sortUserSaveInfo(void);
            // Clears save info vector
            void clearUserSaveInfo(void);
            // These just return user data
            AccountUid getAccountID(void) const;
            u128 getAccountIDU128(void) const;
            std::string getUsername(void) const;
            std::string getPathSafeUsername(void) const;
            graphics::sdlTexture getUserIcon(void) const;
            data::userSaveInfo *getUserSaveInfoAt(int index);
            int getTotalUserSaveInfo(void) const;

        private:
            // Account ID of user
            AccountUid m_AccountID;
            // User name
            std::string m_Username;
            // Safe version of username to use in paths
            std::string m_PathSafeUsername;
            // User's icon
            graphics::sdlTexture m_Icon;
            // Vector of save info for user
            std::vector<data::userSaveInfo> m_UserSaveInfo;

    };
}