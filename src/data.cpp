#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <switch.h>

#include "data.h"
#include "file.h"
#include "util.h"

//FsSaveDataSpaceId_All doesn't work for SD
static const unsigned saveOrder [] = { 0, 1, 2, 3, 4, 100, 101 };

int data::selUser = 0, data::selData = 0;

//User vector
std::vector<data::user> data::users;

//System language
SetLanguage data::sysLang;

//Options
bool data::incDev = false, data::autoBack = true, data::ovrClk = false, data::holdDel = true, data::holdRest = true, data::holdOver = true;
bool data::forceMount = true, data::accSysSave = false, data::sysSaveWrite = false, data::directFsCmd = false, data::skipUser = false, data::zip = false, data::langOverride = false;

uint8_t data::sortType = 0;

//For other save types
static bool sysBCATPushed = false, tempPushed = false;

static std::vector<uint64_t> blacklist;
static std::vector<uint64_t> favorites;
static std::unordered_map<uint64_t, std::string> pathDefs;
std::unordered_map<uint64_t, data::titleInfo> data::titles;

static SDL_Texture *iconMask;

//Sorts titles by sortType
static struct
{
    bool operator()(const data::userTitleInfo& a, const data::userTitleInfo& b)
    {
        //Favorites override EVERYTHING
        if(data::isFavorite(a.saveID) != data::isFavorite(b.saveID)) return data::isFavorite(a.saveID);

        switch(data::sortType)
        {
            case 0://Alpha
                {
                    std::string titleA = data::getTitleNameByTID(a.saveID);
                    std::string titleB = data::getTitleNameByTID(b.saveID);
                    uint32_t tmpA, tmpB;
                    for(unsigned i = 0; i < titleA.length(); )
                    {
                        ssize_t uCnt = decode_utf8(&tmpA, (const uint8_t *)&titleA.data()[i]);
                        decode_utf8(&tmpB, (const uint8_t *)&titleB.data()[i]);
                        tmpA = tolower(tmpA), tmpB = tolower(tmpB);
                        if(tmpA != tmpB)
                            return tmpA < tmpB;

                        i += uCnt;
                    }
                }
                break;

            case 1://Most played
                return a.playStats.playtimeMinutes > b.playStats.playtimeMinutes;
                break;

            case 2://Last Played
                return a.playStats.last_timestampUser > b.playStats.last_timestampUser;
                break;
        }
        return false;
    }
} sortTitles;

//Returns -1 for new
static int getUserIndex(const AccountUid& id)
{
    u128 nId = util::accountUIDToU128(id);
    for(unsigned i = 0; i < data::users.size(); i++)
        if(data::users[i].getUID128() == nId) return i;

    return -1;
}

static bool blacklisted(const uint64_t& tid)
{
    for(uint64_t& bid : blacklist)
        if(tid == bid) return true;

    return false;
}

bool data::isFavorite(const uint64_t& tid)
{
    for(uint64_t& fid : favorites)
        if(tid == fid) return true;

    return false;
}

static bool isDefined(const uint64_t& id)
{
    for(auto& def : pathDefs)
        if(def.first == id) return true;

    return false;
}

static inline bool accountSystemSaveCheck(const FsSaveDataInfo& _inf)
{
    if(_inf.save_data_type == FsSaveDataType_System && util::accountUIDToU128(_inf.uid) != 0 && !data::accSysSave)
        return false;

    return true;
}

//Minimal init/test to avoid loading and creating things I don't need
static bool testMount(const FsSaveDataInfo& _inf)
{
    bool ret = false;
    if(!data::forceMount)
        return true;

    if((ret = fs::mountSave(_inf)))
        fs::unmountSave();

    return ret;
}

