#include <switch.h>
#include <time.h>
#include <mutex>
#include <vector>
#include <condition_variable>

#include "fs.h"
#include "util.h"
#include "cfg.h"

typedef struct
{
    std::mutex buffLock;
    std::condition_variable cond;
    std::vector<uint8_t> sharedBuffer;
    std::string dst, dev;
    bool bufferIsFull = false;
    unzFile unz;
    unsigned int fileSize, writeLimit = 0;
} unzThrdArgs;

static void writeFileFromZip_t(void *a)
{
    unzThrdArgs *in = (unzThrdArgs *)a;
    std::vector<uint8_t> localBuffer;
    unsigned int written = 0, journalCount = 0;

    FILE *out = fopen(in->dst.c_str(), "wb");
    while(written < in->fileSize)
    {
        std::unique_lock<std::mutex> buffLock(in->buffLock);
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
            fs::commitToDevice(in->dev);
            out = fopen(in->dst.c_str(), "ab");
        }
    }
    fclose(out);
}

void fs::copyDirToZip(const std::string& src, zipFile dst, bool trimPath, int trimPlaces, threadInfo *t)
{
    fs::copyArgs *c = NULL;
    if(t)
    {
        t->status->setStatus(ui::getUICString("threadStatusOpeningFolder", 0), src.c_str());
        c = (fs::copyArgs *)t->argPtr;
    }

    fs::dirList *list = new fs::dirList(src);
    for(unsigned i = 0; i < list->getCount(); i++)
    {
        std::string itm = list->getItem(i);
        if(fs::pathIsFiltered(src + itm))
            continue;

        if(list->isDir(i))
        {
            std::string newSrc = src + itm + "/";
            fs::copyDirToZip(newSrc, dst, trimPath, trimPlaces, t);
        }
        else
        {
            time_t raw;
            time(&raw);
            tm *locTime = localtime(&raw);
            zip_fileinfo inf = { locTime->tm_sec, locTime->tm_min, locTime->tm_hour,
                                 locTime->tm_mday, locTime->tm_mon, (1900 + locTime->tm_year), 0, 0, 0 };

            std::string filename = src + itm;
            size_t zipNameStart = 0;
            if(trimPath)
                util::trimPath(filename, trimPlaces);
            else
                zipNameStart = filename.find_first_of('/') + 1;

            if(t)
                t->status->setStatus(ui::getUICString("threadStatusAddingFileToZip", 0), itm.c_str());

            int zipOpenFile = zipOpenNewFileInZip64(dst, filename.substr(zipNameStart, filename.npos).c_str(), &inf, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0);
            if(zipOpenFile == ZIP_OK)
            {
                std::string fullSrc = src + itm;
                if(c)
                {
                    c->offset = 0;
                    c->prog->setMax(fs::fsize(fullSrc));
                    c->prog->update(0);
                }

                FILE *fsrc = fopen(fullSrc.c_str(), "rb");
                size_t readIn = 0;
                uint8_t *buff = new uint8_t[ZIP_BUFF_SIZE];
                while((readIn = fread(buff, 1, ZIP_BUFF_SIZE, fsrc)) > 0)
                {
                    zipWriteInFileInZip(dst, buff, readIn);
                    if(c)
                        c->offset += readIn;
                }
                delete[] buff;
                fclose(fsrc);
            }
        }
    }
}

void copyDirToZip_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *c = (fs::copyArgs *)t->argPtr;
    if(cfg::config["ovrClk"])
    {
        util::sysBoost();
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popCPUBoostEnabled", 0));
    }

    fs::copyDirToZip(c->src, c->z, c->trimZipPath, c->trimZipPlaces, t);

    if(cfg::config["ovrClk"])
        util::sysNormal();

    if(c->cleanup)
    {
        zipClose(c->z, NULL);
        delete c;
    }
    t->finished = true;
}

void fs::copyDirToZipThreaded(const std::string& src, zipFile dst, bool trimPath, int trimPlaces)
{
    fs::copyArgs *send = fs::copyArgsCreate(src, "", "", dst, NULL, true, false, 0);
    ui::newThread(copyDirToZip_t, send, fs::fileDrawFunc);
}

