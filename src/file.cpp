#include <cstdio>
#include <algorithm>
#include <cstring>
#include <vector>
#include <switch.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdarg>
#include <minizip/zip.h>
#include <minizip/unzip.h>
#include <sys/stat.h>

#include "file.h"
#include "util.h"
#include "ui.h"
#include "gfx.h"
#include "data.h"

#define BUFF_SIZE 0xC0000

static std::string wd;

static std::vector<std::string> pathFilter;

static FSFILE *debLog;

static FsFileSystem sv;

typedef struct
{
    std::string to, from, dev;
    zipFile z;
    unzFile unz;
    bool cleanup = false;
    uint64_t offset = 0, fileSize = 0;
    ui::progBar *prog;
    threadStatus *thrdStatus;
    Mutex arglck = 0;
    void argLock() { mutexLock(&arglck); }
    void argUnlock() { mutexUnlock(&arglck); }
} copyArgs;

static copyArgs *copyArgsCreate(const std::string& from, const std::string& to, const std::string& dev, zipFile z, unzFile unz, bool _cleanup)
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

static void copyArgsDestroy(copyArgs *c)
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

static void mkdirRec(const std::string& _p)
{
    //skip first slash
    size_t pos = _p.find('/', 0) + 1;
    while((pos = _p.find('/', pos)) != _p.npos)
    {
        mkdir(_p.substr(0, pos).c_str(), 777);
        ++pos;
    }
}

static inline bool commitToDevice(const std::string& dev)
{
    bool ret = true;
    Result res = fsdevCommitDevice(dev.c_str());
    if(R_FAILED(res))
    {
        fs::logWrite("Error committing to device -> 0x%X", res);
        ui::showPopMessage(POP_FRAME_DEFAULT, "Error commmitting file to device!");
        ret = false;
    }
    return ret;
}

static uint64_t getJournalSize(const data::titleInfo *t)
{
    uint64_t journalSize = 0;
    switch(data::curData.saveInfo.save_data_type)
    {
        case FsSaveDataType_Account:
            journalSize = t->nacp.user_account_save_data_journal_size;
            break;

        case FsSaveDataType_Device:
            journalSize = t->nacp.device_save_data_journal_size;
            break;

        case FsSaveDataType_Bcat:
            journalSize = t->nacp.bcat_delivery_cache_storage_size;
            break;

        case FsSaveDataType_Cache:
            journalSize = t->nacp.cache_storage_journal_size;
            break;

        default:
            journalSize = BUFF_SIZE;
            break;
    }
    return journalSize;
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
    fs::logClose();
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

    return line.substr(pos1, lPos++ - pos1);
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

//Todo: Weird flickering?
static void _fileDrawFunc(void *a)
{
    threadInfo *t = (threadInfo *)a;
    if(!t->finished)
    {
        copyArgs *c = (copyArgs *)t->argPtr;
        std::string tmp;
        t->status->getStatus(tmp);
        c->argLock();
        c->prog->draw(tmp);
        c->argUnlock();
    }
}

static void copyfile_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;
    t->status->setStatus("Copying '" + args->from + "'...");

    uint8_t *buff = new uint8_t[BUFF_SIZE];
    if(data::config["directFsCmd"])
    {
        FSFILE *in = fsfopen(args->from.c_str(), FsOpenMode_Read);
        FSFILE *out = fsfopen(args->to.c_str(), FsOpenMode_Write);

        if(!in || !out)
        {
            fsfclose(in);
            fsfclose(out);
            t->finished = true;
            return;
        }
        size_t readIn = 0;
        while((readIn = fsfread(buff, 1, BUFF_SIZE, in)) > 0)
        {
            fsfwrite(buff, 1, readIn, out);
            args->argLock();
            args->offset = in->offset;
            args->prog->update(args->offset);
            args->argUnlock();
        }
        fsfclose(in);
        fsfclose(out);
    }
    else
    {
        FILE *in = fopen(args->from.c_str(), "rb");
        FILE *out = fopen(args->to.c_str(), "wb");
        if(!in || !out)
        {
            fclose(in);
            fclose(out);
            t->finished = true;
            return;
        }

        size_t readIn = 0;
        while((readIn = fread(buff, 1, BUFF_SIZE, in)) > 0)
        {
            fwrite(buff, 1, readIn, out);
            args->argLock();
            args->offset = ftell(in);
            args->prog->update(args->offset);
            args->argUnlock();
        }
        fclose(in);
        fclose(out);
    }
    delete[] buff;
    copyArgsDestroy(args);
    t->finished = true;
}

