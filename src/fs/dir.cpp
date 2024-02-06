#include <switch.h>
#include <algorithm>

#include "fs.h"
#include "cfg.h"
#include "util.h"

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

bool fs::dirNotEmpty(const std::string& _dir)
{
    fs::dirList tmp(_dir);
    return tmp.getCount() > 0;
}

bool fs::isDir(const std::string& _path)
{
    struct stat s;
    return stat(_path.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
}

void fs::copyDirToDir(const std::string& src, const std::string& dst, threadInfo *t)
{
    if(t)
        t->status->setStatus(ui::getUICString("threadStatusOpeningFolder", 0), src.c_str());

    fs::dirList *list = new fs::dirList(src);
    for(unsigned i = 0; i < list->getCount(); i++)
    {
        if(pathIsFiltered(src + list->getItem(i)))
            continue;

        if(list->isDir(i))
        {
            std::string newSrc = src + list->getItem(i) + "/";
            std::string newDst = dst + list->getItem(i) + "/";
            fs::mkDir(newDst.substr(0, newDst.length() - 1));
            fs::copyDirToDir(newSrc, newDst, t);
        }
        else
        {
            std::string fullSrc = src + list->getItem(i);
            std::string fullDst = dst + list->getItem(i);

            if(t)
                t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), fullSrc.c_str());

            fs::copyFile(fullSrc, fullDst, t);
        }
    }
    delete list;
}

static void copyDirToDir_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *in = (fs::copyArgs *)t->argPtr;
    fs::copyDirToDir(in->src, in->dst, t);
    if(in->cleanup)
        fs::copyArgsDestroy(in);
    t->finished = true;
}

void fs::copyDirToDirThreaded(const std::string& src, const std::string& dst)
{
    fs::copyArgs *send = fs::copyArgsCreate(src, dst, "", NULL, NULL, true, false, 0);
    ui::newThread(copyDirToDir_t, send, fs::fileDrawFunc);
}

void fs::copyDirToDirCommit(const std::string& src, const std::string& dst, const std::string& dev, threadInfo *t)
{
    if(t)
        t->status->setStatus(ui::getUICString("threadStatusOpeningFolder", 0), src.c_str());

    fs::dirList *list = new fs::dirList(src);
    for(unsigned i = 0; i < list->getCount(); i++)
    {
        if(pathIsFiltered(src + list->getItem(i)))
            continue;

        if(list->isDir(i))
        {
            std::string newSrc = src + list->getItem(i) + "/";
            std::string newDst = dst + list->getItem(i) + "/";
            fs::mkDir(newDst.substr(0, newDst.length() - 1));
            fs::copyDirToDirCommit(newSrc, newDst, dev, t);
        }
        else
        {
            std::string fullSrc = src + list->getItem(i);
            std::string fullDst = dst + list->getItem(i);

            if(t)
                t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), fullSrc.c_str());

            fs::copyFileCommit(fullSrc, fullDst, dev, t);
        }
    }
    delete list;
}

static void copyDirToDirCommit_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *in = (fs::copyArgs *)t->argPtr;
    fs::copyDirToDirCommit(in->src, in->dst, in->dev, t);
    if(in->cleanup)
        fs::copyArgsDestroy(in);
    t->finished = true;
}

void fs::copyDirToDirCommitThreaded(const std::string& src, const std::string& dst, const std::string& dev)
{
    fs::copyArgs *send = fs::copyArgsCreate(src, dst, dev, NULL, NULL, true, false, 0);
    ui::newThread(copyDirToDirCommit_t, send, fs::fileDrawFunc);
}

void fs::getDirProps(const std::string& path, unsigned& dirCount, unsigned& fileCount, uint64_t& totalSize)
{
    fs::dirList *d = new fs::dirList(path);
    for(unsigned i = 0; i < d->getCount(); i++)
    {
        if(d->isDir(i))
        {
            ++dirCount;
            std::string newPath = path + d->getItem(i) + "/";
            fs::getDirProps(newPath, dirCount, fileCount, totalSize);
        }
        else
        {
            ++fileCount;
            std::string filePath = path + d->getItem(i);
            totalSize += fs::fsize(filePath);
        }
    }
    delete d;
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

fs::dirList::dirList(const std::string& _path, bool ignoreDotFiles)
{
    DIR *d;
    struct dirent *ent;
    path = _path;
    d = opendir(path.c_str());

    while((ent = readdir(d)))
        if (!ignoreDotFiles || ent->d_name[0] != '.')
            item.emplace_back(path, ent->d_name);

    closedir(d);

    std::sort(item.begin(), item.end(), sortDirList);
}

void fs::dirList::reassign(const std::string& _path)
{
    DIR *d;
    struct dirent *ent;
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
    DIR *d;
    struct dirent *ent;
    d = opendir(path.c_str());

    while((ent = readdir(d)))
        item.emplace_back(path, ent->d_name);

    closedir(d);

    std::sort(item.begin(), item.end(), sortDirList);
}
