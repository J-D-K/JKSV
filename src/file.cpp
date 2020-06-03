#include <cstdio>
#include <algorithm>
#include <cstring>
#include <switch.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdarg>
#include <sys/stat.h>

#include "file.h"
#include "util.h"
#include "ui.h"
#include "gfx.h"
#include "data.h"

#define BUFF_SIZE 512 * 1024

static std::string wd;

static FSFILE *log;

static FsFileSystem sv;

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

typedef struct
{
    std::string from, to, dev;
    uint64_t *pos;
    bool fin;
} copyArgs;

static copyArgs *copyArgsCreate(const std::string& _f, const std::string& _t, const std::string& _dev, uint64_t *_p)
{
    copyArgs *ret = new copyArgs;
    ret->from = _f;
    ret->to = _t;
    ret->dev = _dev;
    ret->pos = _p;
    ret->fin = false;
    return ret;
}

static void copyfile_t(void *a)
{
    copyArgs *args = (copyArgs *)a;

    FILE *f = fopen(args->from.c_str(), "rb");
    FILE *t = fopen(args->to.c_str(), "wb");
    if(f == NULL || t == NULL)
    {
        fclose(f);
        fclose(t);
        args->fin = true;
        return;
    }

    size_t read = 0;
    unsigned char *buff = new unsigned char[BUFF_SIZE];
    while((read = fread(buff, 1, BUFF_SIZE, f)))
    {
        fwrite(buff, 1, read, t);
        *args->pos += read;
    }
    delete[] buff;
    fclose(f);
    fclose(t);

    args->fin = true;
}

static void copyfileFS_t(void *a)
{
    copyArgs *args = (copyArgs *)a;

    FSFILE *f = fsfopen(args->from.c_str(), FsOpenMode_Read);
    FSFILE *t = fsfopen(args->to.c_str(), FsOpenMode_Write);
    if(f == NULL || t == NULL)
    {
        fsfclose(f);
        fsfclose(t);
        args->fin = true;
        return;
    }

    size_t read = 0;
    unsigned char *buff = new unsigned char[BUFF_SIZE];
    while((read = fsfread(buff, 1, BUFF_SIZE, f)))
    {
        fsfwrite(buff, 1, read, t);
        *args->pos += read;
    }
    delete[] buff;
    fsfclose(f);
    fsfclose(t);

    args->fin = true;
}

static void copyfileCommit_t(void *a)
{
    copyArgs *args = (copyArgs *)a;

    FILE *f = fopen(args->from.c_str(), "rb");
    FILE *t = fopen(args->to.c_str(), "wb");
    if(f == NULL || t == NULL)
    {
        fclose(f);
        fclose(t);
        args->fin = true;
        return;
    }

    size_t read = 0;
    unsigned char *buff = new unsigned char[BUFF_SIZE];
    while((read = fread(buff, 1, BUFF_SIZE, f)))
    {
        fwrite(buff, 1, read, t);
        *args->pos += read;
    }
    delete[] buff;
    fclose(f);
    fclose(t);

    fsdevCommitDevice(args->dev.c_str());

    args->fin = true;
}

static void copyfileCommitFS_t(void *a)
{
    copyArgs *args = (copyArgs *)a;

    FSFILE *f = fsfopen(args->from.c_str(), FsOpenMode_Read);
    FSFILE *t = fsfopen(args->to.c_str(), FsOpenMode_Write);
    if(f == NULL || t == NULL)
    {
        fsfclose(f);
        fsfclose(t);
        args->fin = true;
        return;
    }

    size_t read = 0;
    unsigned char *buff = new unsigned char[BUFF_SIZE];
    while((read = fsfread(buff, 1, BUFF_SIZE, f)))
    {
        fsfwrite(buff, 1, read, t);
        *args->pos += read;
    }
    delete[] buff;
    fsfclose(f);
    fsfclose(t);

    fsdevCommitDevice(args->dev.c_str());

    args->fin = true;
}

static void mkdirRec(const std::string& _p)
{
    //skip first slash
    size_t pos = _p.find('/', 0) + 1;
    while((pos = _p.find('/', pos)) != _p.npos)
    {
        std::string create;
        create.assign(_p.begin(), _p.begin() + pos);
        mkdir(create.c_str(), 777);
        ++pos;
    }
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
        mkdirRec(wd);
    }
    else
    {
        mkdir("sdmc:/JKSV", 777);
        wd = "sdmc:/JKSV/";
    }
    fs::logOpen();
}

void fs::exit()
{
    fs::logClose();
}