void fs::copyFile(const std::string& from, const std::string& to)
{
    copyArgs *send = copyArgsCreate(from, to, "", NULL, NULL, false);
    send->fileSize = fs::fsize(from);
    send->prog->setMax(send->fileSize);
    ui::newThread(copyfile_t, send, _fileDrawFunc);
}

void copyFileCommit_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;
    data::titleInfo *info = data::getTitleInfoByTID(data::curData.saveID);
    t->status->setStatus("Copying '" + args->from + "'...");

    uint64_t journalSize = getJournalSize(info), writeCount = 0;
    uint8_t *buff = new uint8_t[BUFF_SIZE];

    if(data::config["directFsCmd"])
    {
        FSFILE *in  = fsfopen(args->from.c_str(), FsOpenMode_Read);
        FSFILE *out = fsfopen(args->to.c_str(), FsOpenMode_Write);

        if(!in || !out)
        {
            fsfclose(in);
            fsfclose(out);
            t->finished = true;
            return;
        }

        size_t readIn = 0;
        while((readIn = fsfread(buff, 1, BUFF_SIZE, in)) > 0)
        {
            fsfwrite(buff, 1, readIn, out);
            writeCount += readIn;
            if(writeCount >= (journalSize - 0x100000))
            {
                writeCount = 0;
                fsfclose(out);
                if(!commitToDevice(args->dev))
                    break;

                out = fsfopen(args->to.c_str(), FsOpenMode_Write | FsOpenMode_Append);
            }
            args->argLock();
            args->offset = out->offset;
            args->prog->update(args->offset);
            args->argUnlock();
        }
        fsfclose(in);
        fsfclose(out);
    }
    else
    {
        FILE *in = fopen(args->from.c_str(), "rb");
        FILE *out = fopen(args->to.c_str(), "wb");

        if(!in || !out)
        {
            fclose(in);
            fclose(out);
            t->finished = true;
            return;
        }

        size_t readIn = 0;
        while((readIn = fread(buff, 1, BUFF_SIZE, in)) > 0)
        {
            fwrite(buff, 1, readIn, out);
            writeCount += readIn;
            if(writeCount >= (journalSize - 0x100000))
            {
                writeCount = 0;
                fclose(out);
                if(!commitToDevice(args->dev))
                    break;

                out = fopen(args->to.c_str(), "ab");
            }
            args->argLock();
            args->offset = ftell(out);
            args->prog->update(args->offset);
            args->argUnlock();
        }
        fclose(in);
        fclose(out);
    }
    delete[] buff;

    commitToDevice(args->dev.c_str());
    copyArgsDestroy(args);
    t->finished = true;
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

void closeZip_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    zipFile z = t->argPtr;
    zipClose(z, NULL);
    t->finished = true;
}

