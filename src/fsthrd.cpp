#include <switch.h>

#include "file.h"
#include "util.h"

static uint64_t getJournalSize(const data::titleInfo *t)
{
    uint64_t journalSize = 0;
    switch(data::curData.saveInfo.save_data_type)
    {
        case FsSaveDataType_Account:
            journalSize = t->nacp.user_account_save_data_journal_size;
            break;

        case FsSaveDataType_Device:
            journalSize = t->nacp.device_save_data_journal_size;
            break;

        case FsSaveDataType_Bcat:
            journalSize = t->nacp.bcat_delivery_cache_storage_size;
            break;

        case FsSaveDataType_Cache:
            journalSize = t->nacp.cache_storage_journal_size;
            break;

        default:
            journalSize = BUFF_SIZE;
            break;
    }
    return journalSize;
}

//Todo: Weird flickering?
void fs::_fileDrawFunc(void *a)
{
    threadInfo *t = (threadInfo *)a;
    if(!t->finished)
    {
        copyArgs *c = (copyArgs *)t->argPtr;
        std::string tmp;
        t->status->getStatus(tmp);
        c->argLock();
        c->prog->draw(tmp);
        c->argUnlock();
    }
}

void fs::copyfile_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;
    t->status->setStatus("Copying '" + args->from + "'...");

    uint8_t *buff = new uint8_t[BUFF_SIZE];
    if(data::config["directFsCmd"])
    {
        FSFILE *in = fsfopen(args->from.c_str(), FsOpenMode_Read);
        FSFILE *out = fsfopen(args->to.c_str(), FsOpenMode_Write);

        if(!in || !out)
        {
            fsfclose(in);
            fsfclose(out);
            t->finished = true;
            return;
        }
        size_t readIn = 0;
        while((readIn = fsfread(buff, 1, BUFF_SIZE, in)) > 0)
        {
            fsfwrite(buff, 1, readIn, out);
            args->argLock();
            args->offset = in->offset;
            args->prog->update(args->offset);
            args->argUnlock();
        }
        fsfclose(in);
        fsfclose(out);
    }
    else
    {
        FILE *in = fopen(args->from.c_str(), "rb");
        FILE *out = fopen(args->to.c_str(), "wb");
        if(!in || !out)
        {
            fclose(in);
            fclose(out);
            t->finished = true;
            return;
        }

        size_t readIn = 0;
        while((readIn = fread(buff, 1, BUFF_SIZE, in)) > 0)
        {
            fwrite(buff, 1, readIn, out);
            args->argLock();
            args->offset = ftell(in);
            args->prog->update(args->offset);
            args->argUnlock();
        }
        fclose(in);
        fclose(out);
    }
    delete[] buff;
    copyArgsDestroy(args);
    t->finished = true;
}

void fs::copyFileCommit_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;
    data::titleInfo *info = data::getTitleInfoByTID(data::curData.saveID);
    t->status->setStatus("Copying '" + args->from + "'...");

    uint64_t journalSize = getJournalSize(info), writeCount = 0;
    uint8_t *buff = new uint8_t[BUFF_SIZE];

    if(data::config["directFsCmd"])
    {
        FSFILE *in  = fsfopen(args->from.c_str(), FsOpenMode_Read);
        FSFILE *out = fsfopen(args->to.c_str(), FsOpenMode_Write);

        if(!in || !out)
        {
            fsfclose(in);
            fsfclose(out);
            t->finished = true;
            return;
        }

        size_t readIn = 0;
        while((readIn = fsfread(buff, 1, BUFF_SIZE, in)) > 0)
        {
            fsfwrite(buff, 1, readIn, out);
            writeCount += readIn;
            if(writeCount >= (journalSize - 0x100000))
            {
                writeCount = 0;
                fsfclose(out);
                if(!commitToDevice(args->dev))
                    break;

                out = fsfopen(args->to.c_str(), FsOpenMode_Write | FsOpenMode_Append);
            }
            args->argLock();
            args->offset = out->offset;
            args->prog->update(args->offset);
            args->argUnlock();
        }
        fsfclose(in);
        fsfclose(out);
    }
    else
    {
        FILE *in = fopen(args->from.c_str(), "rb");
        FILE *out = fopen(args->to.c_str(), "wb");

        if(!in || !out)
        {
            fclose(in);
            fclose(out);
            t->finished = true;
            return;
        }

        size_t readIn = 0;
        while((readIn = fread(buff, 1, BUFF_SIZE, in)) > 0)
        {
            fwrite(buff, 1, readIn, out);
            writeCount += readIn;
            if(writeCount >= (journalSize - 0x100000))
            {
                writeCount = 0;
                fclose(out);
                if(!commitToDevice(args->dev))
                    break;

                out = fopen(args->to.c_str(), "ab");
            }
            args->argLock();
            args->offset = ftell(out);
            args->prog->update(args->offset);
            args->argUnlock();
        }
        fclose(in);
        fclose(out);
    }
    delete[] buff;

    commitToDevice(args->dev.c_str());
    copyArgsDestroy(args);
    t->finished = true;
}

