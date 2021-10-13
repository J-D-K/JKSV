#include <switch.h>
#include <string>

#include "fs.h"
#include "cfg.h"
#include "util.h"

static std::string wd = "sdmc:/JKSV/";

static FSFILE *debLog;

static FsFileSystem sv;

void fs::init()
{
    mkDirRec("sdmc:/config/JKSV/");
    mkDirRec(wd);
    mkdir(std::string(wd + "_TRASH_").c_str(), 777);

    fs::logOpen();
}

bool fs::mountSave(const FsSaveDataInfo& _m)
{
    Result svOpen;
    FsSaveDataAttribute attr = {0};
    switch(_m.save_data_type)
    {
        case FsSaveDataType_System:
        case FsSaveDataType_SystemBcat:
            {
                attr.uid = _m.uid;
                attr.system_save_data_id = _m.system_save_data_id;
                attr.save_data_type = _m.save_data_type;
                svOpen = fsOpenSaveDataFileSystemBySystemSaveDataId(&sv, (FsSaveDataSpaceId)_m.save_data_space_id, &attr);
            }
            break;

        case FsSaveDataType_Account:
            {
                attr.uid = _m.uid;
                attr.application_id = _m.application_id;
                attr.save_data_type = _m.save_data_type;
                attr.save_data_rank = _m.save_data_rank;
                attr.save_data_index = _m.save_data_index;
                svOpen = fsOpenSaveDataFileSystem(&sv, (FsSaveDataSpaceId)_m.save_data_space_id, &attr);
            }
            break;

        case FsSaveDataType_Device:
            {
                attr.application_id = _m.application_id;
                attr.save_data_type = FsSaveDataType_Device;
                svOpen = fsOpenSaveDataFileSystem(&sv, (FsSaveDataSpaceId)_m.save_data_space_id, &attr);
            }
            break;

        case FsSaveDataType_Bcat:
            {
                attr.application_id = _m.application_id;
                attr.save_data_type = FsSaveDataType_Bcat;
                svOpen = fsOpenSaveDataFileSystem(&sv, (FsSaveDataSpaceId)_m.save_data_space_id, &attr);
            }
            break;

        case FsSaveDataType_Cache:
            {
                attr.application_id = _m.application_id;
                attr.save_data_type = FsSaveDataType_Cache;
                attr.save_data_index = _m.save_data_index;
                svOpen = fsOpenSaveDataFileSystem(&sv, (FsSaveDataSpaceId)_m.save_data_space_id, &attr);
            }
            break;

        case FsSaveDataType_Temporary:
            {
                attr.application_id = _m.application_id;
                attr.save_data_type = _m.save_data_type;
                svOpen = fsOpenSaveDataFileSystem(&sv, (FsSaveDataSpaceId)_m.save_data_space_id, &attr);
            }
            break;

        default:
            svOpen = 1;
            break;
    }

    return R_SUCCEEDED(svOpen) && fsdevMountDevice("sv", sv) != -1;
}

bool fs::commitToDevice(const std::string& dev)
{
    bool ret = true;
    Result res = fsdevCommitDevice(dev.c_str());
    if(R_FAILED(res))
    {
        fs::logWrite("Error committing file to device -> 0x%X\n", res);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popErrorCommittingFile", 0));
        ret = false;
    }
    return ret;
}

std::string fs::getWorkDir() { return wd; }

void fs::setWorkDir(const std::string& _w) { wd = _w; }

