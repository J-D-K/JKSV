#ifndef DATA_H
#define DATA_H

#include <switch.h>

#include <vector>
#include <string>
#include <unordered_map>

#include "gfx.h"

#define curUser users[data::selUser]
#define curData users[data::selUser].titles[data::selData]

#define BLD_MON 11
#define BLD_DAY 01
#define BLD_YEAR 2020

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
    void exportIcons();

    //Draws some stats to the upper left corner
    void dispStats();

    //Class to store title info
    class titledata
    {
        public:
            titledata() = default;
            //Attempts to read title's info
            titledata(const FsSaveDataInfo& inf, NsApplicationControlData *dat);

            //Returns title + title without forbidden chars
            std::string getTitle() const { return title;}
            std::string getTitleSafe() const { return titleSafe; }
            std::string getAuthor() const { return author; }

            //Creates title folder
            void createDir() const;
            //Returns folder path
            std::string getPath() const;

            //returns save_data_id string. only used for helping identify nand files
            std::string getTIDStr() const;
            std::string getSaveIDStr() const;

            uint64_t getID() const { return id; }
            uint64_t getSaveID() const { return saveID; }
            uint16_t getSaveIndex() const { return saveIndex; }
            FsSaveDataType getType() const { return (FsSaveDataType)saveDataType; }
            void setID(const uint64_t& _id){ id = _id; }
            void setIndex(const uint16_t& _ind){ saveIndex = _ind; }
            void setType(FsSaveDataType type) { saveDataType = type; }
            void setFav(bool setFav) { favorite = setFav; }
            bool getFav() const { return favorite; }
            void assignIcons();
            tex *getIcon() const { return icon; }
            tex *getIconFav() const { return favIcon; }
            void setPlayTime(const uint32_t& _p){ playMins = _p; }
            uint32_t getPlayTime() const { return playMins; }
            void setLastTimeStamp(const uint32_t& _ts){ lastTimeStamp = _ts; }
            uint32_t getLastTimeStamp() const { return lastTimeStamp; }
            void setLaunchCount(const uint32_t& _lc) { launchCount = _lc; }
            uint32_t getLaunchCount() const { return launchCount; }

        private:
            tex *icon, *favIcon;
            uint8_t saveDataType;
            std::string title, titleSafe, author;
            uint64_t id, saveID;
            uint16_t saveIndex;
            uint32_t playMins, lastTimeStamp, launchCount;
            bool favorite = false;
    };

    //Class to store user info + titles
    class user
    {
        public:
            user() = default;
            user(const AccountUid& _id, const std::string& _backupName);
            user(const AccountUid& _id, const std::string& _backupName, tex *img);

            //Sets ID
            void setUID(const AccountUid& _id);

            //Assigns icon
            void assignIcon(tex *_icn) { userIcon = _icn; }

            //Returns user ID
            AccountUid getUID() const { return userID; }
            u128 getUID128() const { return uID128; }

            //Returns username
            std::string getUsername() const { return username; }
            std::string getUsernameSafe() const { return userSafe; }

            //Vector for storing save data info for user
            std::vector<titledata> titles;
            void loadPlayTimes();

            void drawIcon(int x, int y) { texDraw(userIcon, frameBuffer, x, y); }
            void drawIconHalf(int x, int y) { texDrawSkip(userIcon, frameBuffer, x, y); }
            void delIcon() { texDestroy(userIcon); }

        private:
            AccountUid userID;
            u128 uID128;
            std::string username, userSafe;
            //User icon
            tex* userIcon;
    };

    //Adds title to blacklist
    void blacklistAdd(data::titledata& t);
    //Adds title to favorite list
    void favoriteTitle(data::titledata& t);

    //User vector
    extern std::vector<user> users;
    extern std::unordered_map<uint64_t, std::pair<tex *, tex *>> icons;

    //Options and info
    //Restores config to default
    void restoreDefaultConfig();
    extern int selUser, selData;
    extern SetLanguage sysLang;
    extern bool incDev, autoBack, ovrClk, holdDel, holdRest, holdOver, forceMount, accSysSave, sysSaveWrite, directFsCmd, skipUser, zip;
    extern uint8_t sortType;
}

#endif // DATA_H