static inline void addTitleToList(const uint64_t& tid)
{
    uint64_t outSize = 0;
    NsApplicationControlData *ctrlData = new NsApplicationControlData;
    NacpLanguageEntry *ent;
    Result ctrlRes = nsGetApplicationControlData(NsApplicationControlSource_Storage, tid, ctrlData, sizeof(NsApplicationControlData), &outSize);
    Result nacpRes = nacpGetLanguageEntry(&ctrlData->nacp, &ent);
    size_t iconSize = outSize - sizeof(ctrlData->nacp);

    if(R_SUCCEEDED(ctrlRes) && !(outSize < sizeof(ctrlData->nacp)) && R_SUCCEEDED(nacpRes) && iconSize > 0)
    {
        //Copy nacp
        memcpy(&data::titles[tid].nacp, &ctrlData->nacp, sizeof(NacpStruct));

        //Setup 'shortcuts' to strings
        NacpLanguageEntry *ent;
        nacpGetLanguageEntry(&data::titles[tid].nacp, &ent);
        data::titles[tid].title = ent->name;
        data::titles[tid].author = ent->author;
        if(isDefined(tid))
            data::titles[tid].safeTitle = pathDefs[tid];
        else if((data::titles[tid].safeTitle = util::safeString(ent->name)) == "")
            data::titles[tid].safeTitle = util::getIDStr(tid);

        if(data::isFavorite(tid))
            data::titles[tid].fav = true;

        data::titles[tid].icon = gfx::loadJPEGMem(ctrlData->icon, iconSize);
    }
    else
    {
        memset(&data::titles[tid].nacp, 0, sizeof(NacpStruct));
        data::titles[tid].title = util::getIDStr(tid);
        data::titles[tid].author = "Someone?";
        if(isDefined(tid))
            data::titles[tid].safeTitle = pathDefs[tid];
        else
            data::titles[tid].safeTitle = util::getIDStr(tid);

        data::titles[tid].icon = util::createIconGeneric(util::getIDStrLower(tid).c_str(), 32);
    }

    delete ctrlData;
}

static inline bool titleIsLoaded(const uint64_t& tid)
{
    auto findTid = data::titles.find(tid);

    return findTid == data::titles.end() ? false : true;
}

static void loadUserAccounts()
{
    s32 total = 0;
    AccountUid *uids = new AccountUid[8];
    if(R_SUCCEEDED(accountListAllUsers(uids, 8, &total)))
    {
        for(int i = 0; i < total; i++)
            data::users.emplace_back(uids[i], "");
    }
    delete[] uids;
}

//This can load titles installed without having save data
static void loadTitlesFromRecords()
{
    NsApplicationRecord nsRecord;
    int32_t entryCount = 0, recordOffset = 0;
    while(R_SUCCEEDED(nsListApplicationRecord(&nsRecord, 1, recordOffset++, &entryCount)) && entryCount > 0)
    {
        if(!titleIsLoaded(nsRecord.application_id))
            addTitleToList(nsRecord.application_id);
    }
}