void fs::createSaveData(FsSaveDataType _type, uint64_t _tid, AccountUid _uid, threadInfo *t)
{
    data::titleInfo *tinfo = data::getTitleInfoByTID(_tid);
    if(t)
        t->status->setStatus(ui::getUICString("threadStatusCreatingSaveData", 0), tinfo->title.c_str());

    uint16_t cacheIndex = 0;
    std::string indexStr;
    if(_type == FsSaveDataType_Cache && !(indexStr = util::getStringInput(SwkbdType_NumPad, "0", ui::getUIString("swkbdSaveIndex", 0), 2, 0, NULL)).empty())
        cacheIndex = strtoul(indexStr.c_str(), NULL, 10);
    else if(_type == FsSaveDataType_Cache && indexStr.empty())
    {
        if(t)
            t->finished = true;
        return;
    }

    FsSaveDataAttribute attr;
    memset(&attr, 0, sizeof(FsSaveDataAttribute));
    attr.application_id = _tid;
    attr.uid = _uid;
    attr.system_save_data_id = 0;
    attr.save_data_type = _type;
    attr.save_data_rank = 0;
    attr.save_data_index = cacheIndex;

    FsSaveDataCreationInfo crt;
    memset(&crt, 0, sizeof(FsSaveDataCreationInfo));
    int64_t saveSize = 0, journalSize = 0;
    switch(_type)
    {
        case FsSaveDataType_Account:
            saveSize = tinfo->nacp.user_account_save_data_size;
            journalSize = tinfo->nacp.user_account_save_data_journal_size;
            break;

        case FsSaveDataType_Device:
            saveSize = tinfo->nacp.device_save_data_size;
            journalSize = tinfo->nacp.device_save_data_journal_size;
            break;

        case FsSaveDataType_Bcat:
            saveSize = tinfo->nacp.bcat_delivery_cache_storage_size;
            journalSize = tinfo->nacp.bcat_delivery_cache_storage_size;
            break;

        case FsSaveDataType_Cache:
            saveSize = 32 * 1024 * 1024;
            if(tinfo->nacp.cache_storage_journal_size > tinfo->nacp.cache_storage_data_and_journal_size_max)
                journalSize = tinfo->nacp.cache_storage_journal_size;
            else
                journalSize = tinfo->nacp.cache_storage_data_and_journal_size_max;
            break;

        default:
            if(t)
                t->finished = true;
            return;
            break;
    }
    crt.save_data_size = saveSize;
    crt.journal_size = journalSize;
    crt.available_size = 0x4000;
    crt.owner_id = _type == FsSaveDataType_Bcat ? 0x010000000000000C : tinfo->nacp.save_data_owner_id;
    crt.flags = 0;
    crt.save_data_space_id = FsSaveDataSpaceId_User;

    FsSaveDataMetaInfo meta;
    memset(&meta, 0, sizeof(FsSaveDataMetaInfo));
    if(_type != FsSaveDataType_Bcat)
    {
        meta.size = 0x40060;
        meta.type = FsSaveDataMetaType_Thumbnail;
    }

    Result res = 0;
    if(R_SUCCEEDED(res = fsCreateSaveDataFileSystem(&attr, &crt, &meta)))
    {
        util::createTitleDirectoryByTID(_tid);
        data::loadUsersTitles(false);
        ui::ttlRefresh();
    }
    else
    {
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataCreationFailed", 0));
        fs::logWrite("SaveCreate Failed -> %X\n", res);
    }
}

static void createSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::svCreateArgs *crt = (fs::svCreateArgs *)t->argPtr;
    fs::createSaveData(crt->type, crt->tid, crt->account, t);
    delete crt;
    t->finished = true;
}

void fs::createSaveDataThreaded(FsSaveDataType _type, uint64_t _tid, AccountUid _uid)
{
    fs::svCreateArgs *send = new fs::svCreateArgs;
    send->type = _type;
    send->tid = _tid;
    send->account = _uid;
    ui::newThread(createSaveData_t, send, NULL);
}

