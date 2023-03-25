#include <switch.h>

#include "ui.h"
#include "fs.h"
#include "ldn.h"
#include "util.h"
#include "cfg.h"

static ui::menu *fldMenu = NULL;
ui::slideOutPanel *ui::fldPanel = NULL;
static fs::dirList *fldList = NULL;
static SDL_Texture *fldBuffer;
static unsigned int fldGuideWidth = 0;
static Mutex fldLock = 0;
static std::string driveParent;
static std::vector<drive::gdItem *> driveFldList;

static void fldMenuCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            fs::unmountSave();
            fs::freePathFilters();
            fldMenu->setActive(false);
            ui::fldPanel->closePanel();
            unsigned cusr = data::getCurrentUserIndex();
            ui::ttlSetActive(cusr, true, true);
            ui::updateInput();
            break;
    }
}

static void fldPanelDraw(void *a)
{
    mutexLock(&fldLock);
    SDL_Texture *target = (SDL_Texture *)a;
    gfx::clearTarget(fldBuffer, &ui::slidePanelColor);
    fldMenu->draw(fldBuffer, &ui::txtCont, true);
    gfx::texDraw(target, fldBuffer, 0, 0);
    gfx::drawLine(target, &ui::divClr, 10, 648, fldGuideWidth + 54, 648);
    gfx::drawTextf(target, 18, 32, 673, &ui::txtCont, ui::getUICString("helpFolder", 0));
    mutexUnlock(&fldLock);
}

static void fldFuncCancel(void *a)
{
    std::string *del = (std::string *)a;
    delete del;
}

static void fldFuncOverwrite(void *a)
{
    fs::dirItem *in = (fs::dirItem *)a;
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    std::string *send = new std::string;
    send->assign(util::generatePathByTID(utinfo->tid) + in->getItm());

    ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdOver"], fs::overwriteBackup, fldFuncCancel, send, ui::getUICString("confirmOverwrite", 0), in->getItm().c_str());
    ui::confirm(conf);
}

static void fldFuncDelete(void *a)
{
    fs::dirItem *in = (fs::dirItem *)a;
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    std::string *send = new std::string;
    send->assign(util::generatePathByTID(utinfo->tid) + in->getItm());

    ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdDel"], fs::deleteBackup, fldFuncCancel, send, ui::getUICString("confirmDelete", 0), in->getItm().c_str());
    ui::confirm(conf);
}

static void fldFuncRestore(void *a)
{
    fs::dirItem *in = (fs::dirItem *)a;
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    std::string *send = new std::string;
    send->assign(util::generatePathByTID(utinfo->tid) + in->getItm());

    ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdRest"], fs::restoreBackup, fldFuncCancel, send, ui::getUICString("confirmRestore", 0), in->getItm().c_str());
    ui::confirm(conf);
}

static void fldFuncUpload_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::dirItem *di = (fs::dirItem *)t->argPtr;
    fsSetPriority(FsPriority_Realtime);

    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    data::titleInfo *tinfo = data::getTitleInfoByTID(utinfo->tid);
    std::string path, tmpZip, filename;//Final path to upload from

    if(cfg::config["ovrClk"])
        util::sysBoost();

    //Zip first then upload if folder based backup
    if(di->isDir())
    {
        t->status->setStatus(ui::getUICString("threadStatusCompressingSaveForUpload", 0), di->getItm().c_str());
        filename = di->getItm() + ".zip";
        tmpZip = util::generatePathByTID(utinfo->tid) + di->getItm() + ".zip";
        std::string fldPath = util::generatePathByTID(utinfo->tid) + di->getItm() + "/";

        int zipTrim = util::getTotalPlacesInPath(fs::getWorkDir()) + 2;//Trim path down to save root
        zipFile tmp = zipOpen64(tmpZip.c_str(), 0);
        fs::copyDirToZip(fldPath, tmp, true, zipTrim, NULL);
        zipClose(tmp, NULL);
        path = tmpZip;
    }
    else
    {
        filename = di->getItm();
        path = util::generatePathByTID(utinfo->tid) + di->getItm();
    }

    //Change thread stuff so upload status can be shown
    t->status->setStatus(ui::getUICString("threadStatusUploadingFile", 0), di->getItm().c_str());
    fs::copyArgs *cpyArgs = fs::copyArgsCreate("", "", "", NULL, NULL, false, false, 0);
    cpyArgs->prog->setMax(fs::fsize(path));
    cpyArgs->prog->update(0);
    t->argPtr = cpyArgs;
    t->drawFunc = fs::fileDrawFunc;

    //curlDlArgs
    curlFuncs::curlUpArgs upload;
    upload.f = fopen(path.c_str(), "rb");
    upload.o = &cpyArgs->offset;

    if(fs::gDrive->fileExists(filename))
    {
        std::string id = fs::gDrive->getFileID(filename);
        fs::gDrive->updateFile(id, &upload);
    }
    else
        fs::gDrive->uploadFile(filename, driveParent, &upload);

    fclose(upload.f);

    if(!tmpZip.empty())
        fs::delfile(tmpZip);
    
    fs::copyArgsDestroy(cpyArgs);
    t->drawFunc = NULL;

    if(cfg::config["ovrClk"])
        util::sysNormal();

    ui::fldRefreshMenu();
    
    t->finished = true;
}

