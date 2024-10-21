#include "FsLib.hpp"
#include "cfg.h"
#include "fs.h"
#include "util.h"
#include <condition_variable>
#include <mutex>
#include <switch.h>
#include <time.h>
#include <vector>

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
    while (written < in->fileSize)
    {
        std::unique_lock<std::mutex> buffLock(in->buffLock);
        in->cond.wait(buffLock, [in] { return in->bufferIsFull; });
        localBuffer.clear();
        localBuffer.assign(in->sharedBuffer.begin(), in->sharedBuffer.end());
        in->sharedBuffer.clear();
        in->bufferIsFull = false;
        buffLock.unlock();
        in->cond.notify_one();

        written += fwrite(localBuffer.data(), 1, localBuffer.size(), out);
        journalCount += written;
        if (journalCount >= in->writeLimit)
        {
            journalCount = 0;
            fclose(out);
            fs::commitToDevice(in->dev);
            out = fopen(in->dst.c_str(), "ab");
        }
    }
    fclose(out);
}

void fs::copyDirToZip(const std::string &src, zipFile dst, bool trimPath, int trimPlaces, threadInfo *t)
{
    FsLib::Directory List(src);
    if (!List.IsOpen() || List.GetEntryCount() <= 0)
    {
        return;
    }

    for (int64_t i = 0; i < List.GetEntryCount(); i++)
    {
        if (fs::pathIsFiltered(src + List.GetEntryNameAt(i)))
        {
            continue;
        }

        if (List.EntryAtIsDirectory(i))
        {
            std::string NewSource = src + List.GetEntryNameAt(i) + "/";
            fs::copyDirToZip(NewSource, dst, trimPath, trimPlaces, t);
        }
        else
        {
            std::time_t Timer;
            std::time(&Timer);
            std::tm *LocalTime = std::localtime(&Timer);

            zip_fileinfo ZipFileInfo = {.tmz_date = {.tm_sec = LocalTime->tm_sec,
                                                     .tm_min = LocalTime->tm_min,
                                                     .tm_hour = LocalTime->tm_hour,
                                                     .tm_mday = LocalTime->tm_mday,
                                                     .tm_mon = LocalTime->tm_mon,
                                                     .tm_year = LocalTime->tm_year + 1900},
                                        .dosDate = 0,
                                        .internal_fa = 0,
                                        .external_fa = 0};

            size_t ZipFileNameStart = 0;
            std::string ZipFileName = src + List.GetEntryNameAt(i);
            if (trimPath)
            {
                util::trimPath(ZipFileName, trimPlaces);
            }
            else
            {
                ZipFileNameStart = ZipFileName.find_first_of("/") + 1;
            }

            int ZipError = zipOpenNewFileInZip64(dst,
                                                 ZipFileName.substr(ZipFileNameStart, ZipFileName.length()),
                                                 &ZipFileInfo,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 Z_DEFLATED,
                                                 Z_DEFAULT_COMPRESSION,
                                                 0);
            if (ZipError == ZIP_OK)
            {
                // I really don't like the following code, but again, I don't want to spend too much time on fixing up master branch to work with FsLib
                fs::copyArgs *CopyArgs = NULL;
                if (t)
                {
                    t->status->setStatus(ui::getUICString("threadStatusAddingFileToZip", 0), List.GetEntryNameAt(i).c_str());
                    if (t->argPtr)
                    {
                        CopyArgs = reinterpret_cast<fs::copyArgs *>(t->argPtr);
                    }
                }

                if (CopyArgs)
                {
                    CopyArgs->offset = 0;
                    CopyArgs->prog->setMax(List.GetEntrySizeAt(i));
                    CopyArgs->prog->update(0);
                }

                std::string FullSource = src + List.GetEntryNameAt(i);
                FsLib::File SourceFile(FullSource, FsOpenMode_Read);
                std::unique_ptr<uint8_t[]> FileBuffer(new uint8_t[ZIP_BUFF_SIZE]);
                if (!SourceFile.IsOpen() || !FileBuffer)
                {
                    zipCloseFileInZip(dst);
                    return;
                }

                size_t BytesRead = 0;
                while ((BytesRead = SourceFile.Read(FileBuffer.get(), ZIP_BUFF_SIZE)) > 0)
                {
                    zipWriteInFileInZip(dst, FileBuffer.get(), BytesRead);

                    if (CopyArgs)
                    {
                        CopyArgs->offset += BytesRead;
                    }
                }
            }
        }
    }
}

void copyDirToZip_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *c = (fs::copyArgs *)t->argPtr;
    if (cfg::config["ovrClk"])
    {
        util::sysBoost();
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popCPUBoostEnabled", 0));
    }

    fs::copyDirToZip(c->src, c->z, c->trimZipPath, c->trimZipPlaces, t);

    if (cfg::config["ovrClk"])
        util::sysNormal();

    if (c->cleanup)
    {
        zipClose(c->z, NULL);
        delete c;
    }
    t->finished = true;
}

void fs::copyDirToZipThreaded(const std::string &src, zipFile dst, bool trimPath, int trimPlaces)
{
    fs::copyArgs *send = fs::copyArgsCreate(src, "", "", dst, NULL, true, false, 0);
    ui::newThread(copyDirToZip_t, send, fs::fileDrawFunc);
}

void fs::copyZipToDir(unzFile src, const std::string &dst, const std::string &dev, threadInfo *t)
{
    fs::copyArgs *c = NULL;
    if (t)
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
        if (unzOpenCurrentFile(src) == UNZ_OK)
        {
            if (t)
                t->status->setStatus(ui::getUICString("threadStatusDecompressingFile", 0), filename);

            if (c)
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
            while ((readIn = unzReadCurrentFile(src, buff, BUFF_SIZE)) > 0)
            {
                transferBuffer.insert(transferBuffer.end(), buff, buff + readIn);
                readCount += readIn;

                if (c)
                    c->offset += readIn;

                if (transferBuffer.size() >= unzThrd.writeLimit || readCount == info.uncompressed_size)
                {
                    std::unique_lock<std::mutex> buffLock(unzThrd.buffLock);
                    unzThrd.cond.wait(buffLock, [&unzThrd] { return unzThrd.bufferIsFull == false; });
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
    } while (unzGoToNextFile(src) != UNZ_END_OF_LIST_OF_FILE);
    delete[] buff;
}

static void copyZipToDir_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *c = (fs::copyArgs *)t->argPtr;
    fs::copyZipToDir(c->unz, c->dst, c->dev, t);
    if (c->cleanup)
    {
        unzClose(c->unz);
        delete c;
    }
    t->finished = true;
}

void fs::copyZipToDirThreaded(unzFile src, const std::string &dst, const std::string &dev)
{
    fs::copyArgs *send = fs::copyArgsCreate("", dst, dev, NULL, src, true, false, 0);
    ui::newThread(copyZipToDir_t, send, fs::fileDrawFunc);
}

uint64_t fs::getZipTotalSize(unzFile unz)
{
    uint64_t ret = 0;
    if (unzGoToFirstFile(unz) == UNZ_OK)
    {
        unz_file_info64 finfo;
        char filename[FS_MAX_PATH];
        do
        {
            unzGetCurrentFileInfo64(unz, &finfo, filename, FS_MAX_PATH, NULL, 0, NULL, 0);
            ret += finfo.uncompressed_size;
        } while (unzGoToNextFile(unz) != UNZ_END_OF_LIST_OF_FILE);
        unzGoToFirstFile(unz);
    }
    return ret;
}

bool fs::zipNotEmpty(unzFile unz)
{
    return unzGoToFirstFile(unz) == UNZ_OK;
}
