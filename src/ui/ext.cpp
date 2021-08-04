#include <switch.h>
#include <SDL.h>

#include "ui.h"
#include "file.h"
#include "util.h"

ui::menu *ui::extMenu;

static void extMenuCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            ui::extMenu->setActive(false);
            ui::usrMenu->setActive(true);
            ui::changeState(USR_SEL);
            break;
    }
}

static inline void closeUserPanel()
{
    ui::usrMenu->setActive(false);
    ui::usrSelPanel->closePanel();
}

static void toFMSDtoSD(void *a)
{
    ui::fmPrep(FsSaveDataType_Account, "sdmc:/", false);
    closeUserPanel();
    ui::changeState(FIL_MDE);
}

static void toFMProdInfoF(void *a)
{
    FsFileSystem prodf;
    fsOpenBisFileSystem(&prodf, FsBisPartitionId_CalibrationFile, "");
    fsdevMountDevice("prod-f", prodf);
    closeUserPanel();
    ui::fmPrep(FsSaveDataType_System, "prod-f:/", false);
    ui::changeState(FIL_MDE);
}

static void toFMSafe(void *a)
{
    FsFileSystem safe;
    fsOpenBisFileSystem(&safe, FsBisPartitionId_SafeMode, "");
    fsdevMountDevice("safe", safe);
    closeUserPanel();
    ui::fmPrep(FsSaveDataType_System, "safe:/", false);
    ui::changeState(FIL_MDE);
}

static void toFMSystem(void *a)
{
    FsFileSystem sys;
    fsOpenBisFileSystem(&sys, FsBisPartitionId_System, "");
    fsdevMountDevice("sys", sys);
    closeUserPanel();
    ui::fmPrep(FsSaveDataType_System, "sys:/", false);
    ui::changeState(FIL_MDE);
}

static void toFMUser(void *a)
{
    FsFileSystem user;
    fsOpenBisFileSystem(&user, FsBisPartitionId_User, "");
    fsdevMountDevice("user", user);
    closeUserPanel();
    ui::fmPrep(FsSaveDataType_System, "user:/", false);
    ui::changeState(FIL_MDE);
}

static void _delUpdate(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("Deleting update from NAND...");
    FsFileSystem sys;
    fsOpenBisFileSystem(&sys, FsBisPartitionId_System, "");
    fsdevMountDevice("sys", sys);
    fs::delDir("sys:/Contents/placehld/");
    fsdevUnmountDevice("sys");
    t->finished = true;
}

static void extMenuOptRemoveUpdate(void *a)
{
    ui::newThread(_delUpdate, NULL, NULL);
}

static void extMenuTerminateProcess(void *a)
{
    std::string idStr = util::getStringInput(SwkbdType_QWERTY, "0100000000000000", "Enter Process ID", 18, 0, NULL);
    if(!idStr.empty())
    {
        uint64_t termID = std::strtoull(idStr.c_str(), NULL, 16);
        if(R_SUCCEEDED(pmshellTerminateProgram(termID)))
            ui::showPopMessage(POP_FRAME_DEFAULT, "Process %s successfully shutdown.", idStr.c_str());
    }
}

static void extMenuMountSysSave(void *a)
{
    FsFileSystem sys;
    std::string idStr = util::getStringInput(SwkbdType_QWERTY, "8000000000000000", "Enter Sys Save ID", 18, 0, NULL);
    uint64_t mountID = std::strtoull(idStr.c_str(), NULL, 16);
    if(R_SUCCEEDED(fsOpen_SystemSaveData(&sys, FsSaveDataSpaceId_System, mountID, (AccountUid) {0})))
    {
        fsdevMountDevice("sv", sys);
        ui::fmPrep(FsSaveDataType_System, "sv:/", true);
        ui::usrSelPanel->closePanel();
        ui::changeState(FIL_MDE);
    }
}

//Todo: Not so simple now.
static void extMenuReloadTitles(void *a)
{

}

static void extMenuMountRomFS(void *a)
{
    FsFileSystem tromfs;
    if(R_SUCCEEDED(fsOpenDataFileSystemByCurrentProcess(&tromfs)))
    {
        fsdevMountDevice("tromfs", tromfs);
        ui::fmPrep(FsSaveDataType_System, "tromfs:/", false);
        ui::usrSelPanel->closePanel();
        ui::changeState(FIL_MDE);
    }
}

void ui::extInit()
{
    ui::extMenu = new ui::menu;
    ui::extMenu->setParams(200, 24, 1002, 24, 4);
    ui::extMenu->setCallback(extMenuCallback, NULL);
    ui::extMenu->setActive(false);
    for(unsigned i = 0; i < 11; i++)
        ui::extMenu->addOpt(NULL, ui::exMenuStr[i]);

    //SD to SD
    ui::extMenu->optAddButtonEvent(0, HidNpadButton_A, toFMSDtoSD, NULL);
    //Prodinfo-F
    ui::extMenu->optAddButtonEvent(1, HidNpadButton_A, toFMProdInfoF, NULL);
    //Safe
    ui::extMenu->optAddButtonEvent(2, HidNpadButton_A, toFMSafe, NULL);
    //System
    ui::extMenu->optAddButtonEvent(3, HidNpadButton_A, toFMSystem, NULL);
    //User
    ui::extMenu->optAddButtonEvent(4, HidNpadButton_A, toFMUser, NULL);
    //Del update
    ui::extMenu->optAddButtonEvent(5, HidNpadButton_A, extMenuOptRemoveUpdate, NULL);
    //Terminate Process
    ui::extMenu->optAddButtonEvent(6, HidNpadButton_A, extMenuTerminateProcess, NULL);
    //Mount system save
    ui::extMenu->optAddButtonEvent(7, HidNpadButton_A, extMenuMountSysSave, NULL);
    //Rescan
    ui::extMenu->optAddButtonEvent(8, HidNpadButton_A, extMenuReloadTitles, NULL);
    //RomFS
    ui::extMenu->optAddButtonEvent(9, HidNpadButton_A, extMenuMountRomFS, NULL);
}

void ui::extExit()
{
    delete ui::extMenu;
}

void ui::extUpdate()
{
    ui::extMenu->update();
}

void ui::extDraw(SDL_Texture *target)
{
    ui::extMenu->draw(target, &ui::txtCont, true);
}
