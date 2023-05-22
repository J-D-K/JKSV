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
#include "type.h"
#include "cfg.h"

//FsSaveDataSpaceId_All doesn't work for SD
static const unsigned saveOrder [] = { 0, 1, 2, 3, 4, 100, 101 };

int selUser = 0, selData = 0;

//User vector
std::vector<data::user> data::users;

//System language
SetLanguage data::sysLang;

//For other save types
static bool sysBCATPushed = false, tempPushed = false;
std::unordered_map<uint64_t, data::titleInfo> data::titles;

//Sorts titles by sortType
static struct
{
    bool operator()(const data::userTitleInfo& a, const data::userTitleInfo& b)
    {
        //Favorites override EVERYTHING
        if(cfg::isFavorite(a.tid) != cfg::isFavorite(b.tid)) return cfg::isFavorite(a.tid);

        switch(cfg::sortType)
        {
            case cfg::ALPHA:
                {
                    std::string titleA = data::getTitleNameByTID(a.tid);
                    std::string titleB = data::getTitleNameByTID(b.tid);
                    uint32_t pointA, pointB;
                    for(unsigned i = 0, j = 0; i < titleA.length(); )
                    {
                        ssize_t aCnt = decode_utf8(&pointA, (const uint8_t *)&titleA.data()[i]);
                        ssize_t bCnt = decode_utf8(&pointB, (const uint8_t *)&titleB.data()[j]);
                        pointA = tolower(pointA), pointB = tolower(pointB);
                        if(pointA != pointB)
                            return pointA < pointB;

                        i += aCnt;
                        j += bCnt;
                    }
                }
                break;

            case cfg::MOST_PLAYED:
                return a.playStats.playtime > b.playStats.playtime;
                break;

            case cfg::LAST_PLAYED:
                return a.playStats.last_timestamp_user> b.playStats.last_timestamp_user;
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

static inline bool accountSystemSaveCheck(const FsSaveDataInfo& _inf)
{
    if(_inf.save_data_type == FsSaveDataType_System && util::accountUIDToU128(_inf.uid) != 0 && !cfg::config["accSysSave"])
        return false;

    return true;
}

//Minimal init/test to avoid loading and creating things I don't need
static bool testMount(const FsSaveDataInfo& _inf)
{
    bool ret = false;
    if(!cfg::config["forceMount"])
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
        if(strlen(ent->name) == 0)
            data::titles[tid].title = ctrlData->nacp.lang[SetLanguage_ENUS].name;
        else
            data::titles[tid].title = ent->name;
        data::titles[tid].author = ent->author;
        if(cfg::isDefined(tid))
            data::titles[tid].safeTitle = cfg::getPathDefinition(tid);
        else if((data::titles[tid].safeTitle = util::safeString(ent->name)) == "")
            data::titles[tid].safeTitle = util::getIDStr(tid);

        if(cfg::isFavorite(tid))
            data::titles[tid].fav = true;
        
        data::titles[tid].icon = gfx::texMgr->textureLoadFromMem(IMG_FMT_JPG, ctrlData->icon, iconSize);
        if(!data::titles[tid].icon)
            data::titles[tid].icon = util::createIconGeneric(util::getIDStrLower(tid).c_str(), 32, true);
    }
    else
    {
        memset(&data::titles[tid].nacp, 0, sizeof(NacpStruct));
        data::titles[tid].title = util::getIDStr(tid);
        data::titles[tid].author = "Someone?";
        if(cfg::isDefined(tid))
            data::titles[tid].safeTitle = cfg::getPathDefinition(tid);
        else
            data::titles[tid].safeTitle = util::getIDStr(tid);

        data::titles[tid].icon = util::createIconGeneric(util::getIDStrLower(tid).c_str(), 32, true);
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
            data::users.emplace_back(uids[i], "", "");
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

static void importSVIs()
{
    std::string sviDir = fs::getWorkDir() + "svi/";
    fs::dirList sviList(sviDir);
    if(sviList.getCount() > 0)
    {
        for(unsigned i = 0; i < sviList.getCount(); i++)
        {
            uint64_t tid = 0;
            NacpStruct *nacp = new NacpStruct;
            NacpLanguageEntry *ent;
            std::string sviPath = fs::getWorkDir() + "svi/" + sviList.getItem(i);

            size_t iconSize = fs::fsize(sviPath) - (sizeof(uint64_t) + sizeof(NacpStruct));
            uint8_t *iconBuffer = new uint8_t[iconSize];
            FILE *sviIn = fopen(sviPath.c_str(), "rb");
            fread(&tid, sizeof(uint64_t), 1, sviIn);
            fread(nacp, sizeof(NacpStruct), 1, sviIn);
            fread(iconBuffer, 1, iconSize, sviIn);

            if(!titleIsLoaded(tid))
            {
                nacpGetLanguageEntry(nacp, &ent);
                memcpy(&data::titles[tid].nacp, nacp, sizeof(NacpStruct));
                data::titles[tid].title = ent->name;
                data::titles[tid].author = ent->author;
                if(cfg::isDefined(tid))
                    data::titles[tid].safeTitle = cfg::getPathDefinition(tid);
                else if((data::titles[tid].safeTitle = util::safeString(ent->name)) == "")
                    data::titles[tid].safeTitle = util::getIDStr(tid);

                if(cfg::isFavorite(tid))
                    data::titles[tid].fav = true;

                if(!data::titles[tid].icon && iconSize > 0)
                    data::titles[tid].icon = gfx::texMgr->textureLoadFromMem(IMG_FMT_JPG, iconBuffer, iconSize);
                else if(!data::titles[tid].icon && iconSize == 0)
                    data::titles[tid].icon = util::createIconGeneric(util::getIDStrLower(tid).c_str(), 32, true);
            }
            delete nacp;
            delete[] iconBuffer;
            fclose(sviIn);
        }
    }
}

bool data::loadUsersTitles(bool clearUsers)
{
    static unsigned systemUserCount = 4;
    FsSaveDataInfoReader it;
    FsSaveDataInfo info;
    s64 total = 0;

    loadTitlesFromRecords();
    importSVIs();

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

        users.emplace_back(util::u128ToAccountUID(3), ui::getUIString("saveTypeMainMenu", 0), "Device");
        users.emplace_back(util::u128ToAccountUID(2), ui::getUIString("saveTypeMainMenu", 1), "BCAT");
        users.emplace_back(util::u128ToAccountUID(5), ui::getUIString("saveTypeMainMenu", 2), "Cache");
        users.emplace_back(util::u128ToAccountUID(0), ui::getUIString("saveTypeMainMenu", 3), "System");
    }

    for(unsigned i = 0; i < 7; i++)
    {
        if(R_FAILED(fsOpenSaveDataInfoReader(&it, (FsSaveDataSpaceId)saveOrder[i])))
            continue;

        while(R_SUCCEEDED(fsSaveDataInfoReaderRead(&it, &info, 1, &total)) && total != 0)
        {
            uint64_t tid = 0;
            if(info.save_data_type == FsSaveDataType_System || info.save_data_type == FsSaveDataType_SystemBcat)
                tid = info.system_save_data_id;
            else
                tid = info.application_id;

            if(!titleIsLoaded(tid))
                addTitleToList(tid);

            //Don't bother with this stuff
            if(cfg::isBlacklisted(tid) || !accountSystemSaveCheck(info) || !testMount(info))
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
                        users.emplace_back(util::u128ToAccountUID(4), ui::getUIString("saveTypeMainMenu", 4), "System BCAT");
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
                        users.emplace_back(util::u128ToAccountUID(6), ui::getUIString("saveTypeMainMenu", 5), "Temporary");
                    }
                    break;
            }

            int u = getUserIndex(info.uid);
            if(u == -1)
            {
                users.emplace(data::users.end() - systemUserCount, info.uid, "", "");
                u = getUserIndex(info.uid);
            }

            PdmPlayStatistics playStats;
            if(info.save_data_type == FsSaveDataType_Account || info.save_data_type == FsSaveDataType_Device)
                pdmqryQueryPlayStatisticsByApplicationIdAndUserAccountId(info.application_id, info.uid, false, &playStats);
            else
                memset(&playStats, 0, sizeof(PdmPlayStatistics));
            users[u].addUserTitleInfo(tid, &info, &playStats);
        }
        fsSaveDataInfoReaderClose(&it);
    }

    if(cfg::config["incDev"])
    {
        //Get reference to device save user
        unsigned devPos = getUserIndex(util::u128ToAccountUID(3));
        data::user& dev = data::users[devPos];
        for(unsigned i = 0; i < devPos; i++)
        {
            //Not needed but makes this easier to read
            data::user& u = data::users[i];
            u.titleInfo.insert(u.titleInfo.end(), dev.titleInfo.begin(), dev.titleInfo.end());
        }
    }

    data::sortUserTitles();

    return true;
}

void data::sortUserTitles()
{

    for(data::user& u : data::users)
        std::sort(u.titleInfo.begin(), u.titleInfo.end(), sortTitles);
}

void data::init()
{
    data::loadUsersTitles(true);
}

void data::exit()
{
    for(data::user& u : data::users) u.delIcon();
    for(auto& tinfo : titles)
        SDL_DestroyTexture(tinfo.second.icon);
}

void data::setUserIndex(unsigned _sUser)
{
    selUser = _sUser;
}

data::user *data::getCurrentUser()
{
    return &users[selUser];
}

unsigned data::getCurrentUserIndex()
{
    return selUser;
}

void data::setTitleIndex(unsigned _sTitle)
{
    selData = _sTitle;
}

data::userTitleInfo *data::getCurrentUserTitleInfo()
{
    return &users[selUser].titleInfo[selData];
}

unsigned data::getCurrentUserTitleInfoIndex()
{
    return selData;
}

data::titleInfo *data::getTitleInfoByTID(const uint64_t& tid)
{
    if(titles.find(tid) != titles.end())
        return &titles[tid];
    return NULL;
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

int data::getTitleIndexInUser(const data::user& u, const uint64_t& tid)
{
    for(unsigned i = 0; i < u.titleInfo.size(); i++)
    {
        if(u.titleInfo[i].tid == tid)
            return i;
    }
    return -1;
}

data::user::user(const AccountUid& _id, const std::string& _backupName, const std::string& _safeBackupName)
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
        userIcon = gfx::texMgr->textureLoadFromMem(IMG_FMT_JPG, jpegData, jpgSize);
        delete[] jpegData;

        accountProfileClose(&prof);
    }
    else
    {
        username = _backupName.empty() ? util::getIDStr((uint64_t)uID128) : _backupName;
        userSafe = _safeBackupName.empty() ? util::getIDStr((uint64_t)uID128) : _safeBackupName;
        userIcon = util::createIconGeneric(_backupName.c_str(), 48, false);
    }
    titles.reserve(64);
}

