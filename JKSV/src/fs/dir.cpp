#include "FsLib.hpp"
#include "cfg.h"
#include "fs.h"
#include "util.h"
#include <algorithm>
#include <switch.h>

void fs::mkDir(const std::string &_p)
{
    if (cfg::config["directFsCmd"])
        fsMkDir(_p.c_str());
    else
        mkdir(_p.c_str(), 777);
}

void fs::mkDirRec(const std::string &_p)
{
    //skip first slash
    size_t pos = _p.find('/', 0) + 1;
    while ((pos = _p.find('/', pos)) != _p.npos)
    {
        fs::mkDir(_p.substr(0, pos).c_str());
        ++pos;
    }
}

void fs::delDir(const std::string &path)
{
    FsLib::Directory List(path);
    for (int64_t i = 0; i < List.GetEntryCount(); i++)
    {
        if (pathIsFiltered(path + List.GetEntryNameAt(i)))
        {
            continue;
        }

        if (List.EntryAtIsDirectory(i))
        {
            std::string NewPath = path + List.GetEntryNameAt(i) + "/";
            // Todo: FsLib needs directory deleting functions.
            delDir(NewPath);
            rmdir(NewPath.substr(0, NewPath.length() - 1).c_str());
        }
        else
        {
            std::string TargetPath = path + List.GetEntryNameAt(i);
            std::remove(TargetPath.c_str());
        }
    }
    rmdir(path.c_str());
}

bool fs::dirNotEmpty(const std::string &_dir)
{
    FsLib::Directory TestDir(_dir);
    return TestDir.GetEntryCount() > 0;
}

bool fs::isDir(const std::string &_path)
{
    // Todo: FsLib needs a function to perform this.
    struct stat s;
    return stat(_path.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
}

void fs::copyDirToDir(const std::string &src, const std::string &dst, threadInfo *t)
{
    if (t)
    {
        t->status->setStatus(ui::getUICString("threadStatusOpeningFolder", 0), src.c_str());
    }

    FsLib::Directory List(src);
    if (!List.IsOpen() || List.GetEntryCount() <= 0)
    {
        return;
    }

    for (int64_t i = 0; i < List.GetEntryCount(); i++)
    {
        if (pathIsFiltered(src + List.GetEntryNameAt(i)))
        {
            continue;
        }

        if (List.EntryAtIsDirectory(i))
        {
            std::string NewSource = src + List.GetEntryNameAt(i) + "/";
            std::string NewDestination = dst + List.GetEntryNameAt(i) + "/";
            fs::mkDir(NewDestination.substr(0, NewDestination.length() - 1));
            fs::copyDirToDir(NewSource, NewDestination, t);
        }
        else
        {
            std::string FullSource = src + List.GetEntryNameAt(i);
            std::string FullDestination = dst + List.GetEntryNameAt(i);
            // Update status if thread data passed. I hate my past self. This is such a mess...
            if (t)
            {
                t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), FullSource.c_str());
            }
            fs::copyFile(FullSource, FullDestination, t);
        }
    }
}

static void copyDirToDir_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *in = (fs::copyArgs *)t->argPtr;
    fs::copyDirToDir(in->src, in->dst, t);
    if (in->cleanup)
        fs::copyArgsDestroy(in);
    t->finished = true;
}

void fs::copyDirToDirThreaded(const std::string &src, const std::string &dst)
{
    fs::copyArgs *send = fs::copyArgsCreate(src, dst, "", NULL, NULL, true, false, 0);
    ui::newThread(copyDirToDir_t, send, fs::fileDrawFunc);
}

void fs::copyDirToDirCommit(const std::string &src, const std::string &dst, const std::string &dev, threadInfo *t)
{
    if (t)
    {
        // Todo: Drop this from rewrite. Opening folders is too quick for this to matter. Waste of RAM and string fetching.
        t->status->setStatus(ui::getUICString("threadStatusOpeningFolder", 0), src.c_str());
    }

    FsLib::Directory List(src);
    if (!List.IsOpen() || List.GetEntryCount() <= 0)
    {
        return;
    }

    for (int64_t i = 0; i < List.GetEntryCount(); i++)
    {
        if (pathIsFiltered(src + List.GetEntryNameAt(i)))
        {
            continue;
        }

        if (List.EntryAtIsDirectory(i))
        {
            std::string NewSource = src + List.GetEntryNameAt(i) + "/";
            std::string NewDestination = dst + List.GetEntryNameAt(i) + "/";
            fs::mkDir(NewDestintion.substr(0, NewDestination.length() - 1));
            fs::copyDirToDirCommit(NewSource, NewDestination, dev, t);
        }
        else
        {
            std::string FullSource = src + List.GetEntryNameAt(i);
            std::string FullDestination = dst + List.GetEntryNameAt(i);

            if (t)
            {
                t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), FullSource.c_str());
            }
            fs::copyFileCommit(FullSource, FullDestination, dev, t);
        }
    }
}

static void copyDirToDirCommit_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *in = (fs::copyArgs *)t->argPtr;
    fs::copyDirToDirCommit(in->src, in->dst, in->dev, t);
    if (in->cleanup)
        fs::copyArgsDestroy(in);
    t->finished = true;
}

void fs::copyDirToDirCommitThreaded(const std::string &src, const std::string &dst, const std::string &dev)
{
    fs::copyArgs *send = fs::copyArgsCreate(src, dst, dev, NULL, NULL, true, false, 0);
    ui::newThread(copyDirToDirCommit_t, send, fs::fileDrawFunc);
}

void fs::getDirProps(const std::string &path, unsigned &dirCount, unsigned &fileCount, uint64_t &totalSize)
{
    FsLib::Directory List(path);
    if (!List.IsOpen() || List.GetEntryCount() <= 0)
    {
        return;
    }

    for (int64_t i = 0; i < List.GetEntryCount(); i++)
    {
        if (List.EntryAtIsDirectory(i))
        {
            ++dirCount;
            std::string NewPath = path + List.GetEntryNameAt(i) + "/";
            fs::getDirProps(NewPath, dirCount, fileCount, totalSize);
        }
        else
        {
            ++fileCount;
            totalSize += List.GetEntrySizeAt(i);
        }
    }
}
