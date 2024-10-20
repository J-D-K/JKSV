#pragma once

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

    typedef enum
    {
        GPU_SPEED_0MHz = 0,
        GPU_SPEED_76MHz = 76800000,
        GPU_SPEED_153MHz = 153600000,
        GPU_SPEED_203MHz = 230400000,
        GPU_SPEED_307MHz = 307200000, //Handheld 1
        GPU_SPEED_384MHz = 384000000, //Handheld 2
        GPU_SPEED_460MHz = 460800000,
        GPU_SPEED_537MHz = 537600000,
        GPU_SPEED_614MHz = 614400000,
        GPU_SPEED_768MHz = 768000000, //Docked
        GPU_SPEED_844MHz = 844800000,
        GPU_SPEED_921MHZ = 921600000
    } gpuSpds;

    typedef enum
    {
        RAM_SPEED_0MHz = 0,
        RAM_SPEED_40MHz = 40800000,
        RAM_SPEED_68MHz = 68000000,
        RAM_SPEED_102MHz = 102000000,
        RAM_SPEED_204MHz = 204000000,
        RAM_SPEED_408MHz = 408000000,
        RAM_SPEED_665MHz = 665600000,
        RAM_SPEED_800MHz = 800000000,
        RAM_SPEED_1065MHz = 1065600000,
        RAM_SPEED_1331MHz = 1331200000,
        RAM_SPEED_1600MHz = 1600000000
    } ramSpds;

    //Returns string with date S+ time
    std::string getDateTime(int fmt);

    //Copys dir list to a menu with 'D: ' + 'F: '
    void copyDirListToMenu(const fs::dirList& d, ui::menu& m);

    //Removes last folder from '_path'
    void removeLastFolderFromString(std::string& _path);
    size_t getTotalPlacesInPath(const std::string& _path);
    void trimPath(std::string& _path, uint8_t _places);

    inline bool isASCII(const uint32_t& t)
    {
        return t > 30 && t < 127;
    }

    std::string safeString(const std::string& s);

    std::string getStringInput(SwkbdType _type, const std::string& def, const std::string& head, size_t maxLength, unsigned dictCnt, const std::string dictWords[]);

    std::string getExtensionFromString(const std::string& get);
    std::string getFilenameFromPath(const std::string& get);

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

    void sysBoost();
    void sysNormal();

    inline bool isApplet()
    {
        AppletType type = appletGetAppletType();
        return type == AppletType_LibraryApplet;
    }

    void checkForUpdate(void *a);
}
