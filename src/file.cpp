#include <fstream>
#include <cstdio>
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
        Result res = 0;

        if(open.getType() == FsSaveDataType_SaveData)
        {
            res = fsMount_SaveData(&sv, open.getID(), usr.getUID());
            if(R_FAILED(res))
                return false;

            int r = fsdevMountDevice("sv", sv);
            if(r == -1)
                return false;
        }
        else if(data::sysSave)
        {
            res = fsMount_SystemSaveData(&sv, open.getID());
            if(R_FAILED(res))
                return false;

            int r = fsdevMountDevice("sv", sv);
            if(r == -1)
                return false;
        }

        return true;
    }

    dirList::dirList(const std::string& _path)
    {
        path = _path;
        d = opendir(path.c_str());

        while((ent = readdir(d)))
        {
            item.push_back(ent->d_name);
        }

        closedir(d);
    }

    void dirList::reassign(const std::string& _path)
    {
        path = _path;

        d = opendir(path.c_str());

        item.clear();

        while((ent = readdir(d)))
        {
            item.push_back(ent->d_name);
        }

        closedir(d);
    }

    void dirList::rescan()
    {
        item.clear();
        d = opendir(path.c_str());

        while((ent = readdir(d)))
        {
            item.push_back(ent->d_name);
        }

        closedir(d);
    }

    std::string dirList::getItem(int index)
    {
        return item[index];
    }

    bool dirList::isDir(int index)
    {
        std::string fullPath = path + item[index];
        struct stat s;
        if(stat(fullPath.c_str(), &s) == 0)
        {
            if(S_ISDIR(s.st_mode))
                return true;
        }

        return false;
    }

    unsigned dirList::getCount()
    {
        return item.size();
    }

    void copyFile(const std::string& from, const std::string& to)
    {
        std::fstream f(from, std::ios::in | std::ios::binary);
        std::fstream t(to, std::ios::out | std::ios::binary);

        f.seekg(0, f.end);
        size_t fileSize = f.tellg();
        f.seekg(0, f.beg);

        uint8_t *buff = new uint8_t[BUFF_SIZE];
        ui::progBar prog(fileSize);

        std::string copyString = "Copying " + from + "...";
        copyString = util::getWrappedString(copyString, 24, 1136);
        for(unsigned i = 0; i < fileSize; )
        {
            f.read((char *)buff, BUFF_SIZE);
            t.write((char *)buff, f.gcount());

            i += f.gcount();
            prog.update(i);

            prog.draw(copyString);

            gfxHandleBuffs();
        }

        delete[] buff;

        f.close();
        t.close();
    }

    void copyFileCommit(const std::string& from, const std::string& to, const std::string& dev)
    {
        std::fstream f(from, std::ios::in | std::ios::binary);
        std::fstream t(to, std::ios::out | std::ios::binary);

        f.seekg(0, f.end);
        size_t fileSize = f.tellg();
        f.seekg(0, f.beg);

        uint8_t *buff = new uint8_t[BUFF_SIZE];
        ui::progBar prog(fileSize);

        std::string copyString = "Copying " + from + "...";
        copyString = util::getWrappedString(copyString, 24, 1136);
        for(unsigned i = 0; i < fileSize; )
        {
            f.read((char *)buff, BUFF_SIZE);
            t.write((char *)buff, f.gcount());

            i += f.gcount();
            prog.update(i);

            prog.draw(copyString);

            gfxHandleBuffs();
        }

        delete[] buff;

        f.close();
        t.close();

        Result res = fsdevCommitDevice(dev.c_str());
        if(R_FAILED(res))
            ui::showError("Error committing file to device", res);
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
                std::string newPath = path + "/" + list.getItem(i) + "/";
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
                std::string outPath = util::getTitleDir(u, u.titles[i]) + u.getUsernameSafe() + " - " + util::getDateTime();
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

    std::string getWorkDir()
    {
        return wd;
    }
}
