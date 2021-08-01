#include <string>

#include "ui.h"
#include "file.h"
#include "util.h"
#include "fm.h"

//This is going to be a mess but the old one was too.

//Struct to hold info so i don't have to if else if if if
typedef struct
{
    std::string *path;
    //These can hold the same info needed for the listing menus so might as well use em
    fs::backupArgs *b;
} menuFuncArgs;

static ui::slideOutPanel *devPanel, *sdPanel;
static ui::menu *devMenu, *sdMenu, *devCopyMenu, *sdCopyMenu;
static fs::dirList *devList, *sdList;
static std::string devPath, sdPath, dev;
static menuFuncArgs *devArgs, *sdmcArgs;
static FsSaveDataType type;
static bool commit = false;

//Declarations, implementations down further
static void _listFunctionA(void *a);

/*General stuff*/
static void refreshMenu(ui::menu *m, fs::dirList *d, menuFuncArgs *_args, const std::string& _path)
{
    d->reassign(_path);
    util::copyDirListToMenu(*d, *m);
    for(int i = 1; i < m->getCount(); i++)
    {
        m->optAddButtonEvent(i, HidNpadButton_A, _listFunctionA, _args);
    }
}

/*Callbacks and menu functions*/
static void _devMenuCallback(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    fs::backupArgs *b = (fs::backupArgs *)ma->b;

    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            if(*ma->path != dev && *ma->path != "sdmc:/")
            {
                util::removeLastFolderFromString(*ma->path);
                refreshMenu(b->m, b->d, ma, *ma->path);
            }
            break;

        case HidNpadButton_X:
            devMenu->setActive(false);
            devCopyMenu->setActive(true);
            devPanel->openPanel();
            break;
    }
}

static void _devCopyMenuCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            devCopyMenu->setActive(false);
            devMenu->setActive(true);
            devPanel->closePanel();
            break;
    }
}

static void _sdMenuCallback(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    fs::backupArgs *b = (fs::backupArgs *)ma->b;
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            if(*ma->path != dev && *ma->path != "sdmc:/")
            {
                util::removeLastFolderFromString(*ma->path);
                refreshMenu(b->m, b->d, ma, *ma->path);
            }
            break;

        case HidNpadButton_X:
            sdMenu->setActive(false);
            sdCopyMenu->setActive(true);
            sdPanel->openPanel();
            break;
    }
}

static void _sdCopyMenuCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            sdCopyMenu->setActive(false);
            sdMenu->setActive(true);
            sdPanel->closePanel();
            break;
    }
}

static void _devCopyPanelDraw(void *a)
{
    SDL_Texture *target = (SDL_Texture *)a;
    devCopyMenu->draw(target, &ui::txtCont, true);
}

static void _sdCopyPanelDraw(void *a)
{
    SDL_Texture *target = (SDL_Texture *)a;
    sdCopyMenu->draw(target, &ui::txtCont, true);
}

static void _listFunctionA(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    fs::backupArgs *b = (fs::backupArgs *)ma->b;

    int sel = b->m->getSelected();
    bool isDir = b->d->isDir(sel - 2);
    if(sel == 1 && (*ma->path != dev && *ma->path != "sdmc:/"))
    {
        util::removeLastFolderFromString(*ma->path);
        b->d->reassign(*ma->path);
        util::copyDirListToMenu(*b->d, *b->m);
    }
    else if(sel > 1 && isDir)
    {
        std::string addToPath = b->d->getItem(sel - 2);
        *ma->path += addToPath + "/";
        b->d->reassign(*ma->path);
        util::copyDirListToMenu(*b->d, *b->m);
    }

    for(int i = 1; i < b->m->getCount(); i++)
    {
        b->m->optAddButtonEvent(i, HidNpadButton_A, _listFunctionA, ma);
    }
}

