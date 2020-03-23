#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <switch.h>
#include <zlib.h>

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
static int getUserIndex(const AccountUid& id)
{
    u128 nId = 0, oId = 0;
    nId = util::accountUIDToU128(id);
    for(unsigned i = 0; i < data::users.size(); i++)
    {
        oId = util::accountUIDToU128(data::users[i].getUID());
        if(oId == nId)
            return i;
    }

    return -1;
}

static std::vector<uint64_t> blacklist;

static bool blacklisted(const uint64_t& id)
{
    for(unsigned i = 0; i < blacklist.size(); i++)
    {
        if(id == blacklist[i])
            return true;
    }

    return false;
}

static tex *createDeviceIcon()
{
    tex *ret = texCreate(256, 256);
    texClearColor(ret, ui::rectLt);
    unsigned x = 128 - (textGetWidth("\ue121", ui::shared, 144) / 2);
    drawText("\ue121", ret, ui::shared, x, 56, 144, ui::mnuTxt);
    return ret;
}

namespace data
{
    titledata curData;
    user      curUser;
    std::vector<icn> icons;
    std::vector<user> users;
    bool forceMount = true;
    bool isSpcd = false;

    void loadDataInfo()
    {
        //Check date
        time_t raw;
        time(&raw);
        tm *locTime = localtime(&raw);
        if(locTime->tm_mon == 3 && locTime->tm_mday == 1)
            isSpcd = true;

        //Clear titles + users just in case
        for(unsigned i = 0; i < users.size(); i++)
            users[i].titles.clear();

        users.clear();

        loadBlacklist();

        FsSaveDataInfoReader saveIt;
        s64 total = 0;
        FsSaveDataInfo info;

        if(R_FAILED(fsOpenSaveDataInfoReader(&saveIt, FsSaveDataSpaceId_All)))
        {
            printf("SaveDataIterator Failed\n");
            return;
        }

        //Push System and BCAT user
        user sys, bcat, dev;
        sys.initNoChk(util::u128ToAccountUID(1), "System");
        bcat.initNoChk(util::u128ToAccountUID(2), "BCAT");
        dev.initNoChk(util::u128ToAccountUID(3), "Device Saves");

        //Modify device icon
        dev.delIcon();
        dev.assignIcon(createDeviceIcon());

        users.push_back(dev);
        users.push_back(bcat);
        users.push_back(sys);

        while(true)
        {
            if(R_FAILED(fsSaveDataInfoReaderRead(&saveIt, &info, 1, &total)) || total == 0)
                break;

            switch(info.save_data_type)
            {
                case FsSaveDataType_System:
                    info.uid = util::u128ToAccountUID(1);
                    break;

                case FsSaveDataType_Bcat:
                    info.uid = util::u128ToAccountUID(2);
                    break;

                case FsSaveDataType_Device:
                    info.uid = util::u128ToAccountUID(3);
                    break;
            }

            //If save data, not black listed or just ignore
            if(!blacklisted(info.application_id))
            {
                int u = getUserIndex(info.uid);
                if(u == -1)
                {
                    user newUser;
                    if(newUser.init(info.uid))
                    {
                        //Always insert new users to beginning
                        users.insert(users.begin(), newUser);

                        u = getUserIndex(info.uid);
                        titledata newData;
                        newData.init(info);
                        if(newData.isMountable(newUser.getUID()) || !forceMount)
                            users[u].titles.push_back(newData);
                    }
                }
                else
                {
                    titledata newData;
                    newData.init(info);
                    if(newData.isMountable(users[u].getUID()) || !forceMount)
                        users[u].titles.push_back(newData);
                }
            }
        }

        fsSaveDataInfoReaderClose(&saveIt);

        for(unsigned i = 0; i < users.size(); i++)
            std::sort(users[i].titles.begin(), users[i].titles.end(), sortTitles);

        curUser = users[0];
    }

    void exit()
    {
        for(unsigned i = 0; i < users.size(); i++)
            users[i].delIcon();

        for(unsigned i = 0; i < icons.size(); i++)
            icons[i].deleteData();
    }

