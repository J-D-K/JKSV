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
    ui::extMenu->setParams(32, 32, 1016, 24, 5);
    ui::extMenu->setCallback(extMenuCallback, NULL);
    ui::extMenu->setActive(false);
    for(unsigned i = 0; i < 11; i++)
        ui::extMenu->addOpt(NULL, ui::exMenuStr[i]);

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
