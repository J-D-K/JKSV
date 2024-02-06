#pragma once
#include <switch.h>

#include <vector>
#include <string>
#include <unordered_map>

#include "gfx.h"

#define BLD_MON 07
#define BLD_DAY 10
#define BLD_YEAR 2023

namespace data
{
    //Loads user + title info
    void init();
    void exit();
    bool loadUsersTitles(bool clearUsers);
    void sortUserTitles();

    //Draws some stats to the upper left corner
    void dispStats();

    //Global stuff for all titles/saves
    typedef struct
    {
        NacpStruct nacp;
        std::string title, safeTitle, author;//Shortcuts sorta.
        SDL_Texture *icon = NULL;
        bool fav;
    } titleInfo;

    //Holds stuff specific to user's titles/saves
    typedef struct
    {
        //Makes it easier to grab id
        uint64_t tid;
        FsSaveDataInfo saveInfo;
        PdmPlayStatistics playStats;
    } userTitleInfo;

    //Class to store user info + titles
    class user
    {
        public:
            user() = default;
            user(const AccountUid& _id, const std::string& _backupName, const std::string& _safeBackupName);
            user(const AccountUid& _id, const std::string& _backupName, const std::string& _safeBackupName, SDL_Texture *img);

            //Sets ID
            void setUID(const AccountUid& _id);

            //Assigns icon
            void assignIcon(SDL_Texture *_icn) { userIcon = _icn; }

            //Returns user ID
            AccountUid getUID() const { return userID; }
            u128 getUID128() const { return uID128; }

            //Returns username
            std::string getUsername() const { return username; }
            std::string getUsernameSafe() const { return userSafe; }

            SDL_Texture *getUserIcon(){ return userIcon; }
            void delIcon(){ SDL_DestroyTexture(userIcon); }

            std::vector<data::userTitleInfo> titleInfo;
            void addUserTitleInfo(const uint64_t& _tid, const FsSaveDataInfo *_saveInfo, const PdmPlayStatistics *_stats);

        private:
            AccountUid userID;
            u128 uID128;
            std::string username, userSafe;
            //User icon
            SDL_Texture *userIcon;
    };

    //User vector
    extern std::vector<user> users;
    //Title data/info map
    extern std::unordered_map<uint64_t, data::titleInfo> titles;

    //Sets/Retrieves current user/title
    void setUserIndex(unsigned _sUser);
    data::user *getCurrentUser();
    unsigned getCurrentUserIndex();

    void setTitleIndex(unsigned _sTitle);
    data::userTitleInfo *getCurrentUserTitleInfo();
    unsigned getCurrentUserTitleInfoIndex();

    //Gets pointer to info that also has title + nacp
    data::titleInfo *getTitleInfoByTID(const uint64_t& tid);

    //More shortcut functions
    std::string getTitleNameByTID(const uint64_t& tid);
    std::string getTitleSafeNameByTID(const uint64_t& tid);
    SDL_Texture *getTitleIconByTID(const uint64_t& tid);
    int getTitleIndexInUser(const data::user& u, const uint64_t& tid);
    extern SetLanguage sysLang;
    
}
