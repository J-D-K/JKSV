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

static std::string wd;

static std::vector<std::string> pathFilter;

static FSFILE *debLog;

static FsFileSystem sv;

fs::copyArgs *fs::copyArgsCreate(const std::string& from, const std::string& to, const std::string& dev, zipFile z, unzFile unz, bool _cleanup)
{
    copyArgs *ret = new copyArgs;
    ret->to = to;
    ret->from = from;
    ret->dev = dev;
    ret->z = z;
    ret->unz = unz;
    ret->cleanup = _cleanup;
    ret->prog = new ui::progBar;
    ret->fileSize = 0.0f;
    ret->offset = 0.0f;
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

void fs::mkdirRec(const std::string& _p)
{
    //skip first slash
    size_t pos = _p.find('/', 0) + 1;
    while((pos = _p.find('/', pos)) != _p.npos)
    {
        mkdir(_p.substr(0, pos).c_str(), 777);
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
        ui::showPopMessage(POP_FRAME_DEFAULT, "Error committing file to device!");
        ret = false;
    }
    return ret;
}

void fs::init()
{
    if(fs::fileExists("sdmc:/switch/jksv_dir.txt"))
    {
        char tmp[256];
        FILE *getDir = fopen("sdmc:/switch/jksv_dir.txt", "r");
        fgets(tmp, 256, getDir);
        fclose(getDir);
        wd = tmp;
        util::stripChar('\n', wd);
        util::stripChar('\r', wd);
    }
    else
        wd = "sdmc:/JKSV/";

    mkdirRec(wd);
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
    size_t pPos = line.find_first_of('('), ePos = line.find_first_of('=');
    if(pPos != line.npos || ePos != line.npos)
    {
        lPos = ePos < pPos ? ePos : pPos;
        name.assign(line.begin(), line.begin() + lPos);
    }
    else
        name = "NULL";
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
    copyArgs *send = copyArgsCreate(from, to, "", NULL, NULL, false);
    send->fileSize = fs::fsize(from);
    send->prog->setMax(send->fileSize);
    ui::newThread(copyfile_t, send, _fileDrawFunc);
}

void fs::copyFileCommit(const std::string& from, const std::string& to, const std::string& dev)
{
    copyArgs *send = copyArgsCreate(from, to, dev, NULL, NULL, false);
    send->fileSize = fs::fsize(from);
    send->prog->setMax(send->fileSize);
    ui::newThread(copyFileCommit_t, send, _fileDrawFunc);
}

void fs::copyDirToDir(const std::string& from, const std::string& to)
{
    dirList list(from);

    for(unsigned i = 0; i < list.getCount(); i++)
    {
        if(pathIsFiltered(from + list.getItem(i)))
            continue;

        if(list.isDir(i))
        {
            std::string newFrom = from + list.getItem(i) + "/";
            std::string newTo   = to + list.getItem(i) + "/";
            mkdir(newTo.substr(0, newTo.length() - 1).c_str(), 0777);

            copyDirToDir(newFrom, newTo);
        }
        else
        {
            std::string fullFrom = from + list.getItem(i);
            std::string fullTo   = to   + list.getItem(i);
            copyFile(fullFrom, fullTo);
        }
    }
}

void fs::copyDirToZip(const std::string& from, zipFile to)
{
    if(data::config["ovrClk"])
    {
        util::setCPU(1785000000);
        ui::showPopMessage(POP_FRAME_DEFAULT, "CPU Boost Enabled for ZIP.");
    }
    copyArgs *send = copyArgsCreate(from, "", "", to, NULL, true);
    ui::newThread(copyDirToZip_t, send, _fileDrawFunc);
}

void fs::copyZipToDir(unzFile unz, const std::string& to, const std::string& dev)
{
    if(data::config["ovrClk"])
    {
        util::setCPU(1785000000);
        ui::showPopMessage(POP_FRAME_DEFAULT, "CPU Boost Enabled for ZIP.");
    }
    copyArgs *send = copyArgsCreate("", to, dev, NULL, unz, true);
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
    dirList list(from);

    for(unsigned i = 0; i < list.getCount(); i++)
    {
        if(list.isDir(i))
        {
            std::string newFrom = from + list.getItem(i) + "/";
            std::string newTo   = to + list.getItem(i);
            mkdir(newTo.c_str(), 0777);
            newTo += "/";

            copyDirToDirCommit(newFrom, newTo, dev);
        }
        else
        {
            std::string fullFrom = from + list.getItem(i);
            std::string fullTo   = to   + list.getItem(i);
            copyFileCommit(fullFrom, fullTo, dev);
        }
    }
}

void fs::delfile(const std::string& path)
{
    if(data::config["directFsCmd"])
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

void fs::loadPathFilters(const std::string& _file)
{
    if(fs::fileExists(_file))
    {
        fs::dataFile filter(_file);
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

bool fs::dumpAllUserSaves(const data::user& u)
{
    for(unsigned i = 0; i < u.titleInfo.size(); i++)
    {
        ui::updateInput();

        if(ui::padKeysDown() & HidNpadButton_B)
            return false;

        if(fs::mountSave(u.titleInfo[i].saveInfo))
        {
            util::createTitleDirectoryByTID(u.titleInfo[i].saveID);
            std::string basePath = util::generatePathByTID(u.titleInfo[i].saveID);
            switch(data::config["zip"])
            {
                case true:
                    {
                        std::string outPath = basePath + u.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + ".zip";
                        zipFile zip = zipOpen(outPath.c_str(), 0);
                        fs::copyDirToZip("sv:/", zip);
                        zipClose(zip, NULL);
                    }
                    break;

                case false:
                    {
                        std::string outPath = basePath + u.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + "/";
                        mkdir(outPath.substr(0, outPath.length() - 1).c_str(), 777);
                        fs::copyDirToDir("sv:/", outPath);
                    }
                    break;
            }
            fsdevUnmountDevice("sv");
        }
    }

    ui::updateInput();

    return true;//?
}

std::string fs::getFileProps(const std::string& _path)
{
    std::string ret = "";

    FILE *get = fopen(_path.c_str(), "rb");
    if(get != NULL)
    {
        //Size
        fseek(get, 0, SEEK_END);
        unsigned fileSize = ftell(get);
        fseek(get, 0, SEEK_SET);

        fclose(get);

        //Probably add more later

        char tmp[256];
        std::sprintf(tmp, "Path: #%s#\nSize: %u", _path.c_str(), fileSize);

        ret = tmp;
    }
    return ret;
}

void fs::getDirProps(const std::string& _path, uint32_t& dirCount, uint32_t& fileCount, uint64_t& totalSize)
{
    fs::dirList list(_path);

    for(unsigned i = 0; i < list.getCount(); i++)
    {
        if(list.isDir(i))
        {
            ++dirCount;
            std::string newPath = _path + list.getItem(i) + "/";
            uint32_t dirAdd = 0, fileAdd = 0;
            uint64_t sizeAdd = 0;

            getDirProps(newPath, dirAdd, fileAdd, sizeAdd);
            dirCount += dirAdd;
            fileCount += fileAdd;
            totalSize += sizeAdd;
        }
        else
        {
            ++fileCount;
            std::string filePath = _path + list.getItem(i);

            FILE *gSize = fopen(filePath.c_str(), "rb");
            fseek(gSize, 0, SEEK_END);
            size_t fSize = ftell(gSize);
            fclose(gSize);

            totalSize += fSize;
        }
    }
}

bool fs::fileExists(const std::string& path)
{
    FILE *test = fopen(path.c_str(), "rb");
    if(test != NULL)
    {
        fclose(test);
        return true;
    }

    return false;
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

bool fs::isDir(const std::string& _path)
{
    struct stat s;
    return stat(_path.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
}

void fs::createNewBackup(void *a)
{
    uint64_t held = ui::padKeysHeld();

    std::string out;

    if(held & HidNpadButton_R)
        out = data::users[data::selUser].getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
    else if(held & HidNpadButton_L)
        out = data::users[data::selUser].getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YDM);
    else if(held & HidNpadButton_ZL)
        out = data::users[data::selUser].getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_HOYSTE);
    else
    {
        const std::string dict[] =
        {
            util::getDateTime(util::DATE_FMT_YMD),
            util::getDateTime(util::DATE_FMT_YDM),
            util::getDateTime(util::DATE_FMT_HOYSTE),
            util::getDateTime(util::DATE_FMT_JHK),
            util::getDateTime(util::DATE_FMT_ASC),
            data::users[data::selUser].getUsernameSafe(),
            data::getTitleInfoByTID(data::curData.saveID)->safeTitle,
            util::generateAbbrev(data::curData.saveID),
            ".zip"
        };
        std::string defaultText = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
        out = util::getStringInput(SwkbdType_QWERTY, defaultText, "Enter a name", 64, 9, dict);
    }

    if(!out.empty())
    {
        std::string ext = util::getExtensionFromString(out);
        std::string path = util::generatePathByTID(data::curData.saveID) + out;
        if(data::config["zip"] || ext == "zip")
        {
            if(ext != "zip")//data::zip is on but extension is not zip
                path += ".zip";

            zipFile zip = zipOpen64(path.c_str(), 0);
            fs::copyDirToZip("sv:/", zip);

        }
        else
        {
            mkdir(path.c_str(), 777);
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

    unsigned ind = m->getSelected() - 1;;//Skip new

    std::string itemName = d->getItem(ind);
    if(d->isDir(ind))
    {
        std::string toPath = util::generatePathByTID(data::curData.saveID) + itemName + "/";
        //Delete and recreate
        fs::delDir(toPath);
        mkdir(toPath.c_str(), 777);
        fs::copyDirToDir("sv:/", toPath);
    }
    else if(!d->isDir(ind) && d->getItemExt(ind) == "zip")
    {
        std::string toPath = util::generatePathByTID(data::curData.saveID) + itemName;
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

    unsigned ind = m->getSelected() - 1;

    std::string itemName = d->getItem(ind);
    if((data::curData.saveInfo.save_data_type != FsSaveDataType_System || data::config["sysSaveWrite"]) && m->getSelected() > 0)
    {
        if(data::config["autoBack"] && data::config["zip"])
        {
            std::string autoZip = util::generatePathByTID(data::curData.saveID) + "/AUTO " + data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + ".zip";
            zipFile zip = zipOpen64(autoZip.c_str(), 0);
            fs::copyDirToZip("sv:/", zip);
        }
        else if(data::config["autoBack"])
        {
            std::string autoFolder = util::generatePathByTID(data::curData.saveID) + "/AUTO - " + data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + "/";
            mkdir(autoFolder.substr(0, autoFolder.length() - 1).c_str(), 777);
            fs::copyDirToDir("sv:/", autoFolder);
        }

        if(d->isDir(ind))
        {
            std::string fromPath = util::generatePathByTID(data::curData.saveID) + itemName + "/";
            if(fs::dirNotEmpty(fromPath))
            {
                fs::wipeSave();
                fs::copyDirToDirCommit(fromPath, "sv:/", "sv");
            }
            else
                ui::showPopMessage(POP_FRAME_DEFAULT, "Folder is empty!");
        }
        else if(!d->isDir(ind) && d->getItemExt(ind) == "zip")
        {
            std::string path = util::generatePathByTID(data::curData.saveID) + itemName;
            unzFile unz = unzOpen64(path.c_str());
            if(unz && fs::zipNotEmpty(unz))
            {
                fs::wipeSave();
                fs::copyZipToDir(unz, "sv:/", "sv");
            }
            else
                ui::showPopMessage(POP_FRAME_DEFAULT, "ZIP is empty!");
        }
        else
        {
            //Just copy file over
            std::string fromPath = util::generatePathByTID(data::curData.saveID) + itemName;
            std::string toPath = "sv:/" + itemName;
            fs::copyFileCommit(fromPath, toPath, "sv");
        }
    }

    if(data::config["autoBack"])
        ui::populateFldMenu();

    t->finished = true;
}

void fs::deleteBackup(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::backupArgs *in = (fs::backupArgs *)t->argPtr;
    ui::menu *m = in->m;
    fs::dirList *d = in->d;

    unsigned ind = m->getSelected() - 1;

    std::string itemName = d->getItem(ind);
    t->status->setStatus("Deleting '" + itemName + "'...");

    if(data::config["trashBin"])
    {
        std::string oldPath = util::generatePathByTID(data::curData.saveID) + itemName;
        std::string trashPath = wd + "_TRASH_/" + itemName;
        rename(oldPath.c_str(), trashPath.c_str());
        ui::showPopMessage(POP_FRAME_DEFAULT, "%s moved to trash.", itemName.c_str());
    }
    else if(d->isDir(ind))
    {
        std::string delPath = util::generatePathByTID(data::curData.saveID) + itemName + "/";
        fs::delDir(delPath);
        ui::showPopMessage(POP_FRAME_DEFAULT, "%s has been deleted.", itemName.c_str());
    }
    else
    {
        std::string delPath = util::generatePathByTID(data::curData.saveID) + itemName;
        fs::delfile(delPath);
        ui::showPopMessage(POP_FRAME_DEFAULT, "%s has been deleted.", itemName.c_str());
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