static void fldFuncUpload(void *a)
{
    if(fs::gDrive)
        ui::newThread(fldFuncUpload_t, a, NULL);
    else
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popDriveNotActive", 0));
}

static void fldFuncDownload_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    drive::gdItem *in = (drive::gdItem *)t->argPtr;
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    data::titleInfo *tinfo = data::getTitleInfoByTID(utinfo->tid);
    std::string targetPath = util::generatePathByTID(utinfo->tid) + in->name;
    t->status->setStatus(ui::getUICString("threadStatusDownloadingFile", 0), in->name.c_str());
    
    if(cfg::config["ovrClk"])
        util::sysBoost();

    if(fs::fileExists(targetPath))
        fs::delfile(targetPath);

    //Use this for progress bar
    fs::copyArgs *cpy = fs::copyArgsCreate("", "", "", NULL, NULL, false, false, 0);
    cpy->prog->setMax(in->size);
    cpy->prog->update(0);
    t->argPtr = cpy;
    t->drawFunc = fs::fileDrawFunc;

    //DL struct
    curlFuncs::curlDlArgs dlFile;
    dlFile.path = targetPath;
    dlFile.size = in->size;
    dlFile.o = &cpy->offset;

    fs::gDrive->downloadFile(in->id, &dlFile);

    //fclose(dlFile.f);

    fs::copyArgsDestroy(cpy);
    t->drawFunc = NULL;

    if(cfg::config["ovrClk"])
        util::sysNormal();

    ui::fldRefreshMenu();

    t->finished = true;
}

static void fldFuncDownload(void *a)
{
    drive::gdItem *in = (drive::gdItem *)a;
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    std::string testPath = util::generatePathByTID(utinfo->tid) + in->name;
    if(fs::fileExists(testPath))
    {
        ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdOver"], fldFuncDownload_t, NULL, a, ui::getUICString("confirmDriveOverwrite", 0));
        ui::confirm(conf);
    }
    else
        ui::newThread(fldFuncDownload_t, a, NULL);

}

static void LDNRestoreThread(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *cpy = (fs::copyArgs *)t->argPtr;
    LDN::LDNCommunicate *comm = cpy->comm;

    LDN::sendOK(comm->commFD);

    LDN::copyRemoteSaveFile(t);

cleanOut:
    LDN::destroyCommunicate(comm);
    LDN::destroyLDN();
    if (cpy->cleanup)
      fs::copyArgsDestroy(cpy);
    t->finished = true;
}

static void fldFuncLDNRestoreConfirm(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *cpy = (fs::copyArgs *)t->argPtr;

    ui::newThread(LDNRestoreThread, cpy, fs::fileDrawFunc);
    t->finished = true;
}

static void fldFuncLDNCancel(void *a)
{
    fs::copyArgs *cpy = (fs::copyArgs *)a;
    LDN::LDNCommunicate *comm = cpy->comm;

    LDN::sendAbort(comm->commFD);

    LDN::destroyCommunicate(comm);
    LDN::destroyLDN();

    if (cpy->cleanup)
      fs::copyArgsDestroy(cpy);
}