void fs::copyZipToDir(unzFile src, const std::string& dst, const std::string& dev, threadInfo *t)
{
    fs::copyArgs *c = NULL;
    if(t)
        c = (fs::copyArgs *)t->argPtr;

    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    uint64_t journalSize = getJournalSize(utinfo);
    char filename[FS_MAX_PATH];
    uint8_t *buff = new uint8_t[BUFF_SIZE];
    int readIn = 0;
    unz_file_info64 info;
    do
    {
        unzGetCurrentFileInfo64(src, &info, filename, FS_MAX_PATH, NULL, 0, NULL, 0);
        if(unzOpenCurrentFile(src) == UNZ_OK)
        {
            if(t)
                t->status->setStatus(ui::getUICString("threadStatusDecompressingFile", 0), filename);

            if(c)
            {
                c->prog->setMax(info.uncompressed_size);
                c->prog->update(0);
                c->offset = 0;
            }

            std::string fullDst = dst + filename;
            fs::mkDirRec(fullDst.substr(0, fullDst.find_last_of('/') + 1));

            unzThrdArgs unzThrd;
            unzThrd.dst = fullDst;
            unzThrd.fileSize = info.uncompressed_size;
            unzThrd.dev = dev;
            unzThrd.writeLimit = (journalSize - 0x100000) < TRANSFER_BUFFER_LIMIT ? (journalSize - 0x100000) : TRANSFER_BUFFER_LIMIT;

            Thread writeThread;
            threadCreate(&writeThread, writeFileFromZip_t, &unzThrd, NULL, 0x8000, 0x2B, 2);
            threadStart(&writeThread);

            std::vector<uint8_t> transferBuffer;
            uint64_t readCount = 0;
            while((readIn = unzReadCurrentFile(src, buff, BUFF_SIZE)) > 0)
            {
                transferBuffer.insert(transferBuffer.end(), buff, buff + readIn);
                readCount += readIn;

                if(c)
                    c->offset += readIn;

                if(transferBuffer.size() >= unzThrd.writeLimit || readCount == info.uncompressed_size)
                {
                    std::unique_lock<std::mutex> buffLock(unzThrd.buffLock);
                    unzThrd.cond.wait(buffLock, [&unzThrd]{ return unzThrd.bufferIsFull == false; });
                    unzThrd.sharedBuffer.assign(transferBuffer.begin(), transferBuffer.end());
                    transferBuffer.clear();
                    unzThrd.bufferIsFull = true;
                    unzThrd.cond.notify_one();
                }
            }
            threadWaitForExit(&writeThread);
            threadClose(&writeThread);
            fs::commitToDevice(dev);
        }
    }
    while(unzGoToNextFile(src) != UNZ_END_OF_LIST_OF_FILE);
    delete[] buff;
}

static void copyZipToDir_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *c = (fs::copyArgs *)t->argPtr;
    fs::copyZipToDir(c->unz, c->dst, c->dev, t);
    if(c->cleanup)
    {
        unzClose(c->unz);
        delete c;
    }
    t->finished = true;
}

void fs::copyZipToDirThreaded(unzFile src, const std::string& dst, const std::string& dev)
{
    fs::copyArgs *send = fs::copyArgsCreate("", dst, dev, NULL, src, true, false, 0);
    ui::newThread(copyZipToDir_t, send, fs::fileDrawFunc);
}

uint64_t fs::getZipTotalSize(unzFile unz)
{
    uint64_t ret = 0;
    if(unzGoToFirstFile(unz) == UNZ_OK)
    {
        unz_file_info64 finfo;
        char filename[FS_MAX_PATH];
        do
        {
            unzGetCurrentFileInfo64(unz, &finfo, filename, FS_MAX_PATH, NULL, 0, NULL, 0);
            ret += finfo.uncompressed_size;
        } while(unzGoToNextFile(unz) != UNZ_END_OF_LIST_OF_FILE);
        unzGoToFirstFile(unz);
    }
    return ret;
}

bool fs::zipNotEmpty(unzFile unz)
{
    return unzGoToFirstFile(unz) == UNZ_OK;
}