void ui::fmInit()
{
    //This needs to be done in a strange order so everything works
    devArgs = new menuFuncArgs;
    devArgs->b = new fs::backupArgs;
    devArgs->path = &devPath;

    sdmcArgs = new menuFuncArgs;
    sdmcArgs->b = new fs::backupArgs;
    sdmcArgs->path = &sdPath;

    devMenu = new ui::menu;
    devMenu->setCallback(_devMenuCallback, devArgs);
    devMenu->setParams(10, 8, 590, 18, 6);
    devMenu->setActive(true);
    devArgs->b->m = devMenu;

    devPanel = new ui::slideOutPanel(260, 720, 0, ui::SLD_LEFT, _devCopyPanelDraw);
    devCopyMenu = new ui::menu;
    devCopyMenu->setActive(false);
    devCopyMenu->setCallback(_devCopyMenuCallback, NULL);
    devCopyMenu->setParams(10, 236, 246, 20, 5);
    devCopyMenu->addOpt(NULL, ui::advMenuStr[0] + "SDMC");
    for(int i = 1; i < 6; i++)
        devCopyMenu->addOpt(NULL, advMenuStr[i]);
    ui::registerPanel(devPanel);

    sdMenu = new ui::menu;
    sdMenu->setCallback(_sdMenuCallback, sdmcArgs);
    sdMenu->setParams(620, 8, 590, 18, 6);
    sdMenu->setActive(false);
    sdmcArgs->b->m = sdMenu;

    sdPanel = new ui::slideOutPanel(260, 720, 0, ui::SLD_RIGHT, _sdCopyPanelDraw);
    sdCopyMenu = new ui::menu;
    sdCopyMenu->setActive(false);
    sdCopyMenu->setCallback(_sdCopyMenuCallback, NULL);
    sdCopyMenu->setParams(10, 236, 246, 20, 5);
    for(int i = 0; i < 6; i++)
        sdCopyMenu->addOpt(NULL, advMenuStr[i]);
    ui::registerPanel(sdPanel);

    devList = new fs::dirList;
    sdList  = new fs::dirList;
    devArgs->b->d = devList;
    sdmcArgs->b->d = sdList;
}

void ui::fmExit()
{
    delete devMenu;
    delete sdMenu;
    delete devCopyMenu;
    delete sdCopyMenu;
    delete devList;
    delete sdList;
    delete devArgs->b;
    delete devArgs;
    delete sdmcArgs->b;
    delete sdmcArgs;
}

void ui::fmPrep(const FsSaveDataType& _type, const std::string& _dev, bool _commit)
{
    type = _type;
    dev  = _dev;
    commit = _commit;
    devPath = _dev;
    sdPath = "sdmc:/";

    sdCopyMenu->editOpt(0, NULL, ui::advMenuStr[0] + _dev);

    devList->reassign(dev);
    sdList->reassign(sdPath);
    util::copyDirListToMenu(*devList, *devMenu);
    for(int i = 1; i < devMenu->getCount(); i++)
    {
        devMenu->optAddButtonEvent(i, HidNpadButton_A, _listFunctionA, devArgs);
    }

    util::copyDirListToMenu(*sdList, *sdMenu);
    for(int i = 1; i < sdMenu->getCount(); i++)
    {
        sdMenu->optAddButtonEvent(i, HidNpadButton_A, _listFunctionA, sdmcArgs);
    }
}

void ui::fmUpdate()
{
    //For now? Maybe forever?
    if(devMenu->getActive() || sdMenu->getActive())
    {
        switch(ui::padKeysDown())
        {
            case HidNpadButton_ZL:
            case HidNpadButton_ZR:
                if(devMenu->getActive())
                {
                    devMenu->setActive(false);
                    sdMenu->setActive(true);
                }
                else
                {
                    devMenu->setActive(true);
                    sdMenu->setActive(false);
                }
                break;

            case HidNpadButton_Minus:
                //Can't be 100% sure it's fs's sv
                if(dev != "sdmc:/")
                    fsdevUnmountDevice(dev.c_str());

                if(ui::prevState == EX_MNU)
                {
                    ui::usrSelPanel->openPanel();
                    ui::changeState(EX_MNU);
                }
                else if(ui::prevState == TTL_SEL)
                {
                    ui::usrSelPanel->openPanel();
                    ui::ttlOptsPanel->openPanel();
                    ui::changeState(TTL_SEL);
                }
                break;
        }
    }
    devMenu->update();
    sdMenu->update();
    devCopyMenu->update();
    sdCopyMenu->update();
}

void ui::fmDraw(SDL_Texture *target)
{
    devMenu->draw(target, &ui::txtCont, true);
    sdMenu->draw(target, &ui::txtCont, true);
    gfx::drawLine(target, &ui::divClr, 610, 0, 610, 559);
    gfx::drawTextfWrap(NULL, 14, 30, 654, 600, &ui::txtCont, devPath.c_str());
    gfx::drawTextfWrap(NULL, 14, 640, 654, 600, &ui::txtCont, sdPath.c_str());
}
