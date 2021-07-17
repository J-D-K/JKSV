#pragma once
#include <switch.h>

#include <vector>
#include <string>
#include <unordered_map>

#include "gfx.h"

#define curUser users[data::selUser]
#define curData users[data::selUser].titleInfo[data::selData]

#define BLD_MON 7
#define BLD_DAY 15
#define BLD_YEAR 2021

namespace data
{
    extern bool forceMount;

    //Loads user + title info
    void init();
    void exit();
    bool loadUsersTitles(bool clearUsers);
    void loadBlacklist();
    void saveBlackList();
    void loadCfg();
    void saveCfg();
    void loadFav();
    void saveFav();
    void loadDefs();
    void saveDefs();

    //Draws some stats to the upper left corner
    void dispStats();

    //Global stuff for all titles/saves
    typedef struct
    {
        NacpStruct nacp;
        std::string title, safeTitle, author;//Shortcuts sorta.
        SDL_Texture *icon;
        bool fav;
    } titleInfo;

    //Holds stuff specific to user's titles/saves
    typedef struct
    {
        //Makes it easier to grab id
        uint64_t saveID;
        FsSaveDataInfo saveInfo;
        PdmPlayStatistics playStats;
    } userTitleInfo;

    //Class to store user info + titles
    class user
    {
        public:
            user() = default;
            user(const AccountUid& _id, const std::string& _backupName);
            user(const AccountUid& _id, const std::string& _backupName, SDL_Texture *img);

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

    //Adds title to blacklist
    void blacklistAdd(const uint64_t& tid);
    //Adds title to favorite list
    void favoriteTitle(const uint64_t& tid);
    //Adds path definition for title
    void pathDefAdd(const uint64_t& tid, const std::string& newPath);

    bool isFavorite(const uint64_t& tid);

    //User vector
    extern std::vector<user> users;
    //Title data/info map
    extern std::unordered_map<uint64_t, data::titleInfo> titles;

    //Gets pointer to info
    data::titleInfo *getTitleInfoByTID(const uint64_t& tid);

    //More shortcut functions
    std::string getTitleNameByTID(const uint64_t& tid);
    std::string getTitleSafeNameByTID(const uint64_t& tid);
    SDL_Texture *getTitleIconByTID(const uint64_t& tid);
    inline int getTitleIndexInUser(const data::user& u, const uint64_t& sid)
    {
        for(unsigned i = 0; i < u.titleInfo.size(); i++)
        {
            if(u.titleInfo[i].saveID == sid)
                return i;
        }
        return -1;
    }

    //Options and info
    //Restores config to default
    void restoreDefaultConfig();
    extern int selUser, selData;
    extern SetLanguage sysLang;
    extern std::unordered_map<std::string, bool> config;
    extern uint8_t sortType;
}