static void fldFuncLDNRestore_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *cpy = (fs::copyArgs *)t->argPtr;
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    LDN::LDNCommunicate *comm = LDN::createCommunicate();
    uint64_t retry = 5;
    bool bindSuccess = false;
    struct sockaddr_in *servaddr;
    LdnNetworkInfo netinfo = {0};
    ui::confirmArgs *conf;
    Result rc;

    t->status->setStatus(ui::getUICString("LDNStatus", 4));
    cpy->offset = 0;
    cpy->prog->setMax(retry);
    cpy->prog->update(0);

    while (cpy->offset < retry) {
        rc = LDN::createLDNClient(comm);
        if (R_SUCCEEDED(rc))
        {
            bindSuccess = true;
            cpy->offset = 5;
            cpy->prog->update(cpy->offset);
            break;
        }

        cpy->offset += 1;
        cpy->prog->update(cpy->offset);
        t->status->setStatus(ui::getUICString("LDNStatus", 1), cpy->offset);
        svcSleepThread(1e+9);
    }

    if (!bindSuccess) {
        goto create_fail;
    }

    rc = ldnGetNetworkInfo(&netinfo);
    if (R_FAILED(rc)) {
        goto getinfo_fail;
    }

    svcSleepThread(1e+9 * 1);   //wait for 1s

    servaddr = &comm->serverAddr;
    memset(servaddr, 0, sizeof(*servaddr));
    servaddr->sin_family      = AF_INET;
    servaddr->sin_port        = htons(SAVE_DATA_SERVER_PORT);
    servaddr->sin_addr.s_addr = htonl(netinfo.nodes[0].ip_addr.addr);

    comm->commFD = LDN::bindServer(&comm->serverAddr);
    if (comm->commFD < 0) {
        goto getinfo_fail;
    }

    cpy->comm = comm;
    conf = ui::confirmArgsCreate(cfg::config["holdOver"],
                                 fldFuncLDNRestoreConfirm, fldFuncLDNCancel,
                                 cpy, ui::getUICString("LDNStatus", 5),
                                 netinfo.nodes[0].nickname);
    ui::confirm(conf);
    t->finished = true;
    return;

getinfo_fail:
    LDN::destroyLDN();
create_fail:
    if (cpy->cleanup)
            fs::copyArgsDestroy(cpy);
    LDN::destroyCommunicate(comm);
    ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("LDNStatus", 6));
    t->finished = true;
}

static void fldFuncLDNBackup_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    fs::copyArgs *cpy = (fs::copyArgs *)t->argPtr;
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    LDN::LDNCommunicate *comm = LDN::createCommunicate();
    uint64_t retry = 10;
    bool bindSuccess = false;
    LdnNetworkInfo netinfo = {0};
    struct sockaddr_in clientAddr;
    struct sockaddr_in *servaddr;
    ui::confirmArgs *conf;
    socklen_t length = sizeof(clientAddr);
        int flag = 1;
    Result rc;

    t->status->setStatus(ui::getUICString("LDNStatus", 0));
    cpy->offset = 0;
    cpy->prog->setMax(retry);
    cpy->prog->update(0);

    rc = LDN::createLDNServer(comm);
    if (R_FAILED(rc)) {
        goto err_out;
    }

    while (cpy->offset < retry) {
        rc = ldnGetNetworkInfo(&netinfo);
        if (R_FAILED(rc)) {
            goto ldn_out;
        }

        // just allow one client
        // it must in index 1, index 0 is ap(me)
        if (netinfo.nodes[1].is_connected)
        {
            bindSuccess = true;
            cpy->offset = 10;
            cpy->prog->update(cpy->offset);
            break;
        }

        cpy->offset += 1;
        cpy->prog->update(cpy->offset);
        t->status->setStatus(ui::getUICString("LDNStatus", 1), cpy->offset);
        svcSleepThread(1e+9);
    }

    if (!bindSuccess) {
        goto ldn_out;
    }

    servaddr = &comm->serverAddr;
    memset(servaddr, 0, sizeof(*servaddr));
    servaddr->sin_family = AF_INET;
    servaddr->sin_port = htons(SAVE_DATA_SERVER_PORT);
    servaddr->sin_addr.s_addr = htons(INADDR_ANY);

    comm->serverFD = socket(PF_INET, SOCK_STREAM, 0);
    if (comm->serverFD < 0) {
        rc = -1;
        goto ldn_out;
    }

    // Port allow reuse
    setsockopt(comm->serverFD, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if (bind(comm->serverFD, (struct sockaddr *)servaddr, sizeof(*servaddr))) {
      rc = -1;
      goto ldn_out;
    }

    if (listen(comm->serverFD, LENGTH_OF_LISTEN_QUEUE)) {
        rc = -1;
        goto ldn_out;
    }

    comm->commFD =
        accept(comm->serverFD, (struct sockaddr *)&clientAddr, &length);
    if (comm->commFD < 0)
        goto ldn_out;

    cpy->comm = comm;
    t->status->setStatus(ui::getUICString("LDNStatus", 2), netinfo.nodes[1].nickname);
    cpy->prog->update(0);

    // wait client sure of send
    rc = LDN::waitForOK(comm->commFD);
    if (!rc)
        goto ldn_out;

    if (fs::dirNotEmpty("sv:/")) {
        LDN::copySaveFileToRemote("sv:/", t);
    } else {
        LDN::sendAbort(comm->commFD);
    }

    LDN::destroyCommunicate(comm);
    LDN::destroyLDN();
    if (cpy->cleanup)
        fs::copyArgsDestroy(cpy);
    t->finished = true;
    return;

ldn_out:
    LDN::destroyCommunicate(comm);
    LDN::destroyLDN();
err_out:
    ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("LDNStatus", 3));
    if (cpy->cleanup)
        fs::copyArgsDestroy(cpy);
    t->finished = true;
}