bool fs::extendSaveData(const data::userTitleInfo *tinfo, uint64_t extSize, threadInfo *t)
{
    if(t)
        t->status->setStatus(ui::getUICString("threadStatusExtendingSaveData", 0), data::getTitleNameByTID(tinfo->tid).c_str());

    uint64_t journal = fs::getJournalSizeMax(tinfo);
    uint64_t saveID  = tinfo->saveInfo.save_data_id;
    FsSaveDataSpaceId space = (FsSaveDataSpaceId)tinfo->saveInfo.save_data_space_id;
    Result res = 0;
    if(R_FAILED((res = fsExtendSaveDataFileSystem(space, saveID, extSize, journal))))
    {
        int64_t totalSize = 0;
        fs::mountSave(tinfo->saveInfo);
        fsFsGetTotalSpace(fsdevGetDeviceFileSystem("sv"), "/", &totalSize);
        fs::unmountSave();

        fs::logWrite("Extend Failed: %uMB to %uMB -> %X\n", totalSize / 1024 / 1024, extSize / 1024 / 1024, res);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataExtendFailed", 0));
        return false;
    }
    return true;
}

static void extendSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::svExtendArgs *e = (fs::svExtendArgs *)t->argPtr;
    fs::extendSaveData(e->tinfo, e->extSize, t);
    delete e;
    t->finished = true;
}

void fs::extendSaveDataThreaded(const data::userTitleInfo *tinfo, uint64_t extSize)
{
    fs::svExtendArgs *send = new fs::svExtendArgs;
    send->tinfo = tinfo;
    send->extSize = extSize;
    ui::newThread(extendSaveData_t, send, NULL);
}

uint64_t fs::getJournalSize(const data::userTitleInfo *tinfo)
{
    uint64_t ret = 0;
    data::titleInfo *t = data::getTitleInfoByTID(tinfo->tid);
    switch(tinfo->saveInfo.save_data_type)
    {
        case FsSaveDataType_Account:
            ret = t->nacp.user_account_save_data_journal_size;
            break;

        case FsSaveDataType_Device:
            ret = t->nacp.device_save_data_journal_size;
            break;

        case FsSaveDataType_Bcat:
            ret = t->nacp.bcat_delivery_cache_storage_size;
            break;

        case FsSaveDataType_Cache:
            if(t->nacp.cache_storage_journal_size > 0)
                ret = t->nacp.cache_storage_journal_size;
            else
                ret = t->nacp.cache_storage_data_and_journal_size_max;
            break;

        default:
            ret = BUFF_SIZE;
            break;
    }
    return ret;
}

uint64_t fs::getJournalSizeMax(const data::userTitleInfo *tinfo)
{
    uint64_t ret = 0;
    data::titleInfo *extend = data::getTitleInfoByTID(tinfo->tid);
    switch(tinfo->saveInfo.save_data_type)
    {
        case FsSaveDataType_Account:
            if(extend->nacp.user_account_save_data_journal_size_max > extend->nacp.user_account_save_data_journal_size)
                ret = extend->nacp.user_account_save_data_journal_size_max;
            else
                ret = extend->nacp.user_account_save_data_journal_size;
            break;

        case FsSaveDataType_Bcat:
            ret = extend->nacp.bcat_delivery_cache_storage_size;
            break;

        case FsSaveDataType_Cache:
            if(extend->nacp.cache_storage_data_and_journal_size_max > extend->nacp.cache_storage_journal_size)
                ret = extend->nacp.cache_storage_data_and_journal_size_max;
            else
                ret = extend->nacp.cache_storage_journal_size;
            break;

        case FsSaveDataType_Device:
            if(extend->nacp.device_save_data_journal_size_max > extend->nacp.device_save_data_journal_size)
                ret = extend->nacp.device_save_data_journal_size_max;
            else
                ret = extend->nacp.device_save_data_journal_size;
            break;

        default:
            //will just fail
            ret = 0;
            break;
    }
    return ret;
}

static void wipeSave_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(ui::getUICString("threadStatusResettingSaveData", 0));
    fs::delDir("sv:/");
    fs::commitToDevice("sv");
    t->finished = true;
}

void fs::wipeSave()
{
    ui::newThread(wipeSave_t, NULL, NULL);
}

void fs::createNewBackup(void *a)
{
    if(!fs::dirNotEmpty("sv:/"))
    {
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popSaveIsEmpty", 0));
        return;
    }

    uint64_t held = ui::padKeysHeld();

    data::user *u = data::getCurrentUser();
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    data::titleInfo *t = data::getTitleInfoByTID(d->tid);

    std::string out;

    if(held & HidNpadButton_R || cfg::config["autoName"])
        out = u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
    else if(held & HidNpadButton_L)
        out = u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YDM);
    else if(held & HidNpadButton_ZL)
        out = u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_HOYSTE);
    else
    {
        const std::string dict[] =
        {
            util::getDateTime(util::DATE_FMT_YMD),
            util::getDateTime(util::DATE_FMT_YDM),
            util::getDateTime(util::DATE_FMT_HOYSTE),
            util::getDateTime(util::DATE_FMT_JHK),
            util::getDateTime(util::DATE_FMT_ASC),
            u->getUsernameSafe(),
            t->safeTitle,
            util::generateAbbrev(d->tid),
            ".zip"
        };
        std::string defaultText = u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
        out = util::getStringInput(SwkbdType_QWERTY, defaultText, ui::getUIString("swkbdEnterName", 0), 64, 9, dict);
        out = util::safeString(out);
    }

    if(!out.empty())
    {
        std::string ext = util::getExtensionFromString(out);
        std::string path = util::generatePathByTID(d->tid) + out;
        if(cfg::config["zip"] || ext == "zip")
        {
            if(ext != "zip")//data::zip is on but extension is not zip
                path += ".zip";

            zipFile zip = zipOpen64(path.c_str(), 0);
            fs::copyDirToZipThreaded("sv:/", zip, false, 0);

        }
        else
        {
            fs::mkDir(path);
            path += "/";
            fs::copyDirToDirThreaded("sv:/", path);
        }
        ui::populateFldMenu();
    }
}

void fs::overwriteBackup(void *a)
{
    threadInfo *t = (threadInfo *)a;
    std::string *dst = (std::string *)t->argPtr;
    bool saveHasFiles = fs::dirNotEmpty("sv:/");
    if(fs::isDir(*dst) && saveHasFiles)
    {
        fs::delDir(*dst);
        fs::mkDir(*dst);
        dst->append("/");
        fs::copyDirToDirThreaded("sv:/", *dst);
    }
    else if(!fs::isDir(*dst) && util::getExtensionFromString(*dst) == "zip" && saveHasFiles)
    {
        fs::delfile(*dst);
        zipFile zip = zipOpen64(dst->c_str(), 0);
        fs::copyDirToZipThreaded("sv:/", zip, false, 0);
    }
    delete dst;
    t->finished = true;
}