void copyDirToZip_t(void *a)
{
    threadInfo *t  = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;

    t->status->setStatus("Opening " + args->from + "...");
    fs::dirList *list = new fs::dirList(args->from);

    unsigned listTotal = list->getCount();
    for(unsigned i = 0; i < listTotal; i++)
    {
        std::string itm = list->getItem(i);
        if(fs::pathIsFiltered(args->from + itm))
            continue;

        if(list->isDir(i))
        {
            std::string newFrom = args->from + itm + "/";
            //Fake thread and new args to point to src thread stuff
            //This wouldn't work spawning a new thread.
            threadInfo *tmpThread = new threadInfo;
            tmpThread->status = t->status;
            copyArgs *tmpArgs = new copyArgs;
            tmpArgs->from = newFrom;
            tmpArgs->prog = args->prog;
            tmpArgs->z = args->z;
            tmpArgs->cleanup = false;
            tmpThread->argPtr = tmpArgs;
            copyDirToZip_t(tmpThread);
            delete tmpArgs;
            delete tmpThread;
        }
        else
        {
            zip_fileinfo inf = {0};
            std::string filename = args->from + itm;
            size_t devPos = filename.find_first_of('/') + 1;
            t->status->setStatus("Adding '" + itm + "' to ZIP.");
            int zOpenFile = zipOpenNewFileInZip64(args->z, filename.substr(devPos, filename.length()).c_str(), &inf, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0);
            if(zOpenFile == ZIP_OK)
            {
                std::string fullFrom = args->from + itm;
                args->offset = 0;
                args->fileSize = fs::fsize(fullFrom);
                args->prog->setMax(args->fileSize);
                args->prog->update(0);
                FILE *cpy = fopen(fullFrom.c_str(), "rb");
                size_t readIn = 0;
                uint8_t *buff = new uint8_t[BUFF_SIZE];
                while((readIn = fread(buff, 1, BUFF_SIZE, cpy)) > 0)
                {
                    zipWriteInFileInZip(args->z, buff, readIn);
                    args->offset += readIn;
                    args->prog->update(args->offset);
                }
                delete[] buff;
                fclose(cpy);
                zipCloseFileInZip(args->z);
            }
        }
    }
    delete list;
    if(args->cleanup)
    {
        if(data::config["ovrClk"])
            util::setCPU(1224000000);
        ui::newThread(closeZip_t, args->z, NULL);
        delete args->prog;
        delete args;
    }

    t->finished = true;
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

void copyZipToDir_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;

    data::titleInfo *tinfo = data::getTitleInfoByTID(data::curData.saveID);
    uint64_t journalSize = getJournalSize(tinfo), writeCount = 0;
    char filename[FS_MAX_PATH];
    uint8_t *buff = new uint8_t[BUFF_SIZE];
    int readIn = 0;
    unz_file_info64 info;
    if(unzGoToFirstFile(args->unz) == UNZ_OK)
    {
        do
        {
            unzGetCurrentFileInfo64(args->unz, &info, filename, FS_MAX_PATH, NULL, 0, NULL, 0);
            if(unzOpenCurrentFile(args->unz) == UNZ_OK)
            {
                t->status->setStatus("Copying '" + std::string(filename) + "'...");
                std::string path = args->to + filename;
                mkdirRec(path.substr(0, path.find_last_of('/') + 1));

                args->fileSize = info.uncompressed_size;
                args->offset = 0.0f;
                args->prog->setMax(args->fileSize);
                size_t done = 0;
                if(data::config["directFsCmd"])
                {
                    FSFILE *out = fsfopen(path.c_str(), FsOpenMode_Write);
                    while((readIn = unzReadCurrentFile(args->unz, buff, BUFF_SIZE)) > 0)
                    {
                        done += readIn;
                        writeCount += readIn;
                        args->offset += readIn;
                        args->prog->update(args->offset);
                        fsfwrite(buff, 1, readIn, out);
                        if(writeCount >= (journalSize - 0x100000))
                        {
                            writeCount = 0;
                            fsfclose(out);
                            if(!commitToDevice(args->dev.c_str()))
                                break;

                            out = fsfopen(path.c_str(), FsOpenMode_Write | FsOpenMode_Append);
                        }
                    }
                    fsfclose(out);
                }
                else
                {
                    FILE *out = fopen(path.c_str(), "wb");

                    while((readIn = unzReadCurrentFile(args->unz, buff, BUFF_SIZE)) > 0)
                    {
                        done += readIn;
                        writeCount += readIn;
                        args->offset += readIn;
                        args->prog->update(args->offset);
                        fwrite(buff, 1, readIn, out);
                        if(writeCount >= (journalSize - 0x100000))
                        {
                            writeCount = 0;
                            fclose(out);
                            if(!commitToDevice(args->dev.c_str()))
                                break;

                            out = fopen(path.c_str(), "ab");
                        }
                    }
                    fclose(out);
                }
                unzCloseCurrentFile(args->unz);
                commitToDevice(args->dev.c_str());
            }
        }
        while(unzGoToNextFile(args->unz) != UNZ_END_OF_LIST_OF_FILE);
    }
    else
        ui::showPopMessage(POP_FRAME_DEFAULT, "ZIP file is empty!");

    if(args->cleanup)
    {
        unzClose(args->unz);
        copyArgsDestroy(args);
        if(data::config["ovrClk"])
            util::setCPU(1224000000);
    }
    delete[] buff;
    t->finished = true;
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
        out = util::getStringInput(SwkbdType_QWERTY, "", "Enter a name", 64, 9, dict);
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
            fs::wipeSave();
            std::string fromPath = util::generatePathByTID(data::curData.saveID) + itemName + "/";
            fs::copyDirToDirCommit(fromPath, "sv:/", "sv");
        }
        else if(!d->isDir(ind) && d->getItemExt(ind) == "zip")
        {
            fs::wipeSave();
            std::string path = util::generatePathByTID(data::curData.saveID) + itemName;
            unzFile unz = unzOpen64(path.c_str());
            fs::copyZipToDir(unz, "sv:/", "sv");
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
    remove(logPath.c_str());
    debLog = fsfopen(logPath.c_str(), FsOpenMode_Write);
}

void fs::logWrite(const char *fmt, ...)
{
    char tmp[256];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);
    fsfwrite(tmp, 1, strlen(tmp), debLog);
}

void fs::logClose()
{
    fsfclose(debLog);
}