bool data::loadUsersTitles(bool clearUsers)
{
    static unsigned systemUserCount = 3;
    FsSaveDataInfoReader it;
    FsSaveDataInfo info;
    s64 total = 0;

    loadTitlesFromRecords();

    //Clear titles
    for(data::user& u : data::users)
        u.titleInfo.clear();
    if(clearUsers)
    {
        systemUserCount = 4;
        for(data::user& u : data::users)
            u.delIcon();
        data::users.clear();

        loadUserAccounts();
        sysBCATPushed = false;
        tempPushed = false;
        users.emplace_back(util::u128ToAccountUID(3), "Device");
        users.emplace_back(util::u128ToAccountUID(2), "BCAT");
        users.emplace_back(util::u128ToAccountUID(5), "Cache");
        users.emplace_back(util::u128ToAccountUID(0), "System");
    }

    for(unsigned i = 0; i < 7; i++)
    {
        if(R_FAILED(fsOpenSaveDataInfoReader(&it, (FsSaveDataSpaceId)saveOrder[i])))
            continue;

        while(R_SUCCEEDED(fsSaveDataInfoReaderRead(&it, &info, 1, &total)) && total != 0)
        {
            uint64_t saveID = 0;
            if(info.save_data_type == FsSaveDataType_System || info.save_data_type == FsSaveDataType_SystemBcat)
                saveID = info.system_save_data_id;
            else
                saveID = info.application_id;

            //Don't bother with this stuff
            if(blacklisted(saveID) || !accountSystemSaveCheck(info) || !testMount(info))
                continue;

            switch(info.save_data_type)
            {
                case FsSaveDataType_Bcat:
                    info.uid = util::u128ToAccountUID(2);
                    break;

                case FsSaveDataType_Device:
                    info.uid = util::u128ToAccountUID(3);
                    break;

                case FsSaveDataType_SystemBcat:
                    info.uid = util::u128ToAccountUID(4);
                    if(!sysBCATPushed)
                    {
                        ++systemUserCount;
                        sysBCATPushed = true;
                        users.emplace_back(util::u128ToAccountUID(4), "Sys. BCAT");
                    }
                    break;

                case FsSaveDataType_Cache:
                    info.uid = util::u128ToAccountUID(5);
                    break;

                case FsSaveDataType_Temporary:
                    info.uid = util::u128ToAccountUID(6);
                    if(!tempPushed)
                    {
                        ++systemUserCount;
                        tempPushed = true;
                        users.emplace_back(util::u128ToAccountUID(6), "Temp");
                    }
                    break;
            }

            if(!titleIsLoaded(saveID))
                addTitleToList(saveID);

            int u = getUserIndex(info.uid);
            if(u == -1)
            {
                users.emplace(data::users.end() - systemUserCount, info.uid, "");
                u = getUserIndex(info.uid);
            }

            PdmPlayStatistics playStats;
            if(info.save_data_type == FsSaveDataType_Account || info.save_data_type == FsSaveDataType_Device)
                pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(info.application_id, info.uid, false, &playStats);
            else
                memset(&playStats, 0, sizeof(PdmPlayStatistics));

            users[u].addUserTitleInfo(saveID, &info, &playStats);
        }
        fsSaveDataInfoReaderClose(&it);
    }

    /*if(data::incDev)
    {
        //Get reference to device save user
        data::user& dev = data::users[data::users.size() - 4];
        for(unsigned i = 0; i < users.size() - 4; i++)
        {
            //Not needed but makes this easier to read
            data::user& u = data::users[i];
            u.titles.insert(u.titles.end(), dev.titles.begin(), dev.titles.end());
        }
    }*/

    for(data::user& u : data::users)
        std::sort(u.titleInfo.begin(), u.titleInfo.end(), sortTitles);

    return true;
}

void data::init()
{
    loadBlacklist();
    loadFav();
    loadCfg();
    loadDefs();

    if(data::ovrClk)
        util::setCPU(1224000000);

    uint64_t lang;
    setGetSystemLanguage(&lang);
    setMakeLanguage(lang, &sysLang);

    data::loadUsersTitles(true);
}

void data::exit()
{
    for(data::user& u : data::users) u.delIcon();
    for(auto& tinfo : titles)
        SDL_DestroyTexture(tinfo.second.icon);

    SDL_DestroyTexture(iconMask);

    saveFav();
    saveCfg();
    saveBlackList();
    util::setCPU(1020000000);
}

data::titleInfo *data::getTitleInfoByTID(const uint64_t& tid)
{
    return &titles[tid];
}

std::string data::getTitleNameByTID(const uint64_t& tid)
{
    return titles[tid].title;
}

std::string data::getTitleSafeNameByTID(const uint64_t& tid)
{
    return titles[tid].safeTitle;
}

SDL_Texture *data::getTitleIconByTID(const uint64_t& tid)
{
    return titles[tid].icon;
}

