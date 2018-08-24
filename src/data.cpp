#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <switch.h>

#include "data.h"
#include "file.h"
#include "util.h"

//Sorts titles sort-of alphabetically
static struct
{
    bool operator()(data::titledata& a, data::titledata& b)
    {
        uint32_t tmpA, tmpB;
        for(unsigned i = 0; i < a.getTitle().length(); )
        {
            ssize_t uCnt = decode_utf8(&tmpA, (const uint8_t *)&a.getTitle().data()[i]);
            decode_utf8(&tmpB, (const uint8_t *)&b.getTitle().data()[i]);
            tmpA = tolower(tmpA);
            tmpB = tolower(tmpB);
            if(tmpA != tmpB)
                return tmpA < tmpB;

            i += uCnt;
        }

        return false;
    }
} sortTitles;

//Returns -1 for new
static int getUserIndex(const u128& id)
{
    for(unsigned i = 0; i < data::users.size(); i++)
    {
        if(data::users[i].getUID() == id)
            return i;
    }

    return -1;
}

static std::vector<uint64_t> blacklist;

bool blacklisted(const uint64_t& id)
{
    for(unsigned i = 0; i < blacklist.size(); i++)
    {
        if(id == blacklist[i])
            return true;
    }

    return false;
}

namespace data
{
    titledata curData;
    user      curUser;
    std::vector<icn> icons;
    std::vector<user> users;
    bool sysSave = false, forceMount = true;

    void loadDataInfo()
    {
        blacklist.clear();
        loadBlacklist();

        icn defIcon;
        defIcon.load(0, "romfs:/img/icn/icnDefault.png");
        icons.push_back(defIcon);

        for(unsigned i = 0; i < users.size(); i++)
            users[i].titles.clear();

        users.clear();

        Result res = 0;
        FsSaveDataIterator saveIt;
        size_t total = 0;
        FsSaveDataInfo info;

        res = fsOpenSaveDataIterator(&saveIt, FsSaveDataSpaceId_All);
        if(R_FAILED(res))
        {
            printf("SaveDataIterator Failed\n");
            return;
        }

        while(true)
        {
            res = fsSaveDataIteratorRead(&saveIt, &info, 1, &total);
            if(R_FAILED(res) || total == 0)
                break;

            if((info.SaveDataType == FsSaveDataType_SaveData && !blacklisted(info.titleID)) || sysSave)
            {
                int u = getUserIndex(info.userID);
                if(u == -1)
                {
                    user newUser;
                    if(newUser.init(info.userID) || (sysSave && newUser.initNoChk(info.userID)))
                    {
                        users.push_back(newUser);

                        u = getUserIndex(info.userID);
                        titledata newData;
                        if(newData.init(info) && (newData.isMountable(newUser.getUID()) || !forceMount))
                        {
                            users[u].titles.push_back(newData);
                        }
                    }
                }
                else
                {
                    titledata newData;
                    if(newData.init(info) && (newData.isMountable(users[u].getUID()) || !forceMount))
                    {
                        users[u].titles.push_back(newData);
                    }
                }
            }
        }

        fsSaveDataIteratorClose(&saveIt);

        for(unsigned i = 0; i < users.size(); i++)
            std::sort(users[i].titles.begin(), users[i].titles.end(), sortTitles);

        curUser = users[0];
    }

    void exit()
    {
        for(unsigned i = 0; i < users.size(); i++)
            texDestroy(users[i].userIcon);

        for(unsigned i = 0; i < icons.size(); i++)
            icons[i].deleteData();
    }

    void icn::load(const uint64_t& _id, const uint8_t *jpegData, const size_t& jpegSize)
    {
        titleID = _id;
        iconTex = texLoadJPEGMem(jpegData, jpegSize);
    }

    void icn::load(const uint64_t& _id, const std::string& _png)
    {
        titleID = _id;
        iconTex = texLoadPNGFile(_png.c_str());
    }

    int findIcnIndex(const uint64_t& titleID)
    {
        for(unsigned i = 0; i < icons.size(); i++)
        {
            if(icons[i].getTitleID() == titleID)
                return i;
        }

        return -1;
    }

