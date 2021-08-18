#include <switch.h>

#include "file.h"
#include "util.h"
#include "cfg.h"

static uint64_t getJournalSize(const data::titleInfo *t)
{
    uint64_t journalSize = 0;
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    switch(d->saveInfo.save_data_type)
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
            if(t->nacp.cache_storage_journal_size > 0)
                journalSize = t->nacp.cache_storage_journal_size;
            else
                journalSize = t->nacp.cache_storage_data_and_journal_size_max;
            break;

        default:
            journalSize = BUFF_SIZE;
            break;
    }
    return journalSize;
}

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

void fs::createSaveData_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::svCreateArgs *s = (fs::svCreateArgs *)t->argPtr;

    data::titleInfo *create = data::getTitleInfoByTID(s->tid);
    t->status->setStatus(ui::getUICString("threadStatusCreatingSaveData", 0), create->title.c_str());

    FsSaveDataAttribute attr;
    memset(&attr, 0, sizeof(FsSaveDataAttribute));
    attr.application_id = s->tid;
    attr.uid = s->account;
    attr.system_save_data_id = 0;
    attr.save_data_type = s->type;
    attr.save_data_rank = 0;
    attr.save_data_index = s->index;

    FsSaveDataCreationInfo svCreate;
    memset(&svCreate, 0, sizeof(FsSaveDataCreationInfo));
    int64_t saveSize = 0, journalSize = 0;
    switch(s->type)
    {
        case FsSaveDataType_Account:
            saveSize = create->nacp.user_account_save_data_size;
            journalSize = create->nacp.user_account_save_data_journal_size;
            break;

        case FsSaveDataType_Device:
            saveSize = create->nacp.device_save_data_size;
            journalSize = create->nacp.device_save_data_journal_size;
            break;

        case FsSaveDataType_Bcat:
            saveSize = create->nacp.bcat_delivery_cache_storage_size;
            journalSize = create->nacp.bcat_delivery_cache_storage_size;
            break;

        case FsSaveDataType_Cache:
            saveSize = 32 * 1024 * 1024;//Todo: Add target folder/zip selection for size
            if(create->nacp.cache_storage_journal_size > create->nacp.cache_storage_data_and_journal_size_max)
                journalSize = create->nacp.cache_storage_journal_size;
            else
                journalSize = create->nacp.cache_storage_data_and_journal_size_max;
            break;

        default:
            delete s;
            t->finished = true;
            return;
            break;
    }
    svCreate.save_data_size = saveSize;
    svCreate.journal_size = journalSize;
    svCreate.available_size = 0x4000;
    svCreate.owner_id = create->nacp.save_data_owner_id;
    svCreate.flags = 0;
    svCreate.save_data_space_id = FsSaveDataSpaceId_User;

    FsSaveDataMetaInfo meta;
    memset(&meta, 0, sizeof(FsSaveDataMetaInfo));
    meta.size = 0x40060;
    meta.type = FsSaveDataMetaType_Thumbnail;

    Result res = 0;
    if(R_SUCCEEDED(res = fsCreateSaveDataFileSystem(&attr, &svCreate, &meta)))
    {
        util::createTitleDirectoryByTID(s->tid);
        data::loadUsersTitles(false);
        ui::ttlRefresh();
    }
    else
    {
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("saveDataCreationFailed", 0));
        fs::logWrite("SaveCreate Failed -> %X\n", res);
    }
    delete s;
    t->finished = true;
}

void fs::copyFile_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;
    t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), args->from.c_str());

    args->prog->setMax(fs::fsize(args->from));
    args->prog->update(0);

    uint8_t *buff = new uint8_t[BUFF_SIZE];
    if(cfg::config["directFsCmd"])
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
    if(args->cleanup)
        copyArgsDestroy(args);
    t->finished = true;
}

void fs::copyFileCommit_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;
    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    data::titleInfo *info = data::getTitleInfoByTID(d->tid);
    t->status->setStatus(ui::getUICString("threadStatusCopyingFile", 0), args->from.c_str());

    args->prog->setMax(fs::fsize(args->from));
    args->prog->update(0);

    uint64_t journalSize = getJournalSize(info), writeCount = 0;
    uint8_t *buff = new uint8_t[BUFF_SIZE];

    if(cfg::config["directFsCmd"])
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

    if(args->cleanup)
        copyArgsDestroy(args);
    t->finished = true;
}