    void icn::load(const uint64_t& _id, const uint8_t *jpegData, const size_t& jpegSize)
    {
        titleID = _id;

        if(isSpcd)
            iconTex = util::loadDefaultIcon();
        else
            iconTex = texLoadJPEGMem(jpegData, jpegSize);
    }

    void icn::load(const uint64_t& _id, const std::string& _png)
    {
        titleID = _id;
        iconTex = texLoadPNGFile(_png.c_str());
    }

    void icn::create(const uint64_t& _id, const std::string& _txt)
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

    void titledata::init(const FsSaveDataInfo& inf)
    {
        NsApplicationControlData *dat = new NsApplicationControlData;
        std::memset(dat, 0, sizeof(NsApplicationControlData));
        NacpLanguageEntry *ent = NULL;
        size_t outSz = 0;

        info = inf;
        if(inf.save_data_type == FsSaveDataType_System)
            id = inf.system_save_data_id;
        else
            id = inf.application_id;

        if(R_SUCCEEDED(nsGetApplicationControlData(NsApplicationControlSource_Storage, id, dat, sizeof(NsApplicationControlData), &outSz)) && outSz >= sizeof(dat->nacp) \
                && R_SUCCEEDED(nacpGetLanguageEntry(&dat->nacp, &ent)) && ent != NULL)
        {
            title.assign(ent->name);
            titleSafe.assign(util::safeString(title));
            if(titleSafe.empty())
            {
                char tmp[18];
                sprintf(tmp, "%016lX", id);
                titleSafe.assign(tmp);
            }

            int icnInd = findIcnIndex(id);
            if(icnInd == -1)
            {
                size_t icnSize = outSz - sizeof(dat->nacp);
                icn newIcn;
                newIcn.load(id, dat->icon, icnSize);
                icons.push_back(newIcn);

                icon = icons[findIcnIndex(id)];
            }
            else
                icon = icons[icnInd];
        }
        else
        {
            char tmp[18];
            sprintf(tmp, "%016lX", id);
            title.assign(tmp);
            titleSafe.assign(tmp);
            icon.create(id, "");
        }
        delete dat;
    }

    bool titledata::isMountable(const AccountUid& uID)
    {
        data::user tmpUser;
        tmpUser.setUID(uID);
        if(fs::mountSave(tmpUser, *this))
        {
            fsdevUnmountDevice("sv");
            return true;
        }
        return false;
    }

    bool user::init(const AccountUid& _id)
    {
        userID = _id;

        AccountProfile prof;
        AccountProfileBase base;

        if(R_FAILED(accountGetProfile(&prof, userID)))
            return false;

        if(R_FAILED(accountProfileGet(&prof, NULL, &base)))
            return false;

        username.assign(base.nickname);
        if(username.empty())
            username = "Unknown";
        userSafe = util::safeString(username);
        if(!isSpcd)
        {
            uint32_t sz = 0;
            accountProfileGetImageSize(&prof, &sz);
            uint8_t *profJpeg = new uint8_t[sz];

            accountProfileLoadImage(&prof, profJpeg, sz, &sz);
            userIcon = texLoadJPEGMem(profJpeg, sz);

            delete[] profJpeg;
        }
        else
            userIcon = util::loadDefaultIcon();

        accountProfileClose(&prof);

        return true;
    }

    bool user::initNoChk(const AccountUid& _id, const std::string& _backupName)
    {
        userID = _id;

        username = _backupName;
        userSafe = util::safeString(_backupName);

        //create generic icon
        userIcon = util::createIconGeneric(_backupName.c_str());
        //userIcon = texLoadPNGFile("romfs:/img/icn/icnDefault.png");

        return true;
    }

    void loadBlacklist()
    {
        blacklist.clear();
        if(fs::fileExists(fs::getWorkDir() + "blacklist.txt"))
        {
            std::string line;
            std::fstream bl(fs::getWorkDir() + "blacklist.txt", std::ios::in);

            while(std::getline(bl, line))
            {
                if(line[0] == '#' || line[0] == '\n')
                    continue;

                blacklist.push_back(std::strtoull(line.c_str(), NULL, 16));
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
