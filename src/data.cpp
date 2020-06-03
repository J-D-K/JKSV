#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <switch.h>

#include "data.h"
#include "file.h"
#include "util.h"

int data::selUser = 0, data::selData = 0;

//Icon/User vectors
std::vector<data::icn> data::icons;
std::vector<data::user> data::users;

//System language
SetLanguage data::sysLang;

//Options
bool data::incDev = false, data::autoBack = true, data::ovrClk = false, data::holdDel = true, data::holdRest = true, data::holdOver = true;
bool data::forceMount = true, data::accSysSave = false, data::sysSaveWrite = false, data::directFsCmd = false, data::skipUser = false;

//For other save types
static bool sysBCATPushed = false, cachePushed = false, tempPushed = false;

static std::vector<uint64_t> blacklist;
static std::vector<uint64_t> favorites;

//Sorts titles sort-of alphabetically
static struct
{
    bool operator()(const data::titledata& a, const data::titledata& b)
    {
        if(a.getFav() != b.getFav()) return a.getFav();

        uint32_t tmpA, tmpB;
        for(unsigned i = 0; i < a.getTitle().length(); )
        {
            ssize_t uCnt = decode_utf8(&tmpA, (const uint8_t *)&a.getTitle().data()[i]);
            decode_utf8(&tmpB, (const uint8_t *)&b.getTitle().data()[i]);
            tmpA = tolower(tmpA), tmpB = tolower(tmpB);
            if(tmpA != tmpB)
                return tmpA < tmpB;

            i += uCnt;
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

static int findIcnIndex(const uint64_t& titleID)
{
    for(unsigned i = 0; i < data::icons.size(); i++)
        if(data::icons[i].getTitleID() == titleID) return i;

    return -1;
}

static bool blacklisted(const uint64_t& tid)
{
    for(uint64_t& bid : blacklist)
        if(tid == bid) return true;

    return false;
}

static bool isFavorite(const uint64_t& tid)
{
    for(uint64_t& fid : favorites)
        if(tid == fid) return true;

    return false;
}

static tex *createDeviceIcon()
{
    tex *ret = texCreate(256, 256);
    texClearColor(ret, ui::rectLt);
    unsigned x = 128 - (textGetWidth("\ue121", ui::shared, 188) / 2);
    drawText("\ue121", ret, ui::shared, x, 34, 188, ui::txtCont);
    return ret;
}

static inline std::string getIDStr(const uint64_t& _id)
{
    char tmp[18];
    sprintf(tmp, "%016lX", _id);
    return std::string(tmp);
}

static inline bool accountSystemSaveCheck(const FsSaveDataInfo& _inf)
{
    if(_inf.save_data_type == FsSaveDataType_System && util::accountUIDToU128(_inf.uid) != 1 && !data::accSysSave)
        return false;

    return true;
}

bool data::loadUsersTitles(bool clearUsers)
{
    FsSaveDataInfoReader it;
    FsSaveDataInfo info;
    s64 total = 0;
    if(R_FAILED(fsOpenSaveDataInfoReader(&it, FsSaveDataSpaceId_All)))
        return false;

    //Clear titles
    for(data::user& u : data::users)
        u.titles.clear();
    if(clearUsers)
    {
        for(data::user& u : data::users)
            u.delIcon();

        data::users.clear();
        sysBCATPushed = false;
        cachePushed = false;
        tempPushed = false;
        users.emplace_back(util::u128ToAccountUID(3), "Device Saves", createDeviceIcon());
        users.emplace_back(util::u128ToAccountUID(2), "BCAT");
        users.emplace_back(util::u128ToAccountUID(1), "System");
    }

    NsApplicationControlData *dat = new NsApplicationControlData;
    while(R_SUCCEEDED(fsSaveDataInfoReaderRead(&it, &info, 1, &total)) && total != 0)
    {
        switch(info.save_data_type)
        {
            case FsSaveDataType_System:
                if(util::accountUIDToU128(info.uid) == 0)
                    info.uid = util::u128ToAccountUID(1);
                break;

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
                    sysBCATPushed = true;
                    users.emplace_back(util::u128ToAccountUID(4), "Sys. BCAT");
                }
                break;

            case FsSaveDataType_Cache:
                info.uid = util::u128ToAccountUID(5);
                if(!cachePushed)
                {
                    cachePushed = true;
                    users.emplace_back(util::u128ToAccountUID(5), "Cache");
                }
                break;

            case FsSaveDataType_Temporary:
                info.uid = util::u128ToAccountUID(6);
                if(!tempPushed)
                {
                    tempPushed = true;
                    users.emplace_back(util::u128ToAccountUID(6), "Temp");
                }
                break;
        }

        //Don't bother with this stuff
        if(blacklisted(info.application_id) || blacklisted(info.save_data_id) || !accountSystemSaveCheck(info))
            continue;

        int u = getUserIndex(info.uid);
        if(u == -1)
        {
            users.emplace(users.end() - 3, info.uid, "");
            u = getUserIndex(info.uid);
        }

        data::titledata newData(info, dat);
        if(!forceMount || newData.isMountable(data::users[u].getUID()))
            users[u].titles.push_back(newData);
    }
    delete dat;
    fsSaveDataInfoReaderClose(&it);

    if(data::incDev)
    {
        //Get reference to device save user
        data::user& dev = data::users[data::users.size() - 3];
        for(unsigned i = 0; i < users.size() - 3; i++)
        {
            //Not needed but makes this easier to read
            data::user& u = data::users[i];
            u.titles.insert(u.titles.end(), dev.titles.begin(), dev.titles.end());
        }
    }

    for(data::user& u : data::users)
        std::sort(u.titles.begin(), u.titles.end(), sortTitles);

    return true;
}

void data::init()
{
    loadBlacklist();
    loadFav();
    loadCfg();

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
    for(data::icn& i : data::icons) i.deleteData();

    saveFav();
    saveBlackList();
    util::setCPU(1020000000);
}

data::icn::icn(const uint64_t& _id, const uint8_t *jpegData, const size_t& jpegSize)
{
    titleID = _id;
    iconTex = texLoadJPEGMem(jpegData, jpegSize);
    createFav();
}

data::icn::icn(const uint64_t& _id, const std::string& _txt)
{
    titleID = _id;

    if(_txt.empty())
    {
        //use last 4 bytes of ID
        char tmp[10];
        sprintf(tmp, "%08X", (unsigned int)(_id & 0xFFFFFFFF));
        iconTex = util::createIconGeneric(tmp);
    }
    else
        iconTex = util::createIconGeneric(_txt.c_str());

    createFav();
}

void data::icn::createFav()
{
    iconFav = texCreate(256, 256);
    memcpy(iconFav->data, iconTex->data, 256 * 256 * sizeof(uint32_t));
    drawText("♥", iconFav, ui::shared, 0, 0, 48, clrCreateU32(0xFF4444FF));
}

data::titledata::titledata(const FsSaveDataInfo& inf, NsApplicationControlData *dat)
{
    size_t outSz = 0;
    NacpLanguageEntry *ent = NULL;
    memset(dat, 0, sizeof(NsApplicationControlData));

    if(inf.save_data_type == FsSaveDataType_System || inf.save_data_type == FsSaveDataType_SystemBcat)
        id = inf.system_save_data_id;
    else
        id = inf.application_id;

    saveID = inf.save_data_id;
    saveIndex = inf.save_data_index;
    tidStr    = getIDStr(id);
    saveIDStr = getIDStr(saveID);
    saveDataType = inf.save_data_type;

    Result ctrlDataRes = nsGetApplicationControlData(NsApplicationControlSource_Storage, id, dat, sizeof(NsApplicationControlData), &outSz);
    Result nacpRes = nacpGetLanguageEntry(&dat->nacp, &ent);
    size_t icnSize = outSz - sizeof(dat->nacp);
    int icnInd = findIcnIndex(id);
    if(R_SUCCEEDED(ctrlDataRes) && !(outSz < sizeof(dat->nacp)) && R_SUCCEEDED(nacpRes) && ent != NULL && icnSize > 0)
    {
        title.assign(ent->name);
        titleSafe.assign(util::safeString(title));
        author.assign(ent->author);
        if(titleSafe.empty())
            titleSafe = getIDStr(id);

        if(icnInd == -1)
        {
            data::icons.push_back(data::icn(id, dat->icon, icnSize));
            icnInd = data::icons.size() - 1;
        }

        icon = icons[icnInd];
    }
    else
    {
        if(icnInd == -1)
        {
            icons.push_back(data::icn(id, ""));
            icnInd = data::icons.size() - 1;
        }
        title = getIDStr(id);
        titleSafe = getIDStr(id);
        icon = icons[icnInd];
    }

    favorite = isFavorite(id);
    path = fs::getWorkDir() + titleSafe + "/";
}

bool data::titledata::isMountable(const AccountUid& uid)
{
    data::user tmpUser;
    tmpUser.setUID(uid);
    if(fs::mountSave(tmpUser, *this))
    {
        fs::unmountSave();
        return true;
    }
    return false;
}

void data::titledata::createDir() const
{
    mkdir(std::string(fs::getWorkDir() + titleSafe).c_str(), 777);
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
        userIcon = texLoadJPEGMem(jpegData, jpgSize);
        delete[] jpegData;

        accountProfileClose(&prof);
    }
    else
    {
        username = _backupName.empty() ? getIDStr((uint64_t)uID128) : _backupName;
        userSafe = _backupName.empty() ? getIDStr((uint64_t)uID128) : _backupName;
        userIcon = util::createIconGeneric(_backupName.c_str());
    }
}

data::user::user(const AccountUid& _id, const std::string& _backupName, tex *img) : user(_id, _backupName)
{
    delIcon();
    userIcon = img;
}

void data::user::setUID(const AccountUid& _id)
{
    userID = _id;
    uID128 = util::accountUIDToU128(_id);
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
    for(uint64_t& id : blacklist)
        fprintf(bl, "0x%016lX\n", id);

    fclose(bl);
}

void data::blacklistAdd(titledata& t)
{
    uint64_t tid = t.getID();
    for(data::user& _u : data::users)
    {
        for(unsigned i = 0; i < _u.titles.size(); i++)
            if(_u.titles[i].getID() == tid) _u.titles.erase(_u.titles.begin() + i);
    }
    blacklist.push_back(tid);
}

void data::favoriteTitle(titledata& t)
{
    uint64_t tid = t.getID();
    if(!t.getFav())
    {
        for(data::user& _u : data::users)
        {
            for(unsigned i = 0; i < _u.titles.size(); i++)
                if(_u.titles[i].getID() == tid) _u.titles[i].setFav(true);

            std::sort(_u.titles.begin(), _u.titles.end(), sortTitles);
        }
        favorites.push_back(tid);
    }
    else
    {
        auto ind = std::find(favorites.begin(), favorites.end(), tid);
        favorites.erase(ind);
        for(data::user& _u : data::users)
        {
            for(unsigned i = 0; i < _u.titles.size(); i++)
                if(_u.titles[i].getID() == tid) _u.titles[i].setFav(false);

            std::sort(_u.titles.begin(), _u.titles.end(), sortTitles);
        }
    }
}

void data::loadCfg()
{
    std::string cfgPath = fs::getWorkDir() + "cfg.bin";
    if(fs::fileExists(cfgPath))
    {
        FILE *cfg = fopen(cfgPath.c_str(), "rb");

        uint64_t cfgIn = 0;
        fread(&cfgIn, sizeof(uint64_t), 1, cfg);
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
        ui::textMode = cfgIn >> 54 & 1;
        data::directFsCmd = cfgIn >> 53 & 1;
        data::skipUser = cfgIn >> 52 & 1;
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
    cfgOut |= (uint64_t)ui::textMode << 54;
    cfgOut |= (uint64_t)data::directFsCmd << 53;
    cfgOut |= (uint64_t)data::skipUser << 52;
    fwrite(&cfgOut, sizeof(uint64_t), 1, cfg);

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