void fs::copyDirToDir_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;

    fs::dirList *list = new fs::dirList(args->from);
    for(int i = 0; i < (int)list->getCount(); i++)
    {
        if(pathIsFiltered(args->from + list->getItem(i)))
            continue;

        if(list->isDir(i))
        {
            std::string newSrc = args->from + list->getItem(i) + "/";
            std::string newDst = args->to   + list->getItem(i) + "/";
            fs::mkDir(newDst.substr(0, newDst.length() - 1));

            threadInfo fakeThread;
            fs::copyArgs tmpArgs;
            fakeThread.status = t->status;
            fakeThread.argPtr = &tmpArgs;
            tmpArgs.from = newSrc;
            tmpArgs.to   = newDst;
            tmpArgs.prog = args->prog;
            tmpArgs.cleanup = false;
            fs::copyDirToDir_t(&fakeThread);
        }
        else
        {
            std::string fullSrc = args->from + list->getItem(i);
            std::string fullDst = args->to   + list->getItem(i);

            threadInfo fakeThread;
            fs::copyArgs tmpArgs;
            fakeThread.status = t->status;
            fakeThread.argPtr = &tmpArgs;
            tmpArgs.from = fullSrc;
            tmpArgs.to   = fullDst;
            tmpArgs.prog = args->prog;
            tmpArgs.cleanup = false;
            fs::copyFile_t(&fakeThread);
        }
    }

    delete list;

    if(args->cleanup)
        copyArgsDestroy(args);

    t->finished = true;
}

void fs::copyDirToDirCommit_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;

    fs::dirList *list = new fs::dirList(args->from);
    for(int i = 0; i < (int)list->getCount(); i++)
    {
        if(pathIsFiltered(args->from + list->getItem(i)))
            continue;

        if(list->isDir(i))
        {
            std::string newSrc = args->from + list->getItem(i) + "/";
            std::string newDst = args->to   + list->getItem(i) + "/";
            fs::mkDir(newDst.substr(0, newDst.length() - 1));

            threadInfo fakeThread;
            fs::copyArgs tmpArgs;
            fakeThread.status = t->status;
            fakeThread.argPtr = &tmpArgs;
            tmpArgs.from = newSrc;
            tmpArgs.to   = newDst;
            tmpArgs.dev = args->dev;
            tmpArgs.prog = args->prog;
            tmpArgs.cleanup = false;
            fs::copyDirToDirCommit_t(&fakeThread);
        }
        else
        {
            std::string fullSrc = args->from + list->getItem(i);
            std::string fullDst = args->to   + list->getItem(i);

            threadInfo fakeThread;
            fs::copyArgs tmpArgs;
            fakeThread.status = t->status;
            fakeThread.argPtr = &tmpArgs;
            tmpArgs.from = fullSrc;
            tmpArgs.to   = fullDst;
            tmpArgs.dev = args->dev;
            tmpArgs.prog = args->prog;
            tmpArgs.cleanup = false;
            fs::copyFileCommit_t(&fakeThread);
        }
    }

    delete list;

    if(args->cleanup)
        copyArgsDestroy(args);

    t->finished = true;
}

void fs::copyDirToZip_t(void *a)
{
    threadInfo *t  = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;

    t->status->setStatus(ui::getUICString("threadStatusOpeningFolder", 0), args->from.c_str());
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
            time_t raw;
            time(&raw);
            tm *locTime = localtime(&raw);

            zip_fileinfo inf = { (unsigned)locTime->tm_sec, (unsigned)locTime->tm_min, (unsigned)locTime->tm_hour,
                (unsigned)locTime->tm_mday, (unsigned)locTime->tm_mon, (unsigned)(1900 + locTime->tm_year), 0, 0, 0 };

            std::string filename = args->from + itm;
            size_t devPos = filename.find_first_of('/') + 1;
            t->status->setStatus(ui::getUICString("threadStatusAddingFileToZip", 0), itm.c_str());
            int zOpenFile = zipOpenNewFileInZip64(args->z, filename.substr(devPos, filename.length()).c_str(), &inf, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0);
            if(zOpenFile == ZIP_OK)
            {
                std::string fullFrom = args->from + itm;
                args->prog->setMax(fs::fsize(fullFrom));
                args->prog->update(0);
                args->offset = 0;
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
        if(cfg::config["ovrClk"])
            util::setCPU(util::cpu1224MHz);
        ui::newThread(closeZip_t, args->z, NULL);
        delete args->prog;
        delete args;
    }
    t->finished = true;
}

