#include "fld.h"
#include "ui.h"
#include "file.h"
#include "data.h"
#include "util.h"

static ui::slideOutPanel *fldSlide = NULL;
static ui::menu fldMenu;
static fs::dirList fldList;
static fs::backupArgs backargs;

static void fldCallback(void *a)
{
    if(ui::padKeysDown() & HidNpadButton_B)
    {
        fs::unmountSave();
        fldSlide->closePanel();
        ui::changeState(TTL_SEL);
    }
}

static void slideDrawFunc(void *a)
{
    //Slide panel passes texture/target to draw func
    SDL_Texture *panel = (SDL_Texture *)a;
    fldMenu.draw(panel, &ui::txtCont, true);
}

void ui::fldInit()
{
    //Only allocate new if null
    if(!fldSlide)
    {
        fldSlide = new ui::slideOutPanel(410, 720, 0, slideDrawFunc);
        ui::addPanel(fldSlide);
    }

    fldMenu.reset();
    fldMenu.setParams(12, 32, 390, MENU_FONT_SIZE_DEFAULT, 8);
    fldMenu.setCallback(fldCallback, NULL);

    util::createTitleDirectoryByTID(data::curUser.titleInfo[data::selData].saveID);
    std::string folderPath = util::generatePathByTID(data::curUser.titleInfo[data::selData].saveID);

    fldList.reassign(folderPath);
    fs::loadPathFilters(folderPath + "pathFilters.txt");

    backargs = {&fldMenu, &fldList};

    fldMenu.addOpt(NULL, "New");
    fldMenu.setOptFunc(0, FUNC_A, fs::createNewBackup, &backargs);

    //Same as above, args updated at press
    for(unsigned i = 0; i < fldList.getCount(); i++)
    {
        fldMenu.addOpt(NULL, fldList.getItem(i));

        //Pass the menu to the functions so selected can be grabbed
        //offset i by 1 since new is always first option
        fldMenu.setOptFunc(i + 1, FUNC_A, fs::overwriteBackup, &backargs);
        fldMenu.setOptFunc(i + 1, FUNC_X, fs::deleteBackup, &backargs);
        fldMenu.setOptFunc(i + 1, FUNC_Y, fs::restoreBackup, &backargs);
    }
    fldSlide->openPanel();
}

void ui::fldExit()
{
    if(fldSlide)
        delete fldSlide;
}

void ui::fldUpdate()
{
    fldMenu.update();
}

void ui::fldDraw()
{
    /*if(fldSlide)
        fldSlide->draw(&slidePanelColor);*/
}
