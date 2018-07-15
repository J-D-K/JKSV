#include <string>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>

#include "data.h"
#include "gfx.h"
#include "util.h"
#include "file.h"
#include "ui.h"

static const char verboten[] = { '.', ',', '/', '\\', '<', '>', ':', '"', '|', '?', '*'};

namespace util
{
    std::string getDateTime()
    {
        char ret[48];

        time_t raw;
        time(&raw);
        tm *Time = localtime(&raw);

        sprintf(ret, "%04d-%02d-%02d@%02d-%02d-%02d", Time->tm_year + 1900, Time->tm_mon + 1, Time->tm_mday, Time->tm_hour, Time->tm_min, Time->tm_sec);

        return std::string(ret);
    }

    void makeTitleDir(data::user& u, data::titledata& t)
    {
        std::string path = fs::getWorkDir() + t.getTitleSafe();
        mkdir(path.c_str(), 777);
    }

    std::string getTitleDir(data::user& u, data::titledata& t)
    {
        return std::string(fs::getWorkDir() + t.getTitleSafe() + "/");
    }

    std::string getWrappedString(const std::string& s, const unsigned& sz, const unsigned& maxWidth)
    {
        if(gfx::getTextWidth(s, sz) < maxWidth)
            return s;

        std::string ret = "", tmp = "";
        unsigned first = 0, lastSpace = 0;

        for(unsigned i = 0; i < s.length(); i++)
        {
            tmp += s[i];

            // slash for really long file paths with no spaces.
            if(s[i] == ' ' || s[i] == '/')
                lastSpace = i;

            if(gfx::getTextWidth(tmp, sz) >= maxWidth)
            {
                tmp.assign(s, first, lastSpace - first);

                ret += tmp + "\n";

                first = lastSpace + 1;
                i = lastSpace;

                tmp.clear();
            }
        }
        if(!tmp.empty())
            ret += tmp;

        return ret;
    }

    void copyDirListToMenu(fs::dirList& d, ui::menu& m)
    {
        m.reset();
        m.addOpt(".");
        m.addOpt("..");
        for(unsigned i = 0; i < d.getCount(); i++)
        {
            if(d.isDir(i))
                m.addOpt("D " + d.getItem(i));
            else
                m.addOpt("F " + d.getItem(i));
        }
    }

    void removeLastFolderFromString(std::string& _path)
    {
        unsigned last = _path.find_last_of('/', _path.length() - 2);
        _path.erase(last + 1, _path.length());
    }

    bool isVerboten(char t)
    {
        for(unsigned i = 0; i < 11; i++)
        {
            if(t == verboten[i])
                return true;
        }

        return false;
    }

    std::string safeString(const std::string& s)
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

        //Check for spaces at end
        if(ret[ret.length() - 1] == ' ')
            ret.erase(ret.end() - 1, ret.end());

        return ret;
    }

    std::string getInfoString(data::user& u, data::titledata& d)
    {
        std::string ret = d.getTitle();

        char id[18];
        sprintf(id, " %016lX", d.getID());
        ret += id;

        switch(d.getType())
        {
            case FsSaveDataType_SystemSaveData:
                ret += " System Save";
                break;

            case FsSaveDataType_SaveData:
                ret += " Save Data";
                break;

            case FsSaveDataType_BcatDeliveryCacheStorage:
                ret += " Bcat Delivery Cache";
                break;

            case FsSaveDataType_DeviceSaveData:
                ret += " Device Save";
                break;

            case FsSaveDataType_TemporaryStorage:
                ret = " Temp Storage";
                break;

            case FsSaveDataType_CacheStorage:
                ret+= " Cache Storage";
                break;
        }

        ret += "\n" + u.getUsername();

        return ret;
    }
}