data::user::user(const AccountUid& _id, const std::string& _backupName, const std::string& _safeBackupName, SDL_Texture *img) : user(_id, _backupName, _safeBackupName)
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
    newInfo.tid = tid;
    memcpy(&newInfo.saveInfo, _saveInfo, sizeof(FsSaveDataInfo));
    memcpy(&newInfo.playStats, _stats, sizeof(PdmPlayStatistics));
    titleInfo.push_back(newInfo);
}

static const SDL_Color green = {0x00, 0xDD, 0x00, 0xFF};

void data::dispStats()
{
    data::user *cu = data::getCurrentUser();
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();

    //Easiest/laziest way to do this
    std::string stats = ui::getUICString("debugStatus", 0) + std::to_string(users.size()) + "\n";
    for(data::user& u : data::users)
        stats += u.getUsername() + ": " + std::to_string(u.titleInfo.size()) + "\n";
    stats += ui::getUICString("debugStatus", 1) + cu->getUsername() + "\n";
    stats += ui::getUICString("debugStatus", 2) + data::getTitleNameByTID(d->tid) + "\n";
    stats += ui::getUICString("debugStatus", 3) + data::getTitleSafeNameByTID(d->tid) + "\n";
    stats += ui::getUICString("debugStatus", 4) + std::to_string(cfg::sortType) + "\n";
    gfx::drawTextf(NULL, 16, 2, 2, &green, stats.c_str());
}