data::user::user(const AccountUid& _id, const std::string& _backupName)
{
    userID = _id;
    uID128 = util::accountUIDToU128(_id);

    AccountProfile prof;
    AccountProfileBase base;

    if(R_SUCCEEDED(accountGetProfile(&prof, userID)) && R_SUCCEEDED(accountProfileGet(&prof, NULL, &base)))
    {
        username = base.nickname;
        userSafe = util::safeString(username);
        if(userSafe.empty())
        {
            char tmp[32];
            sprintf(tmp, "Acc%08X", (uint32_t)uID128);
            userSafe = tmp;
        }

        uint32_t jpgSize = 0;
        accountProfileGetImageSize(&prof, &jpgSize);
        uint8_t *jpegData = new uint8_t[jpgSize];
        accountProfileLoadImage(&prof, jpegData, jpgSize, &jpgSize);
        userIcon = gfx::loadJPEGMem(jpegData, jpgSize);
        delete[] jpegData;

        accountProfileClose(&prof);
    }
    else
    {
        username = _backupName.empty() ? util::getIDStr((uint64_t)uID128) : _backupName;
        userSafe = _backupName.empty() ? util::getIDStr((uint64_t)uID128) : _backupName;
        userIcon = util::createIconGeneric(_backupName.c_str(), 40);
    }
    titles.reserve(64);
}

data::user::user(const AccountUid& _id, const std::string& _backupName, SDL_Texture *img) : user(_id, _backupName)
{
    delIcon();
    userIcon = img;
    titles.reserve(64);
}

void data::user::setUID(const AccountUid& _id)
{
    userID = _id;
    uID128 = util::accountUIDToU128(_id);
}

void data::user::addUserTitleInfo(const uint64_t& tid, const FsSaveDataInfo *_saveInfo, const PdmPlayStatistics *_stats)
{
    data::userTitleInfo newInfo;
    newInfo.saveID = tid;
    memcpy(&newInfo.saveInfo, _saveInfo, sizeof(FsSaveDataInfo));
    memcpy(&newInfo.playStats, _stats, sizeof(PdmPlayStatistics));
    titleInfo.emplace_back(newInfo);
}

void data::loadBlacklist()
{
    fs::dataFile blk(fs::getWorkDir() + "blacklist.txt");
    if(blk.isOpen())
    {
        while(blk.readNextLine(false))
            blacklist.push_back(strtoul(blk.getLine().c_str(), NULL, 16));
    }
}

void data::saveBlackList()
{
    std::string blPath = fs::getWorkDir() + "blacklist.txt";
    FILE *bl = fopen(blPath.c_str(), "w");
    for(uint64_t id : blacklist)
        fprintf(bl, "#%s\n0x%016lX\n", data::getTitleNameByTID(id).c_str(), id);

    fclose(bl);
}

void data::blacklistAdd(const uint64_t& tid)
{
    blacklist.push_back(tid);
    for(data::user& _u : data::users)
    {
        for(unsigned i = 0; i < _u.titleInfo.size(); i++)
            if(_u.titleInfo[i].saveID == tid) _u.titleInfo.erase(_u.titleInfo.begin() + i);
    }
}

void data::favoriteTitle(const uint64_t& tid)
{
    if(!isFavorite(tid))
    {
        titles[tid].fav = true;
        favorites.push_back(tid);
    }
    else
    {
        titles[tid].fav = false;
        auto ind = std::find(favorites.begin(), favorites.end(), tid);
        favorites.erase(ind);
    }

    for(auto &u : data::users)
        std::sort(u.titleInfo.begin(), u.titleInfo.end(), sortTitles);
}

void data::loadCfg()
{
    std::string cfgPath = fs::getWorkDir() + "cfg.bin";
    if(fs::fileExists(cfgPath))
    {
        FILE *cfg = fopen(cfgPath.c_str(), "rb");

        uint64_t cfgIn = 0;
        fread(&cfgIn, sizeof(uint64_t), 1, cfg);
        fread(&data::sortType, 1, 1, cfg);
        fclose(cfg);

        data::incDev = cfgIn >> 63 & 1;
        data::autoBack = cfgIn >> 62 & 1;
        data::ovrClk = cfgIn >> 61 & 1;
        data::holdDel = cfgIn >> 60 & 1;
        data::holdRest = cfgIn >> 59 & 1;
        data::holdOver = cfgIn >> 58 & 1;
        data::forceMount = cfgIn >> 57 & 1;
        data::accSysSave = cfgIn >> 56 & 1;
        data::sysSaveWrite = cfgIn >> 55 & 1;
        data::directFsCmd = cfgIn >> 53 & 1;
        data::skipUser = cfgIn >> 52 & 1;
        data::zip = cfgIn >> 51 & 1;
        data::langOverride = cfgIn >> 50 & 1;
    }
}