static void fldFuncLDNBackup(void *a)
{
  fs::copyArgs *send =
      fs::copyArgsCreate("sv:/", "", "", NULL, NULL, true, false, 0);
  ui::newThread(fldFuncLDNBackup_t, send, fs::fileDrawFunc);
}

static void fldFuncLDNRestore(void *a)
{
  fs::copyArgs *recive =
      fs::copyArgsCreate("", "sv:/", "", NULL, NULL, true, false, 0);
  ui::newThread(fldFuncLDNRestore_t, recive, fs::fileDrawFunc);
}

static void fldFuncDriveDelete_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    drive::gdItem *gdi = (drive::gdItem *)t->argPtr;
    t->status->setStatus(ui::getUICString("threadStatusDeletingFile", 0));
    fs::gDrive->deleteFile(gdi->id);
    ui::fldRefreshMenu();
}

static void fldFuncDriveDelete(void *a)
{
    drive::gdItem *in = (drive::gdItem *)a;
    ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdDel"], fldFuncDriveDelete_t, NULL, a, ui::getUICString("confirmDelete", 0), in->name.c_str());
    ui::confirm(conf);
}

static void fldFuncDriveRestore_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    drive::gdItem *gdi = (drive::gdItem *)t->argPtr;
    t->status->setStatus(ui::getUICString("threadStatusDownloadingFile", 0), gdi->name.c_str());

    fs::copyArgs *cpy = fs::copyArgsCreate("", "", "", NULL, NULL, false, false, 0);
    cpy->prog->setMax(gdi->size);
    cpy->prog->update(0);
    t->argPtr = cpy;
    t->drawFunc = fs::fileDrawFunc;

    curlFuncs::curlDlArgs dlFile;
    dlFile.path = "sdmc:/tmp.zip";
    dlFile.size = gdi->size;
    dlFile.o = &cpy->offset;

    fs::gDrive->downloadFile(gdi->id, &dlFile);

    unzFile tmp = unzOpen64("sdmc:/tmp.zip");
    fs::copyZipToDir(tmp, "sv:/", "sv", t);
    unzClose(tmp);
    fs::delfile("sdmc:/tmp.zip");

    fs::copyArgsDestroy(cpy);
    t->argPtr = NULL;
    t->drawFunc = NULL;

    t->finished = true;
}

static void fldFuncDriveRestore(void *a)
{
    drive::gdItem *in = (drive::gdItem *)a;
    ui::confirmArgs *conf = ui::confirmArgsCreate(cfg::config["holdOver"], fldFuncDriveRestore_t, NULL, a, ui::getUICString("confirmRestore", 0), in->name.c_str());
    ui::confirm(conf);
}

void ui::fldInit()
{
    fldGuideWidth = gfx::getTextWidth(ui::getUICString("helpFolder", 0), 18);

    fldMenu = new ui::menu(10, 4, fldGuideWidth + 44, 18, 6);
    fldMenu->setCallback(fldMenuCallback, NULL);
    fldMenu->setActive(false);

    ui::fldPanel = new ui::slideOutPanel(fldGuideWidth + 64, 720, 0, ui::SLD_RIGHT, fldPanelDraw);
    fldBuffer = SDL_CreateTexture(gfx::render, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC | SDL_TEXTUREACCESS_TARGET, fldGuideWidth + 64, 647);
    ui::registerPanel(fldPanel);

    fldList = new fs::dirList;
}

void ui::fldExit()
{
    delete ui::fldPanel;
    delete fldMenu;
    delete fldList;
    SDL_DestroyTexture(fldBuffer);
}

void ui::fldUpdate()
{
    fldMenu->update();
}

