#include <cstdio>
#include <algorithm>
#include <cstring>
#include <vector>
#include <switch.h>
#include <unistd.h>
#include <cstdarg>
#include <sys/stat.h>

#include "file.h"
#include "util.h"
#include "ui.h"
#include "gfx.h"
#include "data.h"
#include "cfg.h"

static std::string wd = "sdmc:/JKSV/";

static std::vector<std::string> pathFilter;

static FSFILE *debLog;

static FsFileSystem sv;

fs::copyArgs *fs::copyArgsCreate(const std::string& from, const std::string& to, const std::string& dev, zipFile z, unzFile unz, bool _cleanup, bool _trimZipPath, uint8_t _trimPlaces)
{
    copyArgs *ret = new copyArgs;
    ret->to = to;
    ret->from = from;
    ret->dev = dev;
    ret->z = z;
    ret->unz = unz;
    ret->cleanup = _cleanup;
    ret->prog = new ui::progBar;
    ret->prog->setMax(0);
    ret->prog->update(0);
    ret->offset = 0;
    ret->trimZipPath = _trimZipPath;
    ret->trimZipPlaces = _trimPlaces;
    return ret;
}

void fs::copyArgsDestroy(copyArgs *c)
{
    delete c->prog;
    delete c;
    c = NULL;
}

static struct
{
    bool operator()(const fs::dirItem& a, const fs::dirItem& b)
    {
        if(a.isDir() != b.isDir())
            return a.isDir();

        for(unsigned i = 0; i < a.getItm().length(); i++)
        {
            char charA = tolower(a.getItm()[i]);
            char charB = tolower(b.getItm()[i]);
            if(charA != charB)
                return charA < charB;
        }
        return false;
    }
} sortDirList;

void fs::mkDir(const std::string& _p)
{
     if(cfg::config["directFsCmd"])
        fsMkDir(_p.c_str());
    else
        mkdir(_p.c_str(), 777);
}

void fs::mkDirRec(const std::string& _p)
{
    //skip first slash
    size_t pos = _p.find('/', 0) + 1;
    while((pos = _p.find('/', pos)) != _p.npos)
    {
        fs::mkDir(_p.substr(0, pos).c_str());
        ++pos;
    }
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

void fs::createSaveData(FsSaveDataType _type, uint64_t _tid, AccountUid _userID)
{
    std::string indexStr;
    uint16_t index = 0;
    if(_type == FsSaveDataType_Cache && !(indexStr = util::getStringInput(SwkbdType_NumPad, "0", ui::getUIString("swkbdSaveIndex", 0), 2, 0, NULL)).empty())
        index = strtoul(indexStr.c_str(), NULL, 10);
    else if(_type == FsSaveDataType_Cache && indexStr.empty())
        return;

    svCreateArgs *send = new svCreateArgs;
    send->type = _type;
    send->account = _userID;
    send->tid = _tid;
    send->index = index;
    ui::newThread(fs::createSaveData_t, send, NULL);
}

void fs::init()
{
    mkDirRec("sdmc:/config/JKSV/");
    mkDirRec(wd);
    mkdir(std::string(wd + "_TRASH_").c_str(), 777);

    fs::logOpen();
}

void fs::exit()
{

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

fs::dirItem::dirItem(const std::string& pathTo, const std::string& sItem)
{
    itm = sItem;

    std::string fullPath = pathTo + sItem;
    struct stat s;
    if(stat(fullPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
        dir = true;
}

std::string fs::dirItem::getName() const
{
    size_t extPos = itm.find_last_of('.'), slPos = itm.find_last_of('/');
    if(extPos == itm.npos)
        return "";

    return itm.substr(slPos + 1, extPos);
}

std::string fs::dirItem::getExt() const
{
    return util::getExtensionFromString(itm);
}

fs::dirList::dirList(const std::string& _path)
{
    path = _path;
    d = opendir(path.c_str());

    while((ent = readdir(d)))
        item.emplace_back(path, ent->d_name);

    closedir(d);

    std::sort(item.begin(), item.end(), sortDirList);
}

void fs::dirList::reassign(const std::string& _path)
{
    path = _path;

    d = opendir(path.c_str());

    item.clear();

    while((ent = readdir(d)))
        item.emplace_back(path, ent->d_name);

    closedir(d);

    std::sort(item.begin(), item.end(), sortDirList);
}

void fs::dirList::rescan()
{
    item.clear();
    d = opendir(path.c_str());

    while((ent = readdir(d)))
        item.emplace_back(path, ent->d_name);

    closedir(d);

    std::sort(item.begin(), item.end(), sortDirList);
}

fs::dataFile::dataFile(const std::string& _path)
{
    f = fopen(_path.c_str(), "r");
    if(f != NULL)
        opened = true;
}

fs::dataFile::~dataFile()
{
    fclose(f);
}

bool fs::dataFile::readNextLine(bool proc)
{
    bool ret = false;
    char tmp[1024];
    while(fgets(tmp, 1024, f))
    {
        if(tmp[0] != '#' && tmp[0] != '\n' && tmp[0] != '\r')
        {
            line = tmp;
            ret = true;
            break;
        }
    }
    util::stripChar('\n', line);
    util::stripChar('\r', line);
    if(proc)
        procLine();

    return ret;
}

void fs::dataFile::procLine()
{
    size_t pPos = line.find_first_of("(=,");
    if(pPos != line.npos)
    {
        lPos = pPos;
        name.assign(line.begin(), line.begin() + lPos);
    }
    else
        name = line;

    util::stripChar(' ', name);
    ++lPos;
}

std::string fs::dataFile::getNextValueStr()
{
    std::string ret = "";
    //Skip all spaces until we hit actual text
    size_t pos1 = line.find_first_not_of(", ", lPos);
    //If reading from quotes
    if(line[pos1] == '"')
        lPos = line.find_first_of('"', ++pos1);
    else
        lPos = line.find_first_of(",;\n", pos1);//Set lPos to end of string we want. This should just set lPos to the end of the line if it fails, which is ok

    ret = line.substr(pos1, lPos++ - pos1);

    util::replaceStr(ret, "\\n", "\n");

    return ret;
}

int fs::dataFile::getNextValueInt()
{
    int ret = 0;
    std::string no = getNextValueStr();
    if(no[0] == '0' && tolower(no[1]) == 'x')
        ret = strtoul(no.c_str(), NULL, 16);
    else
        ret = strtoul(no.c_str(), NULL, 10);

    return ret;
}

//Wrapper functions to use functions from `fsthrd.cpp`
void fs::copyFile(const std::string& from, const std::string& to)
{
    copyArgs *send = copyArgsCreate(from, to, "", NULL, NULL, false, false, 0);
    ui::newThread(copyFile_t, send, _fileDrawFunc);
}

void fs::copyFileCommit(const std::string& from, const std::string& to, const std::string& dev)
{
    copyArgs *send = copyArgsCreate(from, to, dev, NULL, NULL, false, false, 0);
    ui::newThread(copyFileCommit_t, send, _fileDrawFunc);
}

void fs::copyDirToDir(const std::string& from, const std::string& to)
{
    fs::copyArgs *send = fs::copyArgsCreate(from, to, "", NULL, NULL, true, false, 0);
    ui::newThread(fs::copyDirToDir_t, send, _fileDrawFunc);
}

void fs::copyDirToZip(const std::string& from, zipFile to)
{
    if(cfg::config["ovrClk"])
    {
        util::setCPU(util::CPU_SPEED_1785MHz);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popCPUBoostEnabled", 0));
    }
    copyArgs *send = copyArgsCreate(from, "", "", to, NULL, true, false, 0);
    ui::newThread(copyDirToZip_t, send, _fileDrawFunc);
}

void fs::copyZipToDir(unzFile unz, const std::string& to, const std::string& dev)
{
    if(cfg::config["ovrClk"])
    {
        util::setCPU(util::CPU_SPEED_1785MHz);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popCPUBoostEnabled", 0));
    }
    copyArgs *send = copyArgsCreate("", to, dev, NULL, unz, true, false, 0);
    ui::newThread(copyZipToDir_t, send, _fileDrawFunc);
}

bool fs::dirNotEmpty(const std::string& _dir)
{
    fs::dirList tmp(_dir);
    return tmp.getCount() > 0;
}

bool fs::zipNotEmpty(unzFile unzip)
{
    return unzGoToFirstFile(unzip) == UNZ_OK;
}

void fs::copyDirToDirCommit(const std::string& from, const std::string& to, const std::string& dev)
{
    fs::copyArgs *send = copyArgsCreate(from, to, dev, NULL, NULL, true, false, 0);
    ui::newThread(copyDirToDirCommit_t, send, _fileDrawFunc);
}

void fs::delfile(const std::string& path)
{
    if(cfg::config["directFsCmd"])
        fsremove(path.c_str());
    else
        remove(path.c_str());
}

void fs::delDir(const std::string& path)
{
    dirList list(path);
    for(unsigned i = 0; i < list.getCount(); i++)
    {
        if(pathIsFiltered(path + list.getItem(i)))
            continue;

        if(list.isDir(i))
        {
            std::string newPath = path + list.getItem(i) + "/";
            delDir(newPath);

            std::string delPath = path + list.getItem(i);
            rmdir(delPath.c_str());
        }
        else
        {
            std::string delPath = path + list.getItem(i);
            std::remove(delPath.c_str());
        }
    }
    rmdir(path.c_str());
}

void fs::loadPathFilters(const uint64_t& tid)
{
    char path[256];
    sprintf(path, "sdmc:/config/JKSV/0x%016lX_filter.txt", tid);
    if(fs::fileExists(path))
    {
        fs::dataFile filter(path);
        while(filter.readNextLine(false))
            pathFilter.push_back(filter.getLine());
    }
}

bool fs::pathIsFiltered(const std::string& _path)
{
    if(pathFilter.empty())
        return false;

    for(std::string& _p : pathFilter)
    {
        if(_path == _p)
            return true;
    }

    return false;
}

void fs::freePathFilters()
{
    pathFilter.clear();
}

void fs::wipeSave()
{
    ui::newThread(fs::wipesave_t, NULL, NULL);
}

void fs::dumpAllUserSaves()
{
    //This is only really used for the progress bar
    fs::copyArgs *send = fs::copyArgsCreate("", "", "", NULL, NULL, true, false, 0);
    ui::newThread(fs::backupUserSaves_t, send, _fileDrawFunc);
}

void fs::getShowFileProps(const std::string& _path)
{
    size_t size = fs::fsize(_path);
    ui::showMessage(ui::getUICString("fileModeFileProperties", 0), _path.c_str(), util::getSizeString(size).c_str());
}

void fs::getShowDirProps(const std::string& _path)
{
    fs::dirCountArgs *send = new fs::dirCountArgs;
    send->path = _path;
    send->origin = true;
    ui::newThread(fs::getShowDirProps_t, send, NULL);
}

bool fs::fileExists(const std::string& path)
{
    bool ret = false;
    FILE *test = fopen(path.c_str(), "rb");
    if(test != NULL)
        ret = true;
    fclose(test);
    return ret;
}

size_t fs::fsize(const std::string& _f)
{
    size_t ret = 0;
    FILE *get = fopen(_f.c_str(), "rb");
    if(get != NULL)
    {
        fseek(get, 0, SEEK_END);
        ret = ftell(get);
    }
    fclose(get);
    return ret;
}

std::string fs::getWorkDir() { return wd; }

void fs::setWorkDir(const std::string& _w){ wd = _w; }

bool fs::isDir(const std::string& _path)
{
    struct stat s;
    return stat(_path.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
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
            fs::copyDirToZip("sv:/", zip);

        }
        else
        {
            fs::mkDir(path);
            path += "/";
            fs::copyDirToDir("sv:/", path);
        }
        ui::populateFldMenu();
    }
}

void fs::overwriteBackup(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupArgs *in = (fs::backupArgs *)t->argPtr;
    ui::menu *m = in->m;
    fs::dirList *d = in->d;

    data::userTitleInfo *cd = data::getCurrentUserTitleInfo();

    unsigned ind = m->getSelected() - 1;;//Skip new

    std::string itemName = d->getItem(ind);
    bool saveHasFiles = fs::dirNotEmpty("sv:/");
    if(d->isDir(ind) && saveHasFiles)
    {
        std::string toPath = util::generatePathByTID(cd->tid) + itemName + "/";
        //Delete and recreate
        fs::delDir(toPath);
        fs::mkDir(toPath);
        fs::copyDirToDir("sv:/", toPath);
    }
    else if(!d->isDir(ind) && d->getItemExt(ind) == "zip" && saveHasFiles)
    {
        std::string toPath = util::generatePathByTID(cd->tid) + itemName;
        fs::delfile(toPath);
        zipFile zip = zipOpen64(toPath.c_str(), 0);
        fs::copyDirToZip("sv:/", zip);
    }
    t->finished = true;
}

void fs::restoreBackup(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupArgs *in = (fs::backupArgs *)t->argPtr;
    ui::menu *m = in->m;
    fs::dirList *d = in->d;

    data::user *u = data::getCurrentUser();
    data::userTitleInfo *cd = data::getCurrentUserTitleInfo();
    unsigned ind = m->getSelected() - 1;

    std::string itemName = d->getItem(ind);
    if((cd->saveInfo.save_data_type != FsSaveDataType_System || cfg::config["sysSaveWrite"]) && m->getSelected() > 0)
    {
        bool saveHasFiles = fs::dirNotEmpty("sv:/");
        if(cfg::config["autoBack"] && cfg::config["zip"] && saveHasFiles)
        {
            std::string autoZip = util::generatePathByTID(cd->tid) + "/AUTO " + u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + ".zip";
            zipFile zip = zipOpen64(autoZip.c_str(), 0);
            fs::copyDirToZip("sv:/", zip);
        }
        else if(cfg::config["autoBack"] && saveHasFiles)
        {
            std::string autoFolder = util::generatePathByTID(cd->tid) + "/AUTO - " + u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + "/";
            fs::mkDir(autoFolder.substr(0, autoFolder.length() - 1));
            fs::copyDirToDir("sv:/", autoFolder);
        }

        if(d->isDir(ind))
        {
            std::string fromPath = util::generatePathByTID(cd->tid) + itemName + "/";
            if(fs::dirNotEmpty(fromPath))
            {
                fs::wipeSave();
                fs::copyDirToDirCommit(fromPath, "sv:/", "sv");
            }
            else
                ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popFolderIsEmpty", 0));
        }
        else if(!d->isDir(ind) && d->getItemExt(ind) == "zip")
        {
            std::string path = util::generatePathByTID(cd->tid) + itemName;
            unzFile unz = unzOpen64(path.c_str());
            if(unz && fs::zipNotEmpty(unz))
            {
                fs::wipeSave();
                fs::copyZipToDir(unz, "sv:/", "sv");
            }
            else
                ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popZipIsEmpty", 0));
        }
        else
        {
            //Just copy file over
            std::string fromPath = util::generatePathByTID(cd->tid) + itemName;
            std::string toPath = "sv:/" + itemName;
            fs::copyFileCommit(fromPath, toPath, "sv");
        }
    }

    if(cfg::config["autoBack"])
        ui::populateFldMenu();

    t->finished = true;
}

void fs::deleteBackup(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupArgs *in = (fs::backupArgs *)t->argPtr;
    ui::menu *m = in->m;
    fs::dirList *d = in->d;

    data::userTitleInfo *cd = data::getCurrentUserTitleInfo();
    unsigned ind = m->getSelected() - 1;

    std::string itemName = d->getItem(ind);
    t->status->setStatus(ui::getUICString("infoStatus", 10));
    if(cfg::config["trashBin"])
    {
        data::userTitleInfo *getTID = data::getCurrentUserTitleInfo();

        std::string oldPath = util::generatePathByTID(cd->tid) + itemName;
        std::string trashPath = wd + "_TRASH_/" + data::getTitleSafeNameByTID(getTID->tid);
        fs::mkDir(trashPath);
        trashPath += "/" + itemName;

        rename(oldPath.c_str(), trashPath.c_str());
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataBackupMovedToTrash", 0), itemName.c_str());
    }
    else if(d->isDir(ind))
    {
        std::string delPath = util::generatePathByTID(cd->tid) + itemName + "/";
        fs::delDir(delPath);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataBackupDeleted", 0), itemName.c_str());
    }
    else
    {
        std::string delPath = util::generatePathByTID(cd->tid) + itemName;
        fs::delfile(delPath);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataBackupDeleted", 0), itemName.c_str());
    }
    ui::populateFldMenu();
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

