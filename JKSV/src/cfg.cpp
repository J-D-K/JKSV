#include <switch.h>
#include <string>
#include <unordered_map>
#include <json-c/json.h>

#include "cfg.h"
#include "data.h"
#include "file.h"
#include "ui.h"
#include "util.h"
#include "type.h"

std::unordered_map<std::string, bool> cfg::config;
std::vector<uint64_t> cfg::blacklist;
std::vector<uint64_t> cfg::favorites;
static std::unordered_map<uint64_t, std::string> pathDefs;
uint8_t cfg::sortType;
std::string cfg::driveClientID, cfg::driveClientSecret, cfg::driveRefreshToken;
std::string cfg::webdavOrigin, cfg::webdavBasePath, cfg::webdavUser, cfg::webdavPassword;


const char *cfgPath = "sdmc:/config/JKSV/JKSV.cfg", *titleDefPath = "sdmc:/config/JKSV/titleDefs.txt", *workDirLegacy = "sdmc:/switch/jksv_dir.txt";
static std::unordered_map<std::string, unsigned> cfgStrings =
{
    {"workDir", 0}, {"includeDeviceSaves", 1}, {"autoBackup", 2}, {"overclock", 3}, {"holdToDelete", 4}, {"holdToRestore", 5},
    {"holdToOverwrite", 6}, {"forceMount", 7}, {"accountSystemSaves", 8}, {"allowSystemSaveWrite", 9}, {"directFSCommands", 10},
    {"exportToZIP", 11}, {"languageOverride", 12}, {"enableTrashBin", 13}, {"titleSortType", 14}, {"animationScale", 15},
    {"favorite", 16}, {"blacklist", 17}, {"autoName", 18}, {"driveRefreshToken", 19},
};

const std::string _true_ = "true", _false_ = "false";

bool cfg::isBlacklisted(const uint64_t& tid)
{
    for(uint64_t& bid : cfg::blacklist)
        if(tid == bid) return true;

    return false;
}

//Has to be threaded to be compatible with ui::confirm
void cfg::addTitleToBlacklist(void *a)
{
    threadInfo *t = (threadInfo *)a;
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    uint64_t tid = d->tid;
    cfg::blacklist.push_back(tid);
    for(data::user& u : data::users)
    {
        for(unsigned i = 0; i < u.titleInfo.size(); i++)
            if(u.titleInfo[i].tid == tid) u.titleInfo.erase(u.titleInfo.begin() + i);
    }
    ui::ttlRefresh();
    t->finished = true;
}

void cfg::removeTitleFromBlacklist(const uint64_t& tid)
{
    for(unsigned i = 0; i < cfg::blacklist.size(); i++)
    {
        if(cfg::blacklist[i] == tid)
            cfg::blacklist.erase(cfg::blacklist.begin() + i);
    }
    data::loadUsersTitles(false);
    ui::ttlRefresh();
}

bool cfg::isFavorite(const uint64_t& tid)
{
    for(uint64_t& fid : cfg::favorites)
        if(tid == fid) return true;

    return false;
}

static int getFavoriteIndex(const uint64_t& tid)
{
    for(unsigned i = 0; i < cfg::favorites.size(); i++)
    {
        if(tid == cfg::favorites[i])
            return i;
    }
    return -1;
}

void cfg::addTitleToFavorites(const uint64_t& tid)
{
    if(cfg::isFavorite(tid))
    {
        int rem = getFavoriteIndex(tid);
        cfg::favorites.erase(cfg::favorites.begin() + rem);
    }
    else
        cfg::favorites.push_back(tid);

    data::sortUserTitles();
    ui::ttlRefresh();
}

bool cfg::isDefined(const uint64_t& tid)
{
    for(auto& def : pathDefs)
        if(def.first == tid) return true;

    return false;
}

void cfg::pathDefAdd(const uint64_t& tid, const std::string& newPath)
{
    data::titleInfo *t = data::getTitleInfoByTID(tid);
    std::string oldSafe = t->safeTitle;
    std::string tmp = util::safeString(newPath);
    if(!tmp.empty())
    {
        pathDefs[tid] = tmp;
        t->safeTitle = tmp;

        std::string oldOutput = fs::getWorkDir() + oldSafe;
        std::string newOutput = fs::getWorkDir() + tmp;
        rename(oldOutput.c_str(), newOutput.c_str());

        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popChangeOutputFolder", 0), oldSafe.c_str(), tmp.c_str());
    }
    else
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popChangeOutputError", 0), newPath.c_str());
}