void data::saveCfg()
{
    std::string cfgPath = fs::getWorkDir() + "cfg.bin";
    FILE *cfg = fopen(cfgPath.c_str(), "wb");

    //Use 64bit int for space future stuff. Like this for readability.
    uint64_t cfgOut = 0;
    cfgOut |= (uint64_t)data::incDev << 63;
    cfgOut |= (uint64_t)data::autoBack << 62;
    cfgOut |= (uint64_t)data::ovrClk << 61;
    cfgOut |= (uint64_t)data::holdDel << 60;
    cfgOut |= (uint64_t)data::holdRest << 59;
    cfgOut |= (uint64_t)data::holdOver << 58;
    cfgOut |= (uint64_t)data::forceMount << 57;
    cfgOut |= (uint64_t)data::accSysSave << 56;
    cfgOut |= (uint64_t)data::sysSaveWrite << 55;
    cfgOut |= (uint64_t)data::directFsCmd << 53;
    cfgOut |= (uint64_t)data::skipUser << 52;
    cfgOut |= (uint64_t)data::zip << 51;
    cfgOut |= (uint64_t)data::langOverride << 50;
    fwrite(&cfgOut, sizeof(uint64_t), 1, cfg);
    fwrite(&data::sortType, 1, 1, cfg);

    fclose(cfg);
}

void data::restoreDefaultConfig()
{
    data::incDev = false;
    data::autoBack = true;
    data::ovrClk = false;
    data::holdDel = true;
    data::holdRest = true;
    data::holdOver = true;
    data::forceMount = true;
    data::accSysSave = false;
    data::sysSaveWrite = false;
    ui::textMode = false;
    data::directFsCmd = false;
    data::skipUser = false;
    data::zip = false;
    data::sortType = 0;
}

void data::loadFav()
{
    fs::dataFile fav(fs::getWorkDir() + "favorites.txt");
    if(fav.isOpen())
    {
        while(fav.readNextLine(false))
            favorites.push_back(strtoul(fav.getLine().c_str(), NULL, 16));
    }
}

void data::saveFav()
{
    std::string favPath = fs::getWorkDir() + "favorites.txt";
    FILE *fav = fopen(favPath.c_str(), "w");
    for(uint64_t& fid : favorites)
        fprintf(fav, "0x%016lX\n", fid);

    fclose(fav);
}

void data::loadDefs()
{
    std::string defPath = fs::getWorkDir() + "titleDefs.txt";
    if(fs::fileExists(defPath))
    {
        fs::dataFile def(defPath);
        while(def.readNextLine(true))
        {
            uint64_t id = strtoull(def.getName().c_str(), NULL, 16);
            pathDefs[id] = def.getNextValueStr();
        }
    }
}

static const SDL_Color green = {0x00, 0xDD, 0x00, 0xFF};

void data::dispStats()
{
    //Easiest/laziest way to do this
    std::string stats = "User Count: " + std::to_string(users.size()) + "\n";
    for(data::user& u : data::users)
        stats += u.getUsername() + ": " + std::to_string(u.titleInfo.size()) + "\n";
    stats += "Current User: " + data::curUser.getUsername() + "\n";
    stats += "Current Title: " + data::getTitleNameByTID(data::curData.saveID) + "\n";
    stats += "Safe Title: " + data::getTitleSafeNameByTID(data::curData.saveID) + "\n";
    stats += "Sort Type: " + std::to_string(data::sortType) + "\n";
    gfx::drawTextf(NULL, 16, 2, 2, &green, stats.c_str());
}
