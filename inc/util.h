#ifndef UTIL_H
#define UTIL_H

#include "data.h"
#include "ui.h"
#include "file.h"
#include "gfx.h"

namespace util
{
    enum
    {
        DATE_FMT_YMD,
        DATE_FMT_YDM,
        DATE_FMT_HOYSTE,
        DATE_FMT_JHK,
        DATE_FMT_ASC
    };

    typedef enum
    {
        CPU_SPEED_204MHz = 204000000,
        CPU_SPEED_306MHz = 306000000,
        CPU_SPEED_408MHz = 408000000,
        CPU_SPEED_510MHz = 510000000,
        CPU_SPEED_612MHz = 612000000,
        CPU_SPEED_714MHz = 714000000,
        CPU_SPEED_816MHz = 816000000,
        CPU_SPEED_918MHz = 918000000,
        CPU_SPEED_1020MHz = 1020000000, //Default
        CPU_SPEED_1122MHz = 1122000000,
        CPU_SPEED_1224MHz = 1224000000,
        CPU_SPEED_1326MHz = 1326000000,
        CPU_SPEED_1428MHz = 1428000000,
        CPU_SPEED_1581MHz = 1581000000,
        CPU_SPEED_1683MHz = 1683000000,
        CPU_SPEED_1785MHz = 1785000000
    } cpuSpds;

    //Returns string with date S+ time
    std::string getDateTime(int fmt);

    //Copys dir list to a menu with 'D: ' + 'F: '
    void copyDirListToMenu(const fs::dirList& d, ui::menu& m);

    //Removes last folder from '_path'
    void removeLastFolderFromString(std::string& _path);

    std::string safeString(const std::string& s);

    std::string getInfoString(data::user& u, const uint64_t& tid);

    std::string getStringInput(SwkbdType _type, const std::string& def, const std::string& head, size_t maxLength, unsigned dictCnt, const std::string dictWords[]);

    inline std::string getExtensionFromString(const std::string& get)
    {
        size_t ext = get.find_last_of('.');
        if(ext != get.npos)
            return get.substr(ext + 1, get.npos);
        else
            return "";
    }

    std::string generateAbbrev(const uint64_t& tid);

    //removes char from C++ string
    void stripChar(char _c, std::string& _s);

    void replaceStr(std::string& _str, const std::string& _find, const std::string& _rep);

    //For future external translation support. Replaces [button] with button chars
    void replaceButtonsInString(std::string& rep);

    //Creates a basic generic icon for stuff without one
    SDL_Texture *createIconGeneric(const char *txt, int fontSize, bool clearBack);

    inline u128 accountUIDToU128(AccountUid uid)
    {
        return ((u128)uid.uid[0] << 64 | uid.uid[1]);
    }

    inline AccountUid u128ToAccountUID(u128 id)
    {
        AccountUid ret;
        ret.uid[0] = id >> 64;
        ret.uid[1] = id;
        return ret;
    }

    inline std::string getIDStr(const uint64_t& _id)
    {
        char tmp[18];
        sprintf(tmp, "%016lX", _id);
        return std::string(tmp);
    }

    inline std::string getIDStrLower(const uint64_t& _id)
    {
        char tmp[18];
        sprintf(tmp, "%08X", (uint32_t)_id);
        return std::string(tmp);
    }

    inline std::string generatePathByTID(const uint64_t& tid)
    {
        return fs::getWorkDir() + data::getTitleSafeNameByTID(tid) + "/";
    }

    std::string getSizeString(const uint64_t& _size);

    inline void createTitleDirectoryByTID(const uint64_t& tid)
    {
        std::string makePath = fs::getWorkDir() + data::getTitleSafeNameByTID(tid);
        mkdir(makePath.c_str(), 777);
    }

    Result accountDeleteUser(AccountUid *uid);

    void setCPU(uint32_t hz);
    void checkForUpdate(void *a);
}
#endif // UTIL_H
