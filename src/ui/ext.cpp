#include <switch.h>
#include <SDL.h>

#include "ui.h"
#include "file.h"

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

void ui::extInit()
{
    ui::extMenu = new ui::menu;
    ui::extMenu->setParams(200, 32, 1016, 24, 5);
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
