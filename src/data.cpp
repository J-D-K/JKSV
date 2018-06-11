#include <stdio.h>
#include <switch.h>

#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <algorithm>

#include "data.h"
#include "sys.h"
#include "gfx.h"

static const char verboten[] = { '.', ',', '/', '\\', '<', '>', ':', '"', '|', '?', '*'};

bool isVerboten(char t)
{
    for(unsigned i = 0; i < 11; i++)
    {
        if(t == verboten[i])
            return true;
    }

    return false;
}

const std::string safeString(const std::string& s)
{
    std::string ret = "";
    for(unsigned i = 0; i < s.length(); i++)
    {
        if(isVerboten(s[i]))
        {
            ret += ' ';
        }
        else
            ret += s[i];
    }

    return ret;
}

namespace data
{
    titledata curData;
    user      curUser;
    std::vector<user> users;

    bool init()
    {
        Result res = 0;

        res = accountInitialize();
        if(res)
        {
            printf("accountInitialize failed!");
            return false;
        }

        return true;
    }

    bool fini()
    {
        accountExit();

        return true;
    }


    struct
    {
        bool operator()(titledata& a, titledata& b)
        {
            for(unsigned i = 0; i < a.getTitle().length(); i++)
            {
                int charA = tolower(a.getTitle().c_str()[i]), charB = tolower(b.getTitle().c_str()[i]);
                if(charA != charB)
                    return charA < charB;
            }

            return false;
        }
    } sortTitles;

    int getUserIndex(const u128& id)
    {
        for(unsigned i = 0; i < users.size(); i++)
        {
            if(users[i].getUID() == id)
                return i;
        }

        return -1;
    }

    void waitForA()
    {
        while(true)
        {
            hidScanInput();

            uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);

            if(down & KEY_A)
                break;

            gfxFlushBuffers();
            gfxSwapBuffers();
            gfxWaitForVsync();
        }
    }

    void loadDataInfo()
    {
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

            if(info.SaveDataType == FsSaveDataType_SaveData)
            {
                int u = getUserIndex(info.userID);
                if(u == -1)
                {
                    user newUser;
                    if(newUser.init(info.userID))
                    {
                        users.push_back(newUser);

                        u = getUserIndex(info.userID);
                        titledata newData;
                        if(newData.init(info.titleID, info.userID))
                            users[u].titles.push_back(newData);
                    }
                }
                else
                {
                    titledata newData;
                    if(newData.init(info.titleID, info.userID))
                    {
                        users[u].titles.push_back(newData);
                    }
                }
            }
        }

        fsSaveDataIteratorClose(&saveIt);

        for(unsigned i = 0; i < users.size(); i++)
            std::sort(users[i].titles.begin(), users[i].titles.end(), sortTitles);

    }

    bool titledata::init(const uint64_t& _id, const u128& _uID)
    {
        Result res = 0;
        NsApplicationControlData *dat = new NsApplicationControlData;
        std::memset(dat, 0, sizeof(NsApplicationControlData));
        NacpLanguageEntry *ent = NULL;

        id = _id;
        uID = _uID;
        size_t outSz = 0;

        res = nsGetApplicationControlData(1, _id, dat, sizeof(NsApplicationControlData), &outSz);
        if(R_FAILED(res) && outSz < sizeof(dat->nacp))
        {
            printf("nsGetAppCtrlData Failed\n");
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
            titleSafe = safeString(ent->name);
            delete dat;
        }
        else
        {
            char tmp[32];
            sprintf(tmp, "%016lX", id);
            title.assign(tmp);
            titleSafe = safeString(tmp);
        }

        return true;
    }

    const std::string titledata::getTitle()
    {
        return title;
    }

    const std::string titledata::getTitleSafe()
    {
        return titleSafe;
    }

    const uint64_t titledata::getID()
    {
        return id;
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
            username = "???";

        accountProfileClose(&prof);

        return true;
    }

    const u128 user::getUID()
    {
        return userID;
    }

    const std::string user::getUsername()
    {
        return username;
    }
}
