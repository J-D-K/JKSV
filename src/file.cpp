#include <fstream>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <switch.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include "file.h"
#include "util.h"
#include "ui.h"
#include "gfx.h"
#include "data.h"

#define BUFF_SIZE 512 * 1024

static std::string wd;


static Result fsMountBCAT(FsFileSystem *out, data::titledata& open)
{
    FsSaveDataAttribute attr;
    std::memset(&attr, 0, sizeof(FsSaveDataAttribute));
    attr.application_id = open.getID();
    attr.save_data_type = FsSaveDataType_Bcat;

    return fsOpenSaveDataFileSystem(out, FsSaveDataSpaceId_User, &attr);
}

static Result fsMountDeviceSave(FsFileSystem *out, data::titledata& open)
{
    FsSaveDataAttribute attr;
    std::memset(&attr, 0, sizeof(FsSaveDataAttribute));
    attr.application_id = open.getID();
    attr.save_data_type = FsSaveDataType_Device;

    return fsOpenSaveDataFileSystem(out, FsSaveDataSpaceId_User, &attr);
}

static struct
{
    bool operator()(fs::dirItem& a, fs::dirItem& b)
    {
        for(unsigned i = 0; i < a.getItm().length(); i++)
        {
            char charA = tolower(a.getItm()[i]);
            char charB = tolower(b.getItm()[i]);
            if(charA != charB)
                return charA < charB;
        }
        return false;
    }
} sortListAlpha;

namespace fs
{
    void init()
    {
        mkdir("sdmc:/JKSV", 777);
        chdir("sdmc:/JKSV");

        wd = "sdmc:/JKSV/";
    }

    bool mountSave(data::user& usr, data::titledata& open)
    {
        FsFileSystem sv;

        if(open.getType() == FsSaveDataType_Account && R_FAILED(fsOpen_SaveData(&sv, open.getID(), usr.getUID())))
            return false;
        else if(open.getType() == FsSaveDataType_System && R_FAILED(fsOpen_SystemSaveData(&sv, FsSaveDataSpaceId_System, open.getID(), (AccountUid){0})))
            return false;
        else if(open.getType() == FsSaveDataType_Bcat && R_FAILED(fsMountBCAT(&sv, open)))
            return false;
        else if(open.getType() == FsSaveDataType_Device && R_FAILED(fsMountDeviceSave(&sv, open)))
            return false;

        if(fsdevMountDevice("sv", sv) == -1)
            return false;

        return true;
    }

    dirItem::dirItem(const std::string& pathTo, const std::string& sItem)
    {
        itm = sItem;

        std::string fullPath = pathTo + sItem;
        struct stat s;
        if(stat(fullPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode))
            dir = true;
    }

    std::string dirItem::getItm()
    {
        return itm;
    }

    bool dirItem::isDir()
    {
        return dir;
    }

    dirList::dirList(const std::string& _path)
    {
        path = _path;
        d = opendir(path.c_str());

        std::vector<dirItem> dirVect, filVect;

        while((ent = readdir(d)))
        {
            dirItem add(path, ent->d_name);
            if(add.isDir())
                dirVect.push_back(add);
            else
                filVect.push_back(add);
        }

        closedir(d);

        std::sort(dirVect.begin(), dirVect.end(), sortListAlpha);
        std::sort(filVect.begin(), filVect.end(), sortListAlpha);

        item.assign(dirVect.begin(), dirVect.end());
        item.insert(item.end(), filVect.begin(), filVect.end());
    }

    void dirList::reassign(const std::string& _path)
    {
        path = _path;

        d = opendir(path.c_str());

        item.clear();

        std::vector<dirItem> dirVect, filVect;

        while((ent = readdir(d)))
        {
            dirItem add(path, ent->d_name);
            if(add.isDir())
                dirVect.push_back(add);
            else
                filVect.push_back(add);
        }

        closedir(d);

        std::sort(dirVect.begin(), dirVect.end(), sortListAlpha);
        std::sort(filVect.begin(), filVect.end(), sortListAlpha);

        item.assign(dirVect.begin(), dirVect.end());
        item.insert(item.end(), filVect.begin(), filVect.end());
    }