std::string cfg::getPathDefinition(const uint64_t& tid)
{
    return pathDefs[tid];
}

void cfg::addPathToFilter(const uint64_t& tid, const std::string& _p)
{
    char outpath[128];
    sprintf(outpath, "sdmc:/config/JKSV/0x%016lX_filter.txt", tid);
    FILE *filters = fopen(outpath, "a");
    fprintf(filters, "%s\n", _p.c_str());
    fclose(filters);
}

void cfg::resetConfig()
{
    cfg::config["incDev"] = false;
    cfg::config["autoBack"] = true;
    cfg::config["ovrClk"] = false;
    cfg::config["holdDel"] = true;
    cfg::config["holdRest"] = true;
    cfg::config["holdOver"] = true;
    cfg::config["forceMount"] = true;
    cfg::config["accSysSave"] = false;
    cfg::config["sysSaveWrite"] = false;
    cfg::config["directFsCmd"] = false;
    cfg::config["zip"] = false;
    cfg::config["langOverride"] = false;
    cfg::config["trashBin"] = true;
    cfg::config["autoName"] = false;
    cfg::sortType = cfg::ALPHA;
    ui::animScale = 3.0f;
    cfg::config["autoUpload"] = false;
}

static inline bool textToBool(const std::string& _txt)
{
    return _txt == _true_ ? true : false;
}

static inline std::string boolToText(bool _b)
{
    return _b ? _true_ : _false_;
}

static inline std::string sortTypeText()
{
    switch(cfg::sortType)
    {
        case cfg::ALPHA:
            return "ALPHA";
            break;

        case cfg::MOST_PLAYED:
            return "MOST_PLAYED";
            break;

        case cfg::LAST_PLAYED:
            return "LAST_PLAYED";
            break;
    }
    return "";
}

static void loadWorkDirLegacy()
{
    if(fs::fileExists(workDirLegacy))
    {
        char tmp[256];
        memset(tmp, 0, 256);

        FILE *getDir = fopen(workDirLegacy, "r");
        fgets(tmp, 256, getDir);
        fclose(getDir);
        std::string tmpStr = tmp;
        util::stripChar('\n', tmpStr);
        util::stripChar('\r', tmpStr);
        fs::setWorkDir(tmpStr);
        fs::delfile(workDirLegacy);
    }
}

static void loadConfigLegacy()
{
    std::string legacyCfgPath = fs::getWorkDir() + "cfg.bin";
    if(fs::fileExists(legacyCfgPath))
    {
        FILE *oldCfg = fopen(legacyCfgPath.c_str(), "rb");
        uint64_t cfgIn = 0;
        fread(&cfgIn, sizeof(uint64_t), 1, oldCfg);
        fread(&cfg::sortType, 1, 1, oldCfg);
        fread(&ui::animScale, sizeof(float), 1, oldCfg);
        if(ui::animScale == 0)
            ui::animScale = 3.0f;
        fclose(oldCfg);

        cfg::config["incDev"] = cfgIn >> 63 & 1;
        cfg::config["autoBack"] = cfgIn >> 62 & 1;
        cfg::config["ovrClk"] = cfgIn >> 61 & 1;
        cfg::config["holdDel"] = cfgIn >> 60 & 1;
        cfg::config["holdRest"] = cfgIn >> 59 & 1;
        cfg::config["holdOver"] = cfgIn >> 58 & 1;
        cfg::config["forceMount"] = cfgIn >> 57 & 1;
        cfg::config["accSysSave"] = cfgIn >> 56 & 1;
        cfg::config["sysSaveWrite"] = cfgIn >> 55 & 1;
        cfg::config["directFsCmd"] = cfgIn >> 53 & 1;
        cfg::config["zip"] = cfgIn >> 51 & 1;
        cfg::config["langOverride"] = cfgIn >> 50 & 1;
        cfg::config["trashBin"] = cfgIn >> 49 & 1;
        fs::delfile(legacyCfgPath.c_str());
    }
}

static void loadFavoritesLegacy()
{
    std::string legacyFavPath = fs::getWorkDir() + "favorites.txt";
    if(fs::fileExists(legacyFavPath))
    {
        fs::dataFile fav(legacyFavPath);
        while(fav.readNextLine(false))
            cfg::favorites.push_back(strtoul(fav.getLine().c_str(), NULL, 16));
        fav.close();
        fs::delfile(legacyFavPath);
    }
}