void fs::copyZipToDir_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    copyArgs *args = (copyArgs *)t->argPtr;

    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    data::titleInfo *tinfo = data::getTitleInfoByTID(d->tid);
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
            t->status->setStatus(ui::getUICString("threadStatusDecompressingFile", 0), filename);
            std::string path = args->to + filename;
            mkDirRec(path.substr(0, path.find_last_of('/') + 1));

            args->prog->setMax(info.uncompressed_size);
            args->prog->update(0);
            args->offset = 0;
            size_t done = 0;
            if(cfg::config["directFsCmd"])
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
        if(cfg::config["ovrClk"])
            util::setCPU(util::cpu1224MHz);
    }
    delete[] buff;
    t->finished = true;
}

void fs::wipesave_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(ui::getUICString("threadStatusResettingSaveData", 0));
    fs::delDir("sv:/");
    fs::commitToDevice("sv");
    t->finished = true;
}

void fs::closeZip_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    zipFile z = t->argPtr;
    zipClose(z, NULL);
    t->finished = true;
}

void fs::backupUserSaves_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *c = (fs::copyArgs *)t->argPtr;
    data::user *u = data::getCurrentUser();

    if(cfg::config["ovrClk"] && cfg::config["zip"])
    {
        util::setCPU(util::cpu1785MHz);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popCPUBoostEnabled", 0));
    }

    for(unsigned i = 0; i < u->titleInfo.size(); i++)
    {
        std::string title = data::getTitleNameByTID(u->titleInfo[i].tid);
        t->status->setStatus(std::string("#" + title + "#").c_str());
        if((ui::padKeysDown() & HidNpadButton_B) || (ui::padKeysHeld() & HidNpadButton_B))
        {
            delete c;
            t->finished = true;
            return;
        }

        bool saveMounted = fs::mountSave(u->titleInfo[i].saveInfo);
        util::createTitleDirectoryByTID(u->titleInfo[i].tid);
        if(saveMounted && cfg::config["zip"] && fs::dirNotEmpty("sv:/"))
        {
            fs::loadPathFilters(u->titleInfo[i].tid);
            std::string outPath = util::generatePathByTID(u->titleInfo[i].tid) + u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + ".zip";
            zipFile zip = zipOpen64(outPath.c_str(), 0);
            threadInfo fakeThread;
            fs::copyArgs tmpArgs;
            tmpArgs.from = "sv:/";
            tmpArgs.z = zip;
            tmpArgs.cleanup = false;
            tmpArgs.prog = c->prog;
            fakeThread.status = t->status;
            fakeThread.argPtr = &tmpArgs;
            copyDirToZip_t(&fakeThread);
            zipClose(zip, NULL);
            fs::freePathFilters();
        }
        else if(saveMounted && fs::dirNotEmpty("sv:/"))
        {
            fs::loadPathFilters(u->titleInfo[i].tid);
            std::string outPath = util::generatePathByTID(u->titleInfo[i].tid) + u->getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + "/";
            fs::mkDir(outPath.substr(0, outPath.length() - 1));
            threadInfo fakeThread;
            fs::copyArgs tmpArgs;
            tmpArgs.from = "sv:/";
            tmpArgs.to = outPath;
            tmpArgs.cleanup = false;
            tmpArgs.prog = c->prog;
            fakeThread.status = t->status;
            fakeThread.argPtr = &tmpArgs;
            copyDirToDir_t(&fakeThread);
            fs::freePathFilters();
        }
        fs::unmountSave();
    }
    delete c;
    if(cfg::config["ovrClk"] && cfg::config["zip"])
        util::setCPU(util::cpu1224MHz);

    t->finished = true;
}