    void dirList::rescan()
    {
        item.clear();
        d = opendir(path.c_str());

        std::vector<dirItem> dirVect, filVect;

        while((ent = readdir(d)))
        {
            dirItem add(path, ent->d_name);
            if(add.isDir())
                dirVect.push_back(add);
            else
                filVect.push_back(add);
        }

        closedir(d);

        std::sort(dirVect.begin(), dirVect.end(), sortListAlpha);
        std::sort(filVect.begin(), filVect.end(), sortListAlpha);

        item.assign(dirVect.begin(), dirVect.end());
        item.insert(item.end(), filVect.begin(), filVect.end());
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

    void copyFile(const std::string& from, const std::string& to)
    {
        std::fstream f(from, std::ios::in | std::ios::binary);
        std::fstream t(to, std::ios::out | std::ios::binary);

        if(!f.is_open() || !t.is_open())
        {
            f.close();
            t.close();
            return;
        }

        f.seekg(0, f.end);
        size_t fileSize = f.tellg();
        f.seekg(0, f.beg);

        uint8_t *buff = new uint8_t[BUFF_SIZE];
        ui::progBar prog(fileSize);

        for(unsigned i = 0; i < fileSize; )
        {
            f.read((char *)buff, BUFF_SIZE);
            t.write((char *)buff, f.gcount());

            i += f.gcount();
            prog.update(i);

            gfxBeginFrame();
            prog.draw(from, "Copying File:");
            gfxEndFrame();
        }

        delete[] buff;

        f.close();
        t.close();
    }

    void copyFileCommit(const std::string& from, const std::string& to, const std::string& dev)
    {
        std::fstream f(from, std::ios::in | std::ios::binary);
        std::fstream t(to, std::ios::out | std::ios::binary);

        if(!f.is_open() || !t.is_open())
        {
            f.close();
            t.close();
            return;
        }

        f.seekg(0, f.end);
        size_t fileSize = f.tellg();
        f.seekg(0, f.beg);

        uint8_t *buff = new uint8_t[BUFF_SIZE];
        ui::progBar prog(fileSize);

        for(unsigned i = 0; i < fileSize; )
        {
            f.read((char *)buff, BUFF_SIZE);
            t.write((char *)buff, f.gcount());

            i += f.gcount();
            prog.update(i);

            gfxBeginFrame();
            prog.draw(from, "Copying File:");
            gfxEndFrame();
        }

        delete[] buff;

        f.close();
        t.close();

        if(R_FAILED(fsdevCommitDevice(dev.c_str())))
            ui::showMessage("Error committing file to device!", "*ERROR*");
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
                util::makeTitleDir(u, u.titles[i]);

                //sdmc:/JKSV/[title]/[user] - [date]/
                std::string outPath = util::getTitleDir(u, u.titles[i]) + u.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
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
        std::fstream get(_path, std::ios::in | std::ios::binary);
        if(get.is_open())
        {
            //Size
            get.seekg(0, get.end);
            unsigned fileSize = get.tellg();
            get.seekg(0, get.beg);

            get.close();

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

                std::fstream gSize(filePath.c_str(), std::ios::in | std::ios::binary);
                gSize.seekg(0, gSize.end);
                size_t fSize = gSize.tellg();
                gSize.close();

                totalSize += fSize;
            }
        }
    }

    bool fileExists(const std::string& path)
    {
        std::fstream chk(path, std::ios::in);
        if(chk.is_open())
        {
            chk.close();
            return true;
        }

        return false;
    }

    bool isDir(const std::string& _path)
    {
        struct stat s;
        return stat(_path.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
    }

    std::string getWorkDir()
    {
        return wd;
    }
}