    bool titledata::init(const FsSaveDataInfo& inf)
    {
        Result res = 0;
        NsApplicationControlData *dat = new NsApplicationControlData;
        std::memset(dat, 0, sizeof(NsApplicationControlData));
        NacpLanguageEntry *ent = NULL;

        if(inf.SaveDataType == FsSaveDataType_SaveData)
            id = inf.titleID;
        else if(inf.SaveDataType == FsSaveDataType_SystemSaveData)
            id = inf.saveID;

        uID = inf.userID;
        type = (FsSaveDataType)inf.SaveDataType;
        size_t outSz = 0;

        res = nsGetApplicationControlData(1, id, dat, sizeof(NsApplicationControlData), &outSz);
        if(R_FAILED(res) || outSz < sizeof(dat->nacp))
        {
            if(!sysSave)
                printf("nsGetAppCtrlData Failed: 0x%08X\n", (unsigned)res);
            delete dat;
        }

        if(R_SUCCEEDED(res))
        {
            res = nacpGetLanguageEntry(&dat->nacp, &ent);
            if(R_FAILED(res) || ent == NULL)
            {
                printf("nacpGetLanguageEntry Failed\n");
                delete dat;
            }
        }

        if(R_SUCCEEDED(res))
        {
            title.assign(ent->name);
            titleSafe = util::safeString(title);
            if(titleSafe.empty())
            {
                char tmp[18];
                sprintf(tmp, "%016lX", id);
                titleSafe.assign(tmp);
            }

            int iconIndex = findIcnIndex(id);
            if(iconIndex == -1)
            {
                uint32_t icnSize = outSz - sizeof(dat->nacp);
                icn newIcon;
                newIcon.load(id, dat->icon, icnSize);
                icons.push_back(newIcon);

                icon = icons[findIcnIndex(id)];
            }
            else
                icon = icons[iconIndex];

            delete dat;
        }
        else
        {
            char tmp[32];
            sprintf(tmp, "%016lX", (u64)id);
            title.assign(tmp);
            titleSafe = util::safeString(tmp);
            icon = icons[0];
        }

        return true;
    }

    bool titledata::isMountable(const u128& uID)
    {
        data::user tmpUser;
        tmpUser.initNoChk(uID);
        if(fs::mountSave(tmpUser, *this))
        {
            fsdevUnmountDevice("sv");
            return true;
        }
        return false;
    }

    //ASCII Testing
    void titledata::debugCreate(const uint64_t& _id, const std::string& t)
    {
        id = _id;
        title = t;

        titleSafe = util::safeString(t);
        if(titleSafe.empty())
        {
            char tmp[18];
            sprintf(tmp, "%016lX", id);
            titleSafe.assign(tmp);
        }
    }

    bool user::init(const u128& _id)
    {
        Result res = 0;
        userID = _id;

        AccountProfile prof;
        AccountProfileBase base;

        res = accountGetProfile(&prof, userID);
        if(R_FAILED(res))
            return false;

        res = accountProfileGet(&prof, NULL, &base);
        if(R_FAILED(res))
            return false;

        username.assign(base.username);
        if(username.empty())
            username = "Unknown";
        userSafe = util::safeString(username);

        size_t sz = 0;
        accountProfileGetImageSize(&prof, &sz);
        uint8_t *profJpeg = new uint8_t[sz];

        accountProfileLoadImage(&prof, profJpeg, sz, &sz);
        userIcon = texLoadJPEGMem(profJpeg, sz);

        delete[] profJpeg;

        accountProfileClose(&prof);

        return true;
    }

    bool user::initNoChk(const u128& _id)
    {
        Result res = 0;
        userID = _id;

        AccountProfile prof;
        AccountProfileBase base;

        res = accountGetProfile(&prof, userID);
        if(R_SUCCEEDED(res))
        {
            res = accountProfileGet(&prof, NULL, &base);
        }

        if(R_SUCCEEDED(res))
        {
            username.assign(base.username);
            userSafe = util::safeString(username);
            accountProfileClose(&prof);
        }
        else
        {
            username = "Unknown";
            userSafe = "Unknown";
            //This shouldn't happen too much
            userIcon = texLoadPNGFile("romfs:/img/icn/icnDefault.png");
        }

        return true;
    }

    void loadBlacklist()
    {
        if(fs::fileExists(fs::getWorkDir() + "blacklist.txt"))
        {
            std::string line;
            std::fstream bl(fs::getWorkDir() + "blacklist.txt", std::ios::in);

            while(std::getline(bl, line))
            {
                if(line[0] == '#' || line[0] == '\n')
                    continue;

                uint64_t pushID = std::strtoull(line.c_str(), NULL, 16);

                blacklist.push_back(pushID);
            }
            bl.close();
        }
    }

    void blacklistAdd(user& u, titledata& t)
    {
        std::fstream bl(fs::getWorkDir() + "blacklist.txt", std::ios::app);

        std::string titleLine = "#" + t.getTitle() + "\n";
        char idLine[32];
        sprintf(idLine, "0x%016lX\n", t.getID());

        bl.write(titleLine.c_str(), titleLine.length());
        bl.write(idLine, std::strlen(idLine));
        bl.close();

        //Remove it from every user
        for(unsigned i = 0; i < users.size(); i++)
        {
            for(unsigned j = 0; j < users[i].titles.size(); j++)
            {
                if(users[i].titles[j].getID() == t.getID())
                    users[i].titles.erase(users[i].titles.begin() + j);
            }
        }

        int uInd = getUserIndex(u.getUID());
        u = users[uInd];
    }
}