static void loadBlacklistLegacy()
{
    std::string legacyBlPath = fs::getWorkDir() + "blacklist.txt";
    if(fs::fileExists(legacyBlPath))
    {
        fs::dataFile bl(legacyBlPath);
        while(bl.readNextLine(false))
            cfg::blacklist.push_back(strtoul(bl.getLine().c_str(), NULL, 16));
        bl.close();
        fs::delfile(legacyBlPath);
    }
}

static void loadTitleDefsLegacy()
{
    std::string titleDefLegacy = fs::getWorkDir() + "titleDefs.txt";
    if(fs::fileExists(titleDefLegacy))
    {
        fs::dataFile getPaths(titleDefLegacy);
        while(getPaths.readNextLine(true))
        {
            uint64_t tid = strtoul(getPaths.getName().c_str(), NULL, 16);
            pathDefs[tid] = getPaths.getNextValueStr();
        }
        getPaths.close();
        fs::delfile(titleDefLegacy);
    }
}

//Oops
static void loadTitleDefs()
{
    if(fs::fileExists(titleDefPath))
    {
        fs::dataFile getPaths(titleDefPath);
        while(getPaths.readNextLine(true))
        {
            uint64_t tid = strtoul(getPaths.getName().c_str(), NULL, 16);
            pathDefs[tid] = getPaths.getNextValueStr();
        }
    }
}

static void loadDriveConfig()
{
    // Start Google Drive
    fs::dirList cfgList("/config/JKSV/", true);
    std::string clientSecretPath;
    for(unsigned i = 0; i < cfgList.getCount(); i++)
    {
        std::string itemName = cfgList.getItem(i);
        if(itemName.find("client_secret") != itemName.npos)
        {
            clientSecretPath = "/config/JKSV/" + cfgList.getItem(i);
            break;
        }
    }

    if(!clientSecretPath.empty())
    {
        json_object *installed, *clientID, *clientSecret, *driveJSON = json_object_from_file(clientSecretPath.c_str());
        if (driveJSON) 
        {
            if(json_object_object_get_ex(driveJSON, "installed", &installed))
            {
                if(json_object_object_get_ex(installed, "client_id", &clientID) 
                    && json_object_object_get_ex(installed, "client_secret", &clientSecret))
                {
                    cfg::driveClientID = json_object_get_string(clientID);
                    cfg::driveClientSecret = json_object_get_string(clientSecret);
                }
            }
            json_object_put(driveJSON);
        }
    }
    // End Google Drive

    // Webdav
    json_object *webdavJSON = json_object_from_file("/config/JKSV/webdav.json");
    json_object *origin, *basepath, *username, *password;
    if (webdavJSON)
    {
        if (json_object_object_get_ex(webdavJSON, "origin", &origin)) {
            cfg::webdavOrigin = json_object_get_string(origin);
        }
        if (json_object_object_get_ex(webdavJSON, "basepath", &basepath)) {
            cfg::webdavBasePath = json_object_get_string(basepath);
        }
        if (json_object_object_get_ex(webdavJSON, "username", &username)) {
            cfg::webdavUser = json_object_get_string(username);
        }
        if (json_object_object_get_ex(webdavJSON, "password", &password)) {
            cfg::webdavPassword = json_object_get_string(password);
        }
    }
}