void fs::restoreBackup(void *a)
{
    threadInfo *t = (threadInfo *)a;
    std::string *restore = (std::string *)t->argPtr;
    data::user *u = data::getCurrentUser();
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    if((utinfo->saveInfo.save_data_type != FsSaveDataType_System || cfg::config["sysSaveWrite"]))
    {
        bool saveHasFiles = fs::dirNotEmpty("sv:/");
        if(cfg::config["autoBack"] && cfg::config["zip"] && saveHasFiles)
        {
            std::string autoZip = util::generatePathByTID(utinfo->tid) + "/AUTO " + u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + ".zip";
            zipFile zip = zipOpen64(autoZip.c_str(), 0);
            fs::copyDirToZipThreaded("sv:/", zip, false, 0);
        }
        else if(cfg::config["autoBack"] && saveHasFiles)
        {
            std::string autoFolder = util::generatePathByTID(utinfo->tid) + "/AUTO - " + u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + "/";
            fs::mkDir(autoFolder.substr(0, autoFolder.length() - 1));
            fs::copyDirToDirThreaded("sv:/", autoFolder);
        }

        if(fs::isDir(*restore))
        {
            restore->append("/");
            if(fs::dirNotEmpty(*restore))
            {
                t->status->setStatus(ui::getUICString("threadStatusCalculatingSaveSize", 0));
                unsigned dirCount = 0, fileCount = 0;
                uint64_t saveSize = 0;
                int64_t  availSize = 0;
                fs::getDirProps(*restore, dirCount, fileCount, saveSize);
                fsFsGetTotalSpace(fsdevGetDeviceFileSystem("sv"), "/", &availSize);
                if((int)saveSize > availSize)
                {
                    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
                    fs::unmountSave();
                    fs::extendSaveData(utinfo, saveSize + 0x500000, t);
                    fs::mountSave(utinfo->saveInfo);
                }

                fs::wipeSave();
                fs::copyDirToDirCommitThreaded(*restore, "sv:/", "sv");
            }
            else
                ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popFolderIsEmpty", 0));
        }
        else if(!fs::isDir(*restore) && util::getExtensionFromString(*restore) == "zip")
        {
            unzFile unz = unzOpen64(restore->c_str());
            if(unz && fs::zipNotEmpty(unz))
            {
                t->status->setStatus(ui::getUICString("threadStatusCalculatingSaveSize", 0));
                uint64_t saveSize = fs::getZipTotalSize(unz);
                int64_t  availSize  = 0;
                fsFsGetTotalSpace(fsdevGetDeviceFileSystem("sv"), "/", &availSize);
                if((int)saveSize > availSize)
                {
                    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
                    fs::unmountSave();
                    fs::extendSaveData(utinfo, saveSize + 0x500000, t);
                    fs::mountSave(utinfo->saveInfo);
                }

                fs::wipeSave();
                fs::copyZipToDirThreaded(unz, "sv:/", "sv");
            }
            else
            {
                ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popZipIsEmpty", 0));
                unzClose(unz);
            }
        }
        else
        {
            std::string dstPath = "sv:/" + util::getFilenameFromPath(*restore);
            fs::copyFileCommitThreaded(*restore, dstPath, "sv");
        }
    }
    if(cfg::config["autoBack"])
        ui::populateFldMenu();

    delete restore;
    t->finished = true;
}

void fs::deleteBackup(void *a)
{
    threadInfo *t = (threadInfo *)a;
    std::string *deletePath = (std::string *)t->argPtr;
    std::string backupName = util::getFilenameFromPath(*deletePath);

    t->status->setStatus(ui::getUICString("threadStatusDeletingFile", 0));
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    if(cfg::config["trashBin"])
    {
        std::string oldPath = *deletePath;
        std::string trashPath = wd + "_TRASH_/" + data::getTitleSafeNameByTID(utinfo->tid);
        fs::mkDir(trashPath);
        trashPath += "/" + backupName;

        rename(oldPath.c_str(), trashPath.c_str());
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataBackupMovedToTrash", 0), backupName.c_str());
    }
    else if(fs::isDir(*deletePath))
    {
        *deletePath += "/";
        fs::delDir(*deletePath);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataBackupDeleted", 0), backupName.c_str());
    }
    else
    {
        fs::delfile(*deletePath);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataBackupDeleted", 0), backupName.c_str());
    }
    ui::populateFldMenu();
    delete deletePath;
    t->finished = true;
}

void fs::logOpen()
{
    std::string logPath = wd + "log.txt";
    debLog = fsfopen(logPath.c_str(), FsOpenMode_Write);
    fsfclose(debLog);
}

void fs::logWrite(const char *fmt, ...)
{
    std::string logPath = wd + "log.txt";
    debLog = fsfopen(logPath.c_str(), FsOpenMode_Append | FsOpenMode_Write);
    char tmp[256];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);
    fsfwrite(tmp, 1, strlen(tmp), debLog);
    fsfclose(debLog);
}

