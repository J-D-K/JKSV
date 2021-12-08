#include <cstdio>
#include <algorithm>
#include <cstring>
#include <vector>
#include <switch.h>
#include <unistd.h>
#include <cstdarg>
#include <mutex>
#include <condition_variable>
#include <sys/stat.h>

#include "fs.h"
#include "util.h"
#include "ui.h"
#include "gfx.h"
#include "data.h"
#include "cfg.h"

static std::string wd = "sdmc:/JKSV/";

typedef struct
{
    std::mutex bufferLock;
    std::condition_variable cond;
    std::vector<uint8_t> sharedBuffer;
    std::string dst, dev;
    bool bufferIsFull = false;
    unsigned int filesize = 0, writeLimit = 0;
} fileCpyThreadArgs;

static void writeFile_t(void *a)
{
    fsSetPriority(FsPriority_Realtime);
    fileCpyThreadArgs *in = (fileCpyThreadArgs *)a;
    size_t written = 0;
    std::vector<uint8_t> localBuffer;
    FILE *out = fopen(in->dst.c_str(), "wb");

    while(written < in->filesize)
    {
        std::unique_lock<std::mutex> buffLock(in->bufferLock);
        in->cond.wait(buffLock, [in]{ return in->bufferIsFull;});
        localBuffer.clear();
        localBuffer.assign(in->sharedBuffer.begin(), in->sharedBuffer.end());
        in->sharedBuffer.clear();
        in->bufferIsFull = false;
        buffLock.unlock();
        in->cond.notify_one();
        written += fwrite(localBuffer.data(), 1, localBuffer.size(), out);
    }
    fclose(out);
}

static void writeFileCommit_t(void *a)
{
    fsSetPriority(FsPriority_Realtime);
    fileCpyThreadArgs *in = (fileCpyThreadArgs *)a;
    size_t written = 0, journalCount = 0;
    std::vector<uint8_t> localBuffer;
    FILE *out = fopen(in->dst.c_str(), "wb");

    while(written < in->filesize)
    {
        std::unique_lock<std::mutex> buffLock(in->bufferLock);
        in->cond.wait(buffLock, [in]{ return in->bufferIsFull; });
        localBuffer.clear();
        localBuffer.assign(in->sharedBuffer.begin(), in->sharedBuffer.end());
        in->sharedBuffer.clear();
        in->bufferIsFull = false;
        buffLock.unlock();
        in->cond.notify_one();

        written += fwrite(localBuffer.data(), 1, localBuffer.size(), out);

        journalCount += written;
        if(journalCount >= in->writeLimit)
        {
            journalCount = 0;
            fclose(out);
            fs::commitToDevice(in->dev.c_str());
            out = fopen(in->dst.c_str(), "ab");
        }
    }
    fclose(out);
}

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
    size_t filesize = fs::fsize(src);
    if(t)
    {
        c = (fs::copyArgs *)t->argPtr;
        c->offset = 0;
        c->prog->setMax(filesize);
        c->prog->update(0);
    }

    FILE *fsrc = fopen(src.c_str(), "rb");
    if(!fsrc)
    {
        fclose(fsrc);
        return;
    }

    fileCpyThreadArgs thrdArgs;
    thrdArgs.dst = dst;
    thrdArgs.filesize = filesize;

    uint8_t *buff = new uint8_t[BUFF_SIZE];
    std::vector<uint8_t> transferBuffer;
    Thread writeThread;
    threadCreate(&writeThread, writeFile_t, &thrdArgs, NULL, 0x40000, 0x2C, 1);
    threadStart(&writeThread);
    size_t readIn = 0;
    while((readIn = fread(buff, 1, BUFF_SIZE, fsrc)) > 0)
    {
        transferBuffer.insert(transferBuffer.end(), buff, buff + readIn);
        if(c)
            c->offset += readIn;

        if(transferBuffer.size() > TRANSFER_BUFFER_LIMIT || readIn < BUFF_SIZE)
        {
            std::unique_lock<std::mutex> buffLock(thrdArgs.bufferLock);
            thrdArgs.cond.wait(buffLock, [&thrdArgs]{ return thrdArgs.bufferIsFull == false; });
            thrdArgs.sharedBuffer.assign(transferBuffer.begin(), transferBuffer.end());
            transferBuffer.clear();
            thrdArgs.bufferIsFull = true;
            buffLock.unlock();
            thrdArgs.cond.notify_one();
        }
    }
    threadWaitForExit(&writeThread);
    threadClose(&writeThread);
    fclose(fsrc);
    delete[] buff;
}

static void copyFileThreaded_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *in = (fs::copyArgs *)t->argPtr;

    t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), in->src.c_str());

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
    size_t filesize = fs::fsize(src);
    if(t)
    {
        c = (fs::copyArgs *)t->argPtr;
        c->offset = 0;
        c->prog->setMax(filesize);
        c->prog->update(0);
    }

    FILE *fsrc = fopen(src.c_str(), "rb");
    if(!fsrc)
    {
        fclose(fsrc);
        return;
    }

    fileCpyThreadArgs thrdArgs;
    thrdArgs.dst = dst;
    thrdArgs.dev = dev;
    thrdArgs.filesize = filesize;
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    uint64_t journalSpace = fs::getJournalSize(utinfo);
    thrdArgs.writeLimit = (journalSpace - 0x100000) < TRANSFER_BUFFER_LIMIT ? journalSpace - 0x100000 : TRANSFER_BUFFER_LIMIT;

    Thread writeThread;
    threadCreate(&writeThread, writeFileCommit_t, &thrdArgs, NULL, 0x040000, 0x2C, 1);

    uint8_t *buff = new uint8_t[BUFF_SIZE];
    size_t readIn = 0;
    std::vector<uint8_t> transferBuffer;
    threadStart(&writeThread);
    while((readIn = fread(buff, 1, BUFF_SIZE, fsrc)) > 0)
    {
        transferBuffer.insert(transferBuffer.end(), buff, buff + readIn);
        if(c)
            c->offset += readIn;
        if(transferBuffer.size() >= thrdArgs.writeLimit || readIn < BUFF_SIZE)
        {
            std::unique_lock<std::mutex> buffLock(thrdArgs.bufferLock);
            thrdArgs.cond.wait(buffLock, [&thrdArgs]{ return thrdArgs.bufferIsFull == false; });
            thrdArgs.sharedBuffer.assign(transferBuffer.begin(), transferBuffer.end());
            transferBuffer.clear();
            thrdArgs.bufferIsFull = true;
            buffLock.unlock();
            thrdArgs.cond.notify_one();
        }
    }
    threadWaitForExit(&writeThread);
    threadClose(&writeThread);

    fclose(fsrc);
    fs::commitToDevice(dev);
    delete[] buff;
}

static void copyFileCommit_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *in = (fs::copyArgs *)t->argPtr;

    t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), in->src.c_str());
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
    if(!t->finished && t->argPtr)
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

void fs::getShowFileProps(const std::string& _path)
{
    size_t size = fs::fsize(_path);
    ui::showMessage(ui::getUICString("fileModeFileProperties", 0), _path.c_str(), util::getSizeString(size).c_str());
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