bool fs::mountSave(const data::user& usr, const data::titledata& open)
{
    Result svOpen;
    switch(open.getType())
    {
        case FsSaveDataType_System:
            svOpen = fsOpen_SystemSaveData(&sv, FsSaveDataSpaceId_System, open.getID(), usr.getUID128() == 1 ? (AccountUid) { 0 } : usr.getUID());
            break;

        case FsSaveDataType_Account:
            svOpen = fsOpen_SaveData(&sv, open.getID(), usr.getUID());
            break;

        case FsSaveDataType_Bcat:
            svOpen = fsOpen_BcatSaveData(&sv, open.getID());
            break;

        case FsSaveDataType_Device:
            svOpen = fsOpen_DeviceSaveData(&sv, open.getID());
            break;

        case FsSaveDataType_Temporary:
            svOpen = fsOpen_TemporaryStorage(&sv);
            break;

        case FsSaveDataType_Cache:
            svOpen = fsOpen_CacheStorage(&sv, open.getID(), open.getSaveIndex());
            break;

        case FsSaveDataType_SystemBcat:
            svOpen = fsOpen_SystemBcatSaveData(&sv, open.getID());
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

fs::dirList::dirList(const std::string& _path)
{
    path = _path;
    d = opendir(path.c_str());

    while((ent = readdir(d)))
        item.push_back(dirItem(path, ent->d_name));

    closedir(d);

    std::sort(item.begin(), item.end(), sortDirList);
}

void fs::dirList::reassign(const std::string& _path)
{
    path = _path;

    d = opendir(path.c_str());

    item.clear();

    while((ent = readdir(d)))
        item.push_back(dirItem(path, ent->d_name));

    closedir(d);

    std::sort(item.begin(), item.end(), sortDirList);
}

void fs::dirList::rescan()
{
    item.clear();
    d = opendir(path.c_str());

    while((ent = readdir(d)))
        item.push_back(dirItem(path, ent->d_name));

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

void fs::copyFile(const std::string& from, const std::string& to)
{
    Thread cpyThread;
    uint64_t gProg = 0;
    copyArgs *thr = copyArgsCreate(from, to, "", &gProg);
    if(data::directFsCmd)
        threadCreate(&cpyThread, copyfileFS_t, thr, NULL, 0x4000, 0x2B, 1);
    else
        threadCreate(&cpyThread, copyfile_t, thr, NULL, 0x4000, 0x2B, 1);

    ui::progBar fileProg(fsize(from));
    threadStart(&cpyThread);
    while(!thr->fin)
    {
        fileProg.update(*thr->pos);
        gfxBeginFrame();
        fileProg.draw(from, "Copying File...");
        gfxEndFrame();
    }
    threadClose(&cpyThread);
    delete thr;
}

void fs::copyFileCommit(const std::string& from, const std::string& to, const std::string& dev)
{
    Thread cpyThread;
    uint64_t gProg = 0;
    copyArgs *thr = copyArgsCreate(from, to, dev, &gProg);
    if(data::directFsCmd)
        threadCreate(&cpyThread, copyfileCommitFS_t, thr, NULL, 0x4000, 0x2B, 1);
    else
        threadCreate(&cpyThread, copyfileCommit_t, thr, NULL, 0x4000, 0x2B, 1);

    ui::progBar fileProg(fsize(from));
    threadStart(&cpyThread);
    while(!thr->fin)
    {
        fileProg.update(*thr->pos);
        gfxBeginFrame();
        fileProg.draw(from, "Copying File...");
        gfxEndFrame();
    }
    threadClose(&cpyThread);
    delete thr;
}

void fs::copyDirToDir(const std::string& from, const std::string& to)
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

            copyDirToDir(newFrom, newTo);
        }
        else
        {
            std::string fullFrom = from + list.getItem(i);
            std::string fullTo   = to   + list.getItem(i);

            if(hasFreeSpace(fullTo, fsize(fullFrom)))
                copyFile(fullFrom, fullTo);
            else
                ui::showMessage("*Error*", "Not enough free space to copy #%s#!", fullFrom.c_str());
        }
    }
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

            if(hasFreeSpace(fullTo, fsize(fullFrom)))
                copyFileCommit(fullFrom, fullTo, dev);
            else
                ui::showMessage("*Error*", "Not enough free space to copy #%s#!", fullFrom.c_str());
        }
    }
}

void fs::delfile(const std::string& path)
{
    if(data::directFsCmd)
        fsremove(path.c_str());
    else
        remove(path.c_str());
}

void fs::delDir(const std::string& path)
{
    dirList list(path);
    for(unsigned i = 0; i < list.getCount(); i++)
    {
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

bool fs::dumpAllUserSaves(const data::user& u)
{
    for(unsigned i = 0; i < u.titles.size(); i++)
    {
        hidScanInput();

        if(hidKeysHeld(CONTROLLER_P1_AUTO) & KEY_B)
            return false;

        if(fs::mountSave(u, u.titles[i]))
        {
            u.titles[i].createDir();

            //sdmc:/JKSV/[title]/[user] - [date]/
            std::string outPath = u.titles[i].getPath() + u.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_ASC);
            mkdir(outPath.c_str(), 777);
            outPath += "/";
            fs::copyDirToDir("sv:/", outPath);

            fsdevUnmountDevice("sv");
        }
    }

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
        std::sprintf(tmp, "Path: \"%s\"\nSize: %u", _path.c_str(), fileSize);

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
            dirCount++;
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
            fileCount++;
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
        fseek(get, 0, SEEK_SET);
    }
    fclose(get);
    return ret;
}

bool fs::hasFreeSpace(const std::string& _f, int needed)
{
    s64 free = 0;
    //grab device from _f
    size_t endDevPos = _f.find(':', 0);
    std::string dev;
    dev.assign(_f.begin(), _f.begin() + endDevPos);
    fsFsGetFreeSpace(fsdevGetDeviceFileSystem(dev.c_str()), "/", &free);

    return free > needed;
}

std::string fs::getWorkDir() { return wd; }

bool fs::isDir(const std::string& _path)
{
    struct stat s;
    return stat(_path.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
}

void fs::logOpen()
{
    std::string logPath = wd + "log.txt";
    remove(logPath.c_str());
    log = fsfopen(logPath.c_str(), FsOpenMode_Write);
}

void fs::logWrite(const char *fmt, ...)
{
    char tmp[256];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    fsfwrite(tmp, 1, strlen(tmp), log);
}

void fs::logClose()
{
    fsfclose(log);
}

