#include <cstdio>
#include <algorithm>
#include <cstring>
#include <vector>
#include <switch.h>
#include <unistd.h>
#include <cstdarg>
#include <sys/stat.h>

#include "fs.h"
#include "util.h"
#include "ui.h"
#include "gfx.h"
#include "data.h"
#include "cfg.h"

static std::string wd = "sdmc:/JKSV/";

static std::vector<std::string> pathFilter;

fs::copyArgs *fs::copyArgsCreate(const std::string& src, const std::string& dst, const std::string& dev, zipFile z, unzFile unz, bool _cleanup, bool _trimZipPath, uint8_t _trimPlaces)
{
    copyArgs *ret = new copyArgs;
    ret->src = src;
    ret->dst = dst;
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

void fs::copyFile(const std::string& src, const std::string& dst, threadInfo *t)
{
    fs::copyArgs *c = NULL;
    if(t)
    {
        c = (fs::copyArgs *)t->argPtr;
        c->offset = 0;
        c->prog->setMax(fs::fsize(src));
        c->prog->update(0);
    }

    if(cfg::config["directFsCmd"])
    {
        FSFILE *fsrc = fsfopen(src.c_str(), FsOpenMode_Read);
        FSFILE *fdst = fsfopen(dst.c_str(), FsOpenMode_Write);

        if(!fsrc || !fdst)
        {
            fsfclose(fsrc);
            fsfclose(fdst);
            return;
        }

        uint8_t *buff = new uint8_t[BUFF_SIZE];
        size_t readIn = 0;
        while((readIn = fsfread(buff, 1, BUFF_SIZE, fsrc)) > 0)
        {
            fsfwrite(buff, 1, readIn, fdst);
            if(c)
                c->offset += readIn;
        }
        fsfclose(fsrc);
        fsfclose(fdst);
        delete[] buff;
    }
    else
    {
        FILE *fsrc = fopen(src.c_str(), "rb");
        FILE *fdst = fopen(dst.c_str(), "wb");

        if(!fsrc || !fdst)
        {
            fclose(fsrc);
            fclose(fdst);
            return;
        }

        uint8_t *buff = new uint8_t[BUFF_SIZE];
        size_t readIn = 0;
        while((readIn = fread(buff, 1, BUFF_SIZE, fsrc)) > 0)
        {
            fwrite(buff, 1, readIn, fdst);
            if(c)
                c->offset += readIn;
        }
        fclose(fsrc);
        fclose(fdst);
        delete[] buff;
    }
}

static void copyFileThreaded_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *in = (fs::copyArgs *)t->argPtr;

    t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), in->src);

    fs::copyFile(in->src, in->dst, t);
    if(in->cleanup)
        fs::copyArgsDestroy(in);
    t->finished = true;
}

void fs::copyFileThreaded(const std::string& src, const std::string& dst)
{
    fs::copyArgs *send = fs::copyArgsCreate(src, dst, "", NULL, NULL, true, false, 0);
    ui::newThread(copyFileThreaded_t, send, fs::fileDrawFunc);
}

void fs::copyFileCommit(const std::string& src, const std::string& dst, const std::string& dev, threadInfo *t)
{
    fs::copyArgs *c = NULL;
    if(t)
    {
        c = (fs::copyArgs *)t->argPtr;
        c->offset = 0;
        c->prog->setMax(fs::fsize(src));
        c->prog->update(0);
    }

    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    uint64_t journ = fs::getJournalSize(utinfo);
    if(cfg::config["directFsCmd"])
    {
        FSFILE *fsrc = fsfopen(src.c_str(), FsOpenMode_Read);
        FSFILE *fdst = fsfopen(dst.c_str(), FsOpenMode_Write);

        if(!fsrc || !fdst)
        {
            fsfclose(fsrc);
            fsfclose(fdst);
            return;
        }

        uint8_t *buff = new uint8_t[BUFF_SIZE];
        size_t readIn = 0, writeCount = 0;
        while((readIn = fsfread(buff, 1, BUFF_SIZE, fsrc)) > 0)
        {
            fsfwrite(buff, 1, readIn, fdst);
            writeCount += readIn;
            if(c)
                c->offset += readIn;

            if(writeCount >= (journ - 0x100000))
            {
                writeCount = 0;
                fsfclose(fdst);
                if(!fs::commitToDevice(dev.c_str()))
                    break;

                fdst = fsfopen(dst.c_str(), FsOpenMode_Write | FsOpenMode_Append);
            }
        }
        fsfclose(fsrc);
        fsfclose(fdst);
        fs::commitToDevice(dev);
        delete[] buff;
    }
    else
    {
        FILE *fsrc = fopen(src.c_str(), "rb");
        FILE *fdst = fopen(dst.c_str(), "wb");

        if(!fsrc || !fdst)
        {
            fclose(fsrc);
            fclose(fdst);
            return;
        }

        uint8_t *buff = new uint8_t[BUFF_SIZE];
        size_t readIn = 0, writeCount = 0;
        while((readIn = fread(buff, 1, BUFF_SIZE, fsrc)) > 0)
        {
            fwrite(buff, 1, readIn, fdst);
            writeCount += readIn;
            if(c)
                c->offset += readIn;

            if(writeCount >= (journ - 0x100000))
            {
                writeCount = 0;
                fclose(fdst);
                if(!fs::commitToDevice(dev))
                    break;

                fdst = fopen(dst.c_str(), "ab");
            }
        }
        fclose(fsrc);
        fclose(fdst);
        fs::commitToDevice(dev);
        delete[] buff;
    }
}

static void copyFileCommit_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *in = (fs::copyArgs *)t->argPtr;

    t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), in->src);
    in->prog->setMax(fs::fsize(in->src));
    in->prog->update(0);

    fs::copyFileCommit(in->src, in->dst, in->dev, t);
    if(in->cleanup)
        fs::copyArgsDestroy(in);

    t->finished = true;
}

void fs::copyFileCommitThreaded(const std::string& src, const std::string& dst, const std::string& dev)
{
    fs::copyArgs *send = fs::copyArgsCreate(src, dst, dev, NULL, NULL, true, false, 0);
    ui::newThread(copyFileCommit_t, send, fs::fileDrawFunc);
}

void fs::fileDrawFunc(void *a)
{
    threadInfo *t = (threadInfo *)a;
    if(!t->finished)
    {
        copyArgs *c = (copyArgs *)t->argPtr;
        std::string tmp;
        t->status->getStatus(tmp);
        c->argLock();
        c->prog->update(c->offset);
        c->prog->draw(tmp);
        c->argUnlock();
    }
}

void fs::delfile(const std::string& path)
{
    if(cfg::config["directFsCmd"])
        fsremove(path.c_str());
    else
        remove(path.c_str());
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

void fs::dumpAllUserSaves()
{
    //This is only really used for the progress bar
    /*fs::copyArgs *send = fs::copyArgsCreate("", "", "", NULL, NULL, true, false, 0);
    ui::newThread(fs::backupUserSaves_t, send, _fileDrawFunc);*/
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
//    ui::newThread(fs::getShowDirProps_t, send, NULL);
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
