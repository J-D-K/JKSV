#include <string>

#include "ui.h"
#include "file.h"
#include "util.h"
#include "fm.h"

static ui::slideOutPanel *copyPanel;
static ui::menu *devMenu, *sdMenu, *copyMenu;
static fs::dirList *devList, *sdList;
static std::string devPath, sdPath, dev;

static FsSaveDataType type;
static bool commit = false;


static void copyPanelDraw(void *a)
{

}

void ui::fmInit()
{
    devMenu = new ui::menu;
    devMenu->setParams(10, 32, 520, 18, 8);
    devMenu->setActive(true);

    sdMenu = new ui::menu;
    sdMenu->setParams(560, 32, 520, 18, 18);
    sdMenu->setActive(false);

    copyPanel = new ui::slideOutPanel(410, 720, 0, copyPanelDraw);

    devList = new fs::dirList;
    sdList  = new fs::dirList;
}

void ui::fmExit()
{
    delete devMenu;
    delete sdMenu;
    delete devList;
    delete sdList;
    delete copyPanel;
}

void ui::fmPrep(const FsSaveDataType& _type, const std::string& _dev, bool _commit)
{
    type = _type;
    dev  = _dev;
    commit = _commit;
    devPath = _dev;
    sdPath = "sdmc:/";

    devList->reassign(dev);
    sdList->reassign("sdmc:/");
    util::copyDirListToMenu(*devList, *devMenu);
    util::copyDirListToMenu(*sdList, *sdMenu);
}
