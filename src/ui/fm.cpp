#include <string>

#include "ui.h"
#include "file.h"
#include "util.h"
#include "fm.h"
#include "cfg.h"
#include "type.h"

//This is going to be a mess but the old one was too.
//It gets more complicated because of system saves and committing.

//Major Todo: have thread status update
//Todo: Not have menus completely reset.

//Struct to hold info so i don't have to if else if if if
typedef struct
{
    std::string *path;
    ui::menu *m;
    fs::dirList *d;
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
static void refreshMenu(void *a)
{
    threadInfo *t = (threadInfo *)a;
    menuFuncArgs *ma = (menuFuncArgs *)t->argPtr;
    ui::menu *m = ma->m;
    fs::dirList *d = ma->d;

    d->reassign(*ma->path);
    util::copyDirListToMenu(*d, *m);
    for(int i = 1; i < m->getCount(); i++)
    {
        m->optAddButtonEvent(i, HidNpadButton_A, _listFunctionA, ma);
    }
    t->finished = true;
}

/*Callbacks and menu functions*/
static void _devMenuCallback(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            if(*ma->path != dev && *ma->path != "sdmc:/")
            {
                util::removeLastFolderFromString(*ma->path);
                threadInfo fake;
                fake.argPtr = ma;
                refreshMenu(&fake);
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
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            if(*ma->path != dev && *ma->path != "sdmc:/")
            {
                util::removeLastFolderFromString(*ma->path);
                threadInfo fake;
                fake.argPtr = ma;
                refreshMenu(&fake);
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

//These draw to the panel texture passed
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
    ui::menu *m = ma->m;
    fs::dirList *d = ma->d;

    int sel = m->getSelected();
    bool isDir = d->isDir(sel - 2);
    if(sel == 1 && (*ma->path != dev && *ma->path != "sdmc:/"))
    {
        util::removeLastFolderFromString(*ma->path);
        d->reassign(*ma->path);
        util::copyDirListToMenu(*d, *m);
    }
    else if(sel > 1 && isDir)
    {
        std::string addToPath = d->getItem(sel - 2);
        *ma->path += addToPath + "/";
        d->reassign(*ma->path);
        util::copyDirListToMenu(*d, *m);
    }

    for(int i = 1; i < m->getCount(); i++)
        m->optAddButtonEvent(i, HidNpadButton_A, _listFunctionA, ma);
}

static void _copyMenuUpload(void *a)
{

}

static void _copyMenuCopy_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    menuFuncArgs *ma = (menuFuncArgs *)t->argPtr;
    ui::menu *m = ma->m;
    fs::dirList *d = ma->d;

    int sel = m->getSelected();
    if(ma == devArgs)
    {
        if(sel == 0)
            fs::copyDirToDirThreaded(*ma->path, *sdmcArgs->path);
        else if(sel > 1 && d->isDir(sel - 2))
        {
            std::string srcPath = *ma->path + d->getItem(sel - 2) + "/";
            std::string dstPath = *sdmcArgs->path + d->getItem(sel - 2) + "/";
            mkdir(dstPath.substr(0, dstPath.length() - 1).c_str(), 777);
            fs::copyDirToDirThreaded(srcPath, dstPath);
        }
        else if(sel > 1)
        {
            std::string srcPath = *ma->path + d->getItem(sel - 2);
            std::string dstPath = *sdmcArgs->path + d->getItem(sel - 2);
            fs::copyFileThreaded(srcPath, dstPath);
        }
    }
    else if(ma == sdmcArgs)
    {
        //I know
        if(sel == 0)
        {
            if(commit)
                fs::copyDirToDirCommitThreaded(*ma->path, *devArgs->path, dev);
            else
                fs::copyDirToDirThreaded(*ma->path, *devArgs->path);
        }
        else if(sel > 1 && d->isDir(sel - 2))
        {
            std::string srcPath = *ma->path + d->getItem(sel - 2) + "/";
            std::string dstPath = *devArgs->path + d->getItem(sel - 2) + "/";
            mkdir(dstPath.substr(0, dstPath.length() - 1).c_str(), 777);
            if(commit)
                fs::copyDirToDirCommitThreaded(srcPath, dstPath, dev);
            else
                fs::copyDirToDirThreaded(srcPath, dstPath);
        }
        else if(sel > 1)
        {
            std::string srcPath = *ma->path + d->getItem(sel - 2);
            std::string dstPath = *devArgs->path + d->getItem(sel - 2);
            if(commit)
                fs::copyFileCommitThreaded(srcPath, dstPath, dev);
            else
                fs::copyFileThreaded(srcPath, dstPath);
        }
    }
    ui::newThread(refreshMenu, devArgs, NULL);
    ui::newThread(refreshMenu, sdmcArgs, NULL);
    t->finished = true;
}

//Sets up a confirm thread that then runs ^
static void _copyMenuCopy(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    ui::menu *m = ma->m;
    fs::dirList *d = ma->d;

    std::string srcPath, dstPath;
    int sel = m->getSelected();
    if(sel == 0 && ma == devArgs)
    {
        srcPath = *ma->path;
        dstPath = *sdmcArgs->path;
    }
    else if(sel > 1 && ma == devArgs)
    {
        srcPath = *ma->path + d->getItem(sel - 2);
        dstPath = *sdmcArgs->path + d->getItem(sel - 2);
    }
    else if(sel == 0 && ma == sdmcArgs)
    {
        srcPath = *ma->path;
        dstPath = *devArgs->path;
    }
    else if(sel > 1 && ma == sdmcArgs)
    {
        srcPath = *ma->path + d->getItem(m->getSelected() - 2);
        dstPath = *devArgs->path + d->getItem(m->getSelected() - 2);
    }

    if(ma == devArgs ||  (ma == sdmcArgs && (type != FsSaveDataType_System || cfg::config["sysSaveWrite"])))
    {
        ui::confirmArgs *send = ui::confirmArgsCreate(false, _copyMenuCopy_t, NULL, ma, ui::getUICString("confirmCopy", 0), srcPath.c_str(), dstPath.c_str());
        ui::confirm(send);
    }
}

static void _copyMenuDelete_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    menuFuncArgs *ma = (menuFuncArgs *)t->argPtr;
    ui::menu *m = ma->m;
    fs::dirList *d = ma->d;

    t->status->setStatus(ui::getUICString("threadStatusDeletingFile", 0));

    int sel = m->getSelected();
    if(ma == devArgs)
    {
        if(sel == 0 && *ma->path != "sdmc:/")
        {
            fs::delDir(*ma->path);
            if(commit)
                fs::commitToDevice(dev);
        }
        else if(sel > 1 && d->isDir(sel - 2))
        {
            std::string delPath = *ma->path + d->getItem(sel - 2) + "/";
            fs::delDir(delPath);
            if(commit)
                fs::commitToDevice(dev);
        }
        else if(sel > 1)
        {
            std::string delPath = *ma->path + d->getItem(sel - 2);
            fs::delfile(delPath);
            if(commit)
                fs::commitToDevice(dev);
        }
    }
    else
    {
        if(sel == 0 && *ma->path != "sdmc:/")
            fs::delDir(*ma->path);
        else if(sel > 1 && d->isDir(sel - 2))
        {
            std::string delPath = *ma->path + d->getItem(sel - 2) + "/";
            fs::delDir(delPath);
        }
        else if(sel > 1)
        {
            std::string delPath = *ma->path + d->getItem(sel - 2);
            fs::delfile(delPath);
        }
    }
    ui::newThread(refreshMenu, devArgs, NULL);
    ui::newThread(refreshMenu, sdmcArgs, NULL);
    t->finished = true;
}

//Prepares confirm then runs ^
static void _copyMenuDelete(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    ui::menu *m = ma->m;
    fs::dirList *d = ma->d;

    int sel = m->getSelected();
    std::string itmPath;
    if(sel == 0)
        itmPath = *ma->path;
    else if(sel > 1)
        itmPath = *ma->path + d->getItem(sel - 2);

    if(ma == sdmcArgs || (ma == devArgs && (sel == 0 || sel > 1) && (type != FsSaveDataType_System || cfg::config["sysSaveWrite"])))
    {
        ui::confirmArgs *send = ui::confirmArgsCreate(cfg::config["holdDel"], _copyMenuDelete_t, NULL, a, ui::getUICString("confirmDelete", 0), itmPath.c_str());
        ui::confirm(send);
    }
}

static void _copyMenuRename(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    ui::menu *m = ma->m;
    fs::dirList *d = ma->d;

    int sel = m->getSelected();
    if(sel > 1)
    {
        std::string getNewName = util::getStringInput(SwkbdType_QWERTY, d->getItem(sel - 2), ui::getUIString("swkbdRename", 0), 64, 0, NULL);
        if(!getNewName.empty())
        {
            std::string prevPath = *ma->path + d->getItem(sel - 2);
            std::string newPath  = *ma->path + getNewName;
            rename(prevPath.c_str(), newPath.c_str());
        }
        threadInfo fake;
        fake.argPtr = devArgs;
        refreshMenu(&fake);
        fake.argPtr = sdmcArgs;
        refreshMenu(&fake);
    }
}

static void _copyMenuMkDir(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    std::string getNewFolder = util::getStringInput(SwkbdType_QWERTY, ui::getUIString("fileModeMenuMkDir", 0), ui::getUIString("swkbdMkDir", 0), 64, 0, NULL);
    if(!getNewFolder.empty())
    {
        std::string createPath = *ma->path + getNewFolder;
        mkdir(createPath.c_str(), 777);
    }
    threadInfo fake;
    fake.argPtr = devArgs;
    refreshMenu(&fake);
    fake.argPtr = sdmcArgs;
    refreshMenu(&fake);
}

static void _copyMenuGetShowDirProps_t(void *a)
{
    threadInfo *t = (threadInfo *)a;
    std::string *p = (std::string *)t->argPtr;
    unsigned dirCount = 0, fileCount = 0;
    uint64_t totalSize = 0;
    t->status->setStatus(ui::getUICString("threadStatusGetDirProps", 0));
    fs::getDirProps(*p, dirCount, fileCount, totalSize);
    ui::showMessage(ui::getUICString("fileModeFolderProperties", 0), p->c_str(), dirCount, fileCount, util::getSizeString(totalSize).c_str());
    delete p;
    t->finished = true;
}

static void _copyMenuGetProps(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    ui::menu *m = ma->m;
    fs::dirList *d = ma->d;

    int sel = m->getSelected();
    if(sel == 0)
    {
        std::string *folderPath = new std::string(*ma->path);
        ui::newThread(_copyMenuGetShowDirProps_t, folderPath, NULL);
    }
    else if(sel > 1 && d->isDir(sel - 2))
    {
        std::string *folderPath = new std::string;
        folderPath->assign(*ma->path + d->getItem(sel - 2) + "/");
        ui::newThread(_copyMenuGetShowDirProps_t, folderPath, NULL);
    }
    else if(sel > 1)
    {
        std::string filePath = *ma->path + d->getItem(sel - 2);
        fs::getShowFileProps(filePath);
    }
}

static void _copyMenuClose(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    if(ma == devArgs)
    {
        devCopyMenu->setActive(false);
        devMenu->setActive(true);
        devPanel->closePanel();
    }
    else
    {
        sdCopyMenu->setActive(false);
        sdMenu->setActive(true);
        sdPanel->closePanel();
    }
}

static void _devMenuAddToPathFilter(void *a)
{
    menuFuncArgs *ma = (menuFuncArgs *)a;
    ui::menu *m = ma->m;
    fs::dirList *d = ma->d;

    int sel = m->getSelected();
    if(sel > 1)
    {
        data::userTitleInfo *tinfo = data::getCurrentUserTitleInfo();
        std::string filterPath = *ma->path + d->getItem(sel - 2);
        cfg::addPathToFilter(tinfo->tid, filterPath);
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popAddedToPathFilter", 0), filterPath.c_str());
    }
}

void ui::fmInit()
{
    //This needs to be done in a strange order so everything works
    devArgs = new menuFuncArgs;
    devArgs->path = &devPath;

    sdmcArgs = new menuFuncArgs;
    sdmcArgs->path = &sdPath;

    devMenu = new ui::menu(10, 8, 590, 18, 5);
    devMenu->setCallback(_devMenuCallback, devArgs);
    devMenu->setActive(true);
    devArgs->m = devMenu;

    devPanel = new ui::slideOutPanel(288, 720, 0, ui::SLD_LEFT, _devCopyPanelDraw);
    devCopyMenu = new ui::menu(10, 185, 268, 20, 5);
    devCopyMenu->setActive(false);
    devCopyMenu->setCallback(_devCopyMenuCallback, NULL);
    devCopyMenu->addOpt(NULL, ui::getUIString("fileModeMenu", 0) + "SDMC");
    for(int i = 1; i < 4; i++)
        devCopyMenu->addOpt(NULL, ui::getUIString("fileModeMenu", i));
    //Manually do this so I can place the last option higher up
    devCopyMenu->addOpt(NULL, ui::getUIString("fileModeMenu", 6));
    devCopyMenu->addOpt(NULL, ui::getUIString("fileModeMenu", 4));
    devCopyMenu->addOpt(NULL, ui::getUIString("fileModeMenu", 5));

    devCopyMenu->optAddButtonEvent(0, HidNpadButton_A, _copyMenuCopy, devArgs);
    devCopyMenu->optAddButtonEvent(1, HidNpadButton_A, _copyMenuDelete, devArgs);
    devCopyMenu->optAddButtonEvent(2, HidNpadButton_A, _copyMenuRename, devArgs);
    devCopyMenu->optAddButtonEvent(3, HidNpadButton_A, _copyMenuMkDir, devArgs);
    devCopyMenu->optAddButtonEvent(4, HidNpadButton_A, _devMenuAddToPathFilter, devArgs);
    devCopyMenu->optAddButtonEvent(5, HidNpadButton_A, _copyMenuGetProps, devArgs);
    devCopyMenu->optAddButtonEvent(6, HidNpadButton_A, _copyMenuClose, devArgs);
    ui::registerPanel(devPanel);

    sdMenu = new ui::menu(620, 8, 590, 18, 5);
    sdMenu->setCallback(_sdMenuCallback, sdmcArgs);
    sdMenu->setActive(false);
    sdmcArgs->m = sdMenu;

    sdPanel = new ui::slideOutPanel(288, 720, 0, ui::SLD_RIGHT, _sdCopyPanelDraw);
    sdCopyMenu = new ui::menu(10, 210, 268, 20, 5);
    sdCopyMenu->setActive(false);
    sdCopyMenu->setCallback(_sdCopyMenuCallback, NULL);
    for(int i = 0; i < 6; i++)
        sdCopyMenu->addOpt(NULL, ui::getUIString("fileModeMenu", i));
    sdCopyMenu->optAddButtonEvent(0, HidNpadButton_A, _copyMenuCopy, sdmcArgs);
    sdCopyMenu->optAddButtonEvent(1, HidNpadButton_A, _copyMenuDelete, sdmcArgs);
    sdCopyMenu->optAddButtonEvent(2, HidNpadButton_A, _copyMenuRename, sdmcArgs);
    sdCopyMenu->optAddButtonEvent(3, HidNpadButton_A, _copyMenuMkDir, sdmcArgs);
    sdCopyMenu->optAddButtonEvent(4, HidNpadButton_A, _copyMenuGetProps, sdmcArgs);
    sdCopyMenu->optAddButtonEvent(5, HidNpadButton_A, _copyMenuClose, sdmcArgs);
    ui::registerPanel(sdPanel);

    devList = new fs::dirList;
    sdList  = new fs::dirList;
    devArgs->d = devList;
    sdmcArgs->d = sdList;
}

void ui::fmExit()
{
    delete devMenu;
    delete sdMenu;
    delete devCopyMenu;
    delete sdCopyMenu;
    delete devList;
    delete sdList;
    delete devArgs;
    delete sdmcArgs;
}

void ui::fmPrep(const FsSaveDataType& _type, const std::string& _dev, const std::string& _baseSDMC, bool _commit)
{
    type = _type;
    dev  = _dev;
    commit = _commit;
    devPath = _dev;
    sdPath = _baseSDMC;

    sdCopyMenu->editOpt(0, NULL, ui::getUIString("fileModeMenu", 0) + _dev);

    devList->reassign(dev);
    sdList->reassign(sdPath);
    util::copyDirListToMenu(*devList, *devMenu);
    for(int i = 1; i < devMenu->getCount(); i++)
        devMenu->optAddButtonEvent(i, HidNpadButton_A, _listFunctionA, devArgs);

    util::copyDirListToMenu(*sdList, *sdMenu);
    for(int i = 1; i < sdMenu->getCount(); i++)
        sdMenu->optAddButtonEvent(i, HidNpadButton_A, _listFunctionA, sdmcArgs);
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