void fs::copyDirToZip_t(void *a)
{
    threadInfo *t  = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;

    t->status->setStatus("Opening " + args->from + "...");
    fs::dirList *list = new fs::dirList(args->from);

    unsigned listTotal = list->getCount();
    for(unsigned i = 0; i < listTotal; i++)
    {
        std::string itm = list->getItem(i);
        if(fs::pathIsFiltered(args->from + itm))
            continue;

        if(list->isDir(i))
        {
            std::string newFrom = args->from + itm + "/";
            //Fake thread and new args to point to src thread stuff
            //This wouldn't work spawning a new thread.
            threadInfo tmpThread;
            tmpThread.status = t->status;
            copyArgs tmpArgs;
            tmpArgs.from = newFrom;
            tmpArgs.prog = args->prog;
            tmpArgs.z = args->z;
            tmpArgs.cleanup = false;
            tmpThread.argPtr = &tmpArgs;
            copyDirToZip_t(&tmpThread);
        }
        else
        {
            zip_fileinfo inf = {0};
            std::string filename = args->from + itm;
            size_t devPos = filename.find_first_of('/') + 1;
            t->status->setStatus("Adding '" + itm + "' to ZIP.");
            int zOpenFile = zipOpenNewFileInZip64(args->z, filename.substr(devPos, filename.length()).c_str(), &inf, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0);
            if(zOpenFile == ZIP_OK)
            {
                std::string fullFrom = args->from + itm;
                args->offset = 0;
                args->fileSize = fs::fsize(fullFrom);
                args->prog->setMax(args->fileSize);
                args->prog->update(0);
                FILE *cpy = fopen(fullFrom.c_str(), "rb");
                size_t readIn = 0;
                uint8_t *buff = new uint8_t[BUFF_SIZE];
                while((readIn = fread(buff, 1, BUFF_SIZE, cpy)) > 0)
                {
                    zipWriteInFileInZip(args->z, buff, readIn);
                    args->offset += readIn;
                    args->prog->update(args->offset);
                }
                delete[] buff;
                fclose(cpy);
                zipCloseFileInZip(args->z);
            }
        }
    }
    delete list;
    if(args->cleanup)
    {
        if(data::config["ovrClk"])
            util::setCPU(1224000000);
        ui::newThread(closeZip_t, args->z, NULL);
        delete args->prog;
        delete args;
    }
    t->finished = true;
}

void fs::closeZip_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    zipFile z = t->argPtr;
    zipClose(z, NULL);
    t->finished = true;
}

void fs::copyZipToDir_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;

    data::titleInfo *tinfo = data::getTitleInfoByTID(data::curData.saveID);
    uint64_t journalSize = getJournalSize(tinfo), writeCount = 0;
    char filename[FS_MAX_PATH];
    uint8_t *buff = new uint8_t[BUFF_SIZE];
    int readIn = 0;
    unz_file_info64 info;
    do
    {
        unzGetCurrentFileInfo64(args->unz, &info, filename, FS_MAX_PATH, NULL, 0, NULL, 0);
        if(unzOpenCurrentFile(args->unz) == UNZ_OK)
        {
            t->status->setStatus("Copying '" + std::string(filename) + "'...");
            std::string path = args->to + filename;
            mkdirRec(path.substr(0, path.find_last_of('/') + 1));

            args->fileSize = info.uncompressed_size;
            args->offset = 0.0f;
            args->prog->setMax(args->fileSize);
            size_t done = 0;
            if(data::config["directFsCmd"])
            {
                FSFILE *out = fsfopen(path.c_str(), FsOpenMode_Write);
                while((readIn = unzReadCurrentFile(args->unz, buff, BUFF_SIZE)) > 0)
                {
                    done += readIn;
                    writeCount += readIn;
                    args->offset += readIn;
                    args->prog->update(args->offset);
                    fsfwrite(buff, 1, readIn, out);
                    if(writeCount >= (journalSize - 0x100000))
                    {
                        writeCount = 0;
                        fsfclose(out);
                        if(!commitToDevice(args->dev.c_str()))
                            break;

                        out = fsfopen(path.c_str(), FsOpenMode_Write | FsOpenMode_Append);
                    }
                }
                fsfclose(out);
            }
            else
            {
                FILE *out = fopen(path.c_str(), "wb");

                while((readIn = unzReadCurrentFile(args->unz, buff, BUFF_SIZE)) > 0)
                {
                    done += readIn;
                    writeCount += readIn;
                    args->offset += readIn;
                    args->prog->update(args->offset);
                    fwrite(buff, 1, readIn, out);
                    if(writeCount >= (journalSize - 0x100000))
                    {
                        writeCount = 0;
                        fclose(out);
                        if(!commitToDevice(args->dev.c_str()))
                            break;

                        out = fopen(path.c_str(), "ab");
                    }
                }
                fclose(out);
            }
            unzCloseCurrentFile(args->unz);
            commitToDevice(args->dev.c_str());
        }
    }
    while(unzGoToNextFile(args->unz) != UNZ_END_OF_LIST_OF_FILE);

    if(args->cleanup)
    {
        unzClose(args->unz);
        copyArgsDestroy(args);
        if(data::config["ovrClk"])
            util::setCPU(1224000000);
    }
    delete[] buff;
    t->finished = true;
}