void cfg::loadConfig()
{
    cfg::resetConfig();
    loadWorkDirLegacy();
    loadConfigLegacy();
    loadFavoritesLegacy();
    loadBlacklistLegacy();
    loadTitleDefsLegacy();
    loadTitleDefs();

    if(fs::fileExists(cfgPath))
    {
        fs::dataFile cfgRead(cfgPath);
        while(cfgRead.readNextLine(true))
        {
            std::string varName = cfgRead.getName();
            if(cfgStrings.find(varName) != cfgStrings.end())
            {
                switch(cfgStrings[varName])
                {
                    case 0:
                        fs::setWorkDir(cfgRead.getNextValueStr());
                        break;

                    case 1:
                        cfg::config["incDev"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 2:
                        cfg::config["autoBack"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 3:
                        cfg::config["ovrClk"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 4:
                        cfg::config["holdDel"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 5:
                        cfg::config["holdRest"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 6:
                        cfg::config["holdOver"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 7:
                        cfg::config["forceMount"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 8:
                        cfg::config["accSysSave"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 9:
                        cfg::config["sysSaveWrite"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 10:
                        cfg::config["directFsCmd"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 11:
                        cfg::config["zip"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 12:
                        cfg::config["langOverride"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 13:
                        cfg::config["trashBin"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 14:
                        {
                            std::string getSort = cfgRead.getNextValueStr();
                            if(getSort == "ALPHA")
                                cfg::sortType = cfg::ALPHA;
                            else if(getSort == "MOST_PLAYED")
                                cfg::sortType = cfg::MOST_PLAYED;
                            else
                                cfg::sortType = cfg::LAST_PLAYED;
                        }
                        break;

                    case 15:
                        {
                            std::string animFloat = cfgRead.getNextValueStr();
                            ui::animScale = strtof(animFloat.c_str(), NULL);
                        }
                        break;

                    case 16:
                        {
                            std::string tid = cfgRead.getNextValueStr();
                            cfg::favorites.push_back(strtoul(tid.c_str(), NULL, 16));
                        }
                        break;

                    case 17:
                        {
                            std::string tid = cfgRead.getNextValueStr();
                            cfg::blacklist.push_back(strtoul(tid.c_str(), NULL, 16));
                        }
                        break;

                    case 18:
                        cfg::config["autoName"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    case 19:
                        cfg::driveRefreshToken = cfgRead.getNextValueStr();
                        break;

                    case 20:
                        cfg::config["autoUpload"] = textToBool(cfgRead.getNextValueStr());
                        break;

                    default:
                        break;
                }
            }
        }
    }
    loadDriveConfig();
}

static void savePathDefs()
{
    FILE *pathOut = fopen(titleDefPath, "w");
    for(auto& p : pathDefs)
        fprintf(pathOut, "0x%016lX = \"%s\"\n", p.first, p.second.c_str());
    fclose(pathOut);
}

void cfg::saveConfig()
{
    FILE *cfgOut = fopen("sdmc:/config/JKSV/JKSV.cfg", "w");
    fprintf(cfgOut, "#JKSV config.\nworkDir = \"%s\"\n\n", fs::getWorkDir().c_str());
    fprintf(cfgOut, "includeDeviceSaves = %s\n", boolToText(cfg::config["incDev"]).c_str());
    fprintf(cfgOut, "autoBackup = %s\n", boolToText(cfg::config["autoBack"]).c_str());
    fprintf(cfgOut, "autoName = %s\n", boolToText(cfg::config["autoName"]).c_str());
    fprintf(cfgOut, "overclock = %s\n", boolToText(cfg::config["ovrClk"]).c_str());
    fprintf(cfgOut, "holdToDelete = %s\n", boolToText(cfg::config["holdDel"]).c_str());
    fprintf(cfgOut, "holdToRestore = %s\n", boolToText(cfg::config["holdRest"]).c_str());
    fprintf(cfgOut, "holdToOverwrite = %s\n", boolToText(cfg::config["holdOver"]).c_str());
    fprintf(cfgOut, "forceMount = %s\n", boolToText(cfg::config["forceMount"]).c_str());
    fprintf(cfgOut, "accountSystemSaves = %s\n", boolToText(cfg::config["accSysSaves"]).c_str());
    fprintf(cfgOut, "allowSystemSaveWrite = %s\n", boolToText(cfg::config["sysSaveWrite"]).c_str());
    fprintf(cfgOut, "directFSCommands = %s\n", boolToText(cfg::config["directFsCmd"]).c_str());
    fprintf(cfgOut, "exportToZIP = %s\n", boolToText(cfg::config["zip"]).c_str());
    fprintf(cfgOut, "languageOverride = %s\n", boolToText(cfg::config["langOverride"]).c_str());
    fprintf(cfgOut, "enableTrashBin = %s\n", boolToText(cfg::config["trashBin"]).c_str());
    fprintf(cfgOut, "titleSortType = %s\n", sortTypeText().c_str());
    fprintf(cfgOut, "animationScale = %f\n", ui::animScale);
    fprintf(cfgOut, "autoUpload = %s\n", boolToText(cfg::config["autoUpload"]).c_str());

    if(!cfg::driveRefreshToken.empty())
        fprintf(cfgOut, "driveRefreshToken = %s\n", cfg::driveRefreshToken.c_str());

    if(!cfg::favorites.empty())
    {
        fprintf(cfgOut, "\n#favorites\n");
        for(uint64_t& f : cfg::favorites)
            fprintf(cfgOut, "favorite = 0x%016lX\n", f);
    }

    if(!cfg::blacklist.empty())
    {
        fprintf(cfgOut, "\n#blacklist\n");
        for(uint64_t& b : cfg::blacklist)
            fprintf(cfgOut, "blacklist = 0x%016lX\n", b);
    }
    fclose(cfgOut);

    if(!pathDefs.empty())
        savePathDefs();
}
