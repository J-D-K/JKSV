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

static FsFile logFile;
static s64 offset = 0;

static struct
{
    bool operator()(fs::dirItem& a, fs::dirItem& b)
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

static inline size_t fsize(const std::string& _f)
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

namespace fs
{
    void init()
    {
        mkdir("sdmc:/JKSV", 777);
        logOpen();
        wd = "sdmc:/JKSV/";
    }

    void exit()
    {
        logClose();
    }

    bool mountSave(data::user& usr, data::titledata& open)
    {
        Result svOpen;
        FsFileSystem sv;

        switch(open.getType())
        {
            case FsSaveDataType_System:
                svOpen = fsOpen_SystemSaveData(&sv, FsSaveDataSpaceId_System, open.getID(), (AccountUid){0});
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
                svOpen = 1; //For Now
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

    dirItem::dirItem(const std::string& pathTo, const std::string& sItem)
    {
        itm = sItem;

        std::string fullPath = pathTo + sItem;
        struct stat s;
        if(stat(fullPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
            dir = true;
    }

    dirList::dirList(const std::string& _path)
    {
        path = _path;
        d = opendir(path.c_str());

        while((ent = readdir(d)))
        {
            dirItem add(path, ent->d_name);
            item.push_back(add);
        }
        closedir(d);

        std::sort(item.begin(), item.end(), sortDirList);
    }

    void dirList::reassign(const std::string& _path)
    {
        path = _path;

        d = opendir(path.c_str());

        item.clear();

        while((ent = readdir(d)))
        {
            dirItem add(path, ent->d_name);
            item.push_back(add);
        }
        closedir(d);

        std::sort(item.begin(), item.end(), sortDirList);
    }

    void dirList::rescan()
    {
        item.clear();
        d = opendir(path.c_str());

        while((ent = readdir(d)))
        {
            dirItem add(path, ent->d_name);
            item.push_back(add);
        }
        closedir(d);

        std::sort(item.begin(), item.end(), sortDirList);
    }

    std::string dirList::getItem(int index)
    {
        return item[index].getItm();
    }

    bool dirList::isDir(int index)
    {
        return item[index].isDir();
    }

    unsigned dirList::getCount()
    {
        return item.size();
    }

    dataFile::dataFile(const std::string& _path)
    {
        f = fopen(_path.c_str(), "r");
        if(f != NULL)
            opened = true;
    }

    dataFile::~dataFile()
    {
        fclose(f);
    }

    std::string dataFile::getNextLine()
    {
        std::string ret = "";
        char tmp[1024];
        while(fgets(tmp, 1024, f))
        {
            if(tmp[0] != '#' && tmp[0] != '\n')
            {
                ret = tmp;
                break;
            }
        }
        util::stripNL(ret);
        return ret;
    }

    void copyFile(const std::string& from, const std::string& to)
    {
        Thread cpyThread;
        uint64_t gProg = 0;
        copyArgs *thr = copyArgsCreate(from, to, "", &gProg);
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

    void copyFileCommit(const std::string& from, const std::string& to, const std::string& dev)
    {
        Thread cpyThread;
        uint64_t gProg = 0;
        copyArgs *thr = copyArgsCreate(from, to, dev, &gProg);
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

    void copyDirToDir(const std::string& from, const std::string& to)
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

                copyFile(fullFrom, fullTo);
            }
        }
    }

    void copyDirToDirCommit(const std::string& from, const std::string& to, const std::string& dev)
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

    void delDir(const std::string& path)
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

    void dumpAllUserSaves(data::user& u)
    {
        for(unsigned i = 0; i < u.titles.size(); i++)
        {
            if(fs::mountSave(u, u.titles[i]))
            {
                u.titles[i].createDir();

                //sdmc:/JKSV/[title]/[user] - [date]/
                std::string outPath = u.titles[i].getPath() + u.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
                mkdir(outPath.c_str(), 777);
                outPath += "/";

                std::string root = "sv:/";

                fs::copyDirToDir(root, outPath);

                fsdevUnmountDevice("sv");
            }
        }
    }

    std::string getFileProps(const std::string& _path)
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

    void getDirProps(const std::string& _path, uint32_t& dirCount, uint32_t& fileCount, uint64_t& totalSize)
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

    bool fileExists(const std::string& path)
    {
        FILE *test = fopen(path.c_str(), "rb");
        if(test != NULL)
        {
            fclose(test);
            return true;
        }

        return false;
    }

    std::string getWorkDir() { return wd; }

    bool isDir(const std::string& _path)
    {
        struct stat s;
        return stat(_path.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
    }

    void logOpen()
    {
        remove("sdmc:/JKSV/log.txt");
        FsFileSystem *sd = fsdevGetDeviceFileSystem("sdmc");
        fsFsCreateFile(sd, "/JKSV/log.txt", 0, FsWriteOption_Flush);
        fsFsOpenFile(sd, "/JKSV/log.txt", FsOpenMode_Write, &logFile);
    }

    void logWrite(const char *fmt, ...)
    {
        char tmp[256];
        va_list args;
        va_start(args, fmt);
        vsprintf(tmp, fmt, args);

        unsigned tmpLength = strlen(tmp);
        s64 curSize = 0;
        fsFileGetSize(&logFile, &curSize);
        curSize += tmpLength;
        fsFileSetSize(&logFile, curSize);

        fsFileWrite(&logFile, offset, tmp, tmpLength, 0);
        offset += tmpLength;
    }

    void logClose()
    {
        fsFileClose(&logFile);
    }
}