void ui::fldPopulateMenu()
{
    mutexLock(&fldLock);

    fldMenu->reset();

    data::userTitleInfo *d = data::getCurrentUserTitleInfo();
    data::titleInfo *t = data::getTitleInfoByTID(d->tid);
    util::createTitleDirectoryByTID(d->tid);
    std::string targetDir = util::generatePathByTID(d->tid);

    fldList->reassign(targetDir);
    fs::loadPathFilters(d->tid);

    fldMenu->addOpt(NULL, ui::getUICString("folderMenuNew", 0));
    fldMenu->optAddButtonEvent(0, HidNpadButton_A, fs::createNewBackup, NULL);

    unsigned fldInd = 1;
    fldMenu->addOpt(NULL, "[LDN]");
    fldMenu->optAddButtonEvent(fldInd, HidNpadButton_A, fldFuncLDNBackup,
                               NULL);
    fldMenu->optAddButtonEvent(fldInd, HidNpadButton_Y, fldFuncLDNRestore,
                               NULL);
    fldInd += 1;

    if(fs::gDrive)
    {
        if(!fs::gDrive->dirExists(t->title, fs::jksvDriveID))
            fs::gDrive->createDir(t->title, fs::jksvDriveID);

        driveParent = fs::gDrive->getDirID(t->title, fs::jksvDriveID);

        fs::gDrive->getListWithParent(driveParent, driveFldList);
        for(unsigned i = 0; i < driveFldList.size(); i++, fldInd++)
        {
            fldMenu->addOpt(NULL, "[GD] " + driveFldList[i]->name);

            fldMenu->optAddButtonEvent(fldInd, HidNpadButton_A, fldFuncDownload, driveFldList[i]);
            fldMenu->optAddButtonEvent(fldInd, HidNpadButton_X, fldFuncDriveDelete, driveFldList[i]);
            fldMenu->optAddButtonEvent(fldInd, HidNpadButton_Y, fldFuncDriveRestore, driveFldList[i]);
        }
    }

    for(unsigned i = 0; i < fldList->getCount(); i++, fldInd++)
    {
        fs::dirItem *di = fldList->getDirItemAt(i);
        fldMenu->addOpt(NULL, di->getItm());

        fldMenu->optAddButtonEvent(fldInd, HidNpadButton_A, fldFuncOverwrite, di);
        fldMenu->optAddButtonEvent(fldInd, HidNpadButton_X, fldFuncDelete, di);
        fldMenu->optAddButtonEvent(fldInd, HidNpadButton_Y, fldFuncRestore, di);
        fldMenu->optAddButtonEvent(fldInd, HidNpadButton_ZR, fldFuncUpload, di);
    }
    fldMenu->setActive(true);
    ui::fldPanel->openPanel();

    mutexUnlock(&fldLock);
}

void ui::fldRefreshMenu()
{
    mutexLock(&fldLock);

    fldMenu->reset();
    data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
    data::titleInfo *tinfo = data::getTitleInfoByTID(utinfo->tid);
    std::string targetDir = util::generatePathByTID(utinfo->tid);

    fldList->reassign(targetDir);
    fldMenu->addOpt(NULL, ui::getUIString("folderMenuNew", 0));
    fldMenu->optAddButtonEvent(0, HidNpadButton_A, fs::createNewBackup, NULL);

    unsigned fldInd = 1;
    fldMenu->addOpt(NULL, "[LDN]");
    fldMenu->optAddButtonEvent(fldInd, HidNpadButton_A, fldFuncLDNBackup,
                               NULL);
    fldMenu->optAddButtonEvent(fldInd, HidNpadButton_Y, fldFuncLDNRestore,
                               NULL);
    fldInd += 1;

    if(fs::gDrive)
    {
        fs::gDrive->getListWithParent(driveParent, driveFldList);

        for(unsigned i = 0; i < driveFldList.size(); i++, fldInd++)
        {
            fldMenu->addOpt(NULL, "[GD] " + driveFldList[i]->name);

            fldMenu->optAddButtonEvent(fldInd, HidNpadButton_A, fldFuncDownload, driveFldList[i]);
            fldMenu->optAddButtonEvent(fldInd, HidNpadButton_X, fldFuncDriveDelete, driveFldList[i]);
            fldMenu->optAddButtonEvent(fldInd, HidNpadButton_Y, fldFuncDriveRestore, driveFldList[i]);
        }
    }

    for(unsigned i = 0; i < fldList->getCount(); i++, fldInd++)
    {
        fs::dirItem *di = fldList->getDirItemAt(i);
        fldMenu->addOpt(NULL, di->getItm());

        fldMenu->optAddButtonEvent(fldInd, HidNpadButton_A, fldFuncOverwrite, di);
        fldMenu->optAddButtonEvent(fldInd, HidNpadButton_X, fldFuncDelete, di);
        fldMenu->optAddButtonEvent(fldInd, HidNpadButton_Y, fldFuncRestore, di);
        fldMenu->optAddButtonEvent(fldInd, HidNpadButton_ZR, fldFuncUpload, di);
    }

    mutexUnlock(&fldLock);
}