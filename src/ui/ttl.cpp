#include "ui.h"
#include "ttl.h"
#include "file.h"
#include "util.h"

class titleTile
{
    public:
        titleTile(unsigned _w, unsigned _h, bool _fav, SDL_Texture *_icon)
        {
            w = _w;
            h = _h;
            wS = _w;
            hS = _h;
            fav = _fav;
            icon = _icon;
        }

        void draw(SDL_Texture *target, int x, int y, bool sel);

    private:
        unsigned w, h, wS, hS;
        bool fav = false;
        SDL_Texture *icon;
};

static uint8_t clrShft = 0;
static bool clrAdd = true;
static int selRectX = 38, selRectY = 38;
static int x = 34, y = 69, ttlHelpX = 0;
static std::vector<titleTile> titleList;
static ui::menu *ttlOpts;
static ui::slideOutPanel *ttlOptsPanel, *infoPanel;
static std::string infoPanelString;

static void ttlOptsCallback(void *a)
{
    switch(ui::padKeysDown())
    {
        case HidNpadButton_B:
            ttlOpts->setActive(false);
            ttlOptsPanel->closePanel();
            ui::updateInput();
            break;
    }
}

static void ttlOptsPanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    ttlOpts->draw(panel, &ui::txtCont, true);
}

static void ttlOptsShowInfoPanel(void *a)
{
    ttlOpts->setActive(false);
    ttlOptsPanel->closePanel();
    infoPanelString = util::getInfoString(data::curUser, data::curData.saveID);
    infoPanel->openPanel();
}

static void ttlOptsBlacklistTitle(void *a)
{
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    if(ui::confirm(false, ui::confBlacklist.c_str(), title.c_str()))
    {
        data::blacklistAdd(data::curData.saveID);
        ui::setupTiles(NULL);
    }
}

static void ttlOptsResetSaveData(void *a)
{
    std::string title = data::getTitleNameByTID(data::curData.saveID);
    if(ui::confirm(data::holdDel, ui::saveDataReset.c_str(), title.c_str()) && fs::mountSave(data::curData.saveInfo))
    {
        fs::delDir("sv:/");
        fsdevCommitDevice("sv:/");
        fs::unmountSave();
        ui::showPopup(POP_FRAME_DEFAULT, ui::saveDataResetSuccess.c_str(), title.c_str());
    }
}

static void ttlOptsDeleteSaveData(void *a)
{
    FsSaveDataAttribute attr;
    attr.application_id = data::curData.saveID;
    attr.uid = data::curUser.getUID();
    attr.system_save_data_id = 0;
    attr.save_data_type = data::curData.saveInfo.save_data_type;
    attr.save_data_rank = data::curData.saveInfo.save_data_rank;
    attr.save_data_index = data::curData.saveInfo.save_data_index;

    std::string title = data::getTitleNameByTID(data::curData.saveID);
    if(ui::confirm(data::holdDel, ui::confEraseNand.c_str(), title.c_str()) && R_SUCCEEDED(fsDeleteSaveDataFileSystemBySaveDataAttribute(FsSaveDataSpaceId_User, &attr)))
    {
        data::loadUsersTitles(false);
        if(data::curUser.titleInfo.size() == 0)
        {
            //Kick back to user
            ttlOptsPanel->closePanel();//JIC
            ui::usrMenuSetActive(true);
            ui::changeState(USR_SEL);
        }
        else if(data::selData > (int)data::curUser.titleInfo.size() - 1)
            data::selData = data::curUser.titleInfo.size() - 1;

        ui::setupTiles(NULL);
        ui::showPopup(POP_FRAME_DEFAULT, ui::saveDataDeleteSuccess.c_str(), title.c_str());
    }
}

static void infoPanelDraw(void *a)
{
    SDL_Texture *panel = (SDL_Texture *)a;
    gfx::texDraw(panel, data::getTitleIconByTID(data::curData.saveID), 77, 32);
    gfx::drawTextfWrap(panel, 18, 32, 320, 362, &ui::txtCont, infoPanelString.c_str());
}

void ui::ttlInit()
{
    ttlHelpX = 1220 - gfx::getTextWidth(ui::titleHelp.c_str(), 18);

    ttlOpts = new ui::menu;
    ttlOpts->setParams(10, 32, 390, 18, 8);

    ttlOptsPanel = new ui::slideOutPanel(410, 720, 0, ttlOptsPanelDraw);
    ttlOpts->setCallback(ttlOptsCallback, NULL);
    ui::addPanel(ttlOptsPanel);

    infoPanel = new ui::slideOutPanel(410, 720, 0, infoPanelDraw);
    ui::addPanel(infoPanel);

    ttlOpts->setActive(false);
    ttlOpts->addOpt(NULL, ui::titleOptString[0]);
    ttlOpts->setOptFunc(0, FUNC_A, ttlOptsShowInfoPanel, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[1]);
    ttlOpts->setOptFunc(1, FUNC_A, ttlOptsBlacklistTitle, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[2]);
    ttlOpts->setOptFunc(2, FUNC_A, ttlOptsResetSaveData, NULL);
    ttlOpts->addOpt(NULL, ui::titleOptString[3]);
    ttlOpts->setOptFunc(3, FUNC_A, ttlOptsDeleteSaveData, NULL);
}

void ui::ttlExit()
{
    delete ttlOptsPanel;
    delete ttlOpts;
    delete infoPanel;
}

void ui::setupTiles(void *)
{
    titleList.clear();
    int infoSize = data::curUser.titleInfo.size();
    for(int i = 0; i < infoSize; i++)
    {
        uint64_t sid = data::curUser.titleInfo[i].saveID;
        SDL_Texture *iconPtr = data::getTitleIconByTID(sid);
        bool fav = data::isFavorite(sid);
        titleList.emplace_back(128, 128, fav, iconPtr);
    }
}

void ui::ttlReset()
{
    clrShft = 0;
    clrAdd = true;
    selRectX = 38;
    selRectY = 38;
    x = 34;
    y = 69;
    //Reset data
    data::selData = 0;
}

static inline void updateTitleScroll()
{
    if(selRectY > 264)
        y -= 48;
    else if(selRectY > 144)
        y -= 24;
    else if(selRectY < -82)
        y += 48;
    else if(selRectY < 38)
        y += 24;
}

void ui::ttlUpdate()
{
    ttlOpts->update();

    if(ttlOptsPanel->isOpen() || infoPanel->isOpen())
        return;

    uint64_t down = ui::padKeysDown();

    switch(down)
    {
        case HidNpadButton_A:
            if(fs::mountSave(data::curData.saveInfo))
            {
                fldInit();
                ui::changeState(FLD_SEL);
            }
            else
                ui::showPopup(POP_FRAME_DEFAULT, "Failed to Mount save!");
            break;

        case HidNpadButton_B:
            ui::usrMenuSetActive(true);
            ui::changeState(USR_SEL);
            break;

        case HidNpadButton_X:
            ttlOpts->setActive(true);
            ttlOptsPanel->openPanel();
            break;

        case HidNpadButton_Y:
            {
                uint64_t sid = data::curData.saveID;
                data::favoriteTitle(sid);
                setupTiles(NULL);
                if(data::isFavorite(sid))
                    data::selData = data::getTitleIndexInUser(data::curUser, sid);
            }
            break;

        case HidNpadButton_StickLUp:
        case HidNpadButton_Up:
            data::selData -= 7;
            if(data::selData < 0)
                data::selData = 0;
            break;

        case HidNpadButton_StickLDown:
        case HidNpadButton_Down:
            data::selData += 7;
            if(data::selData > (int)data::curUser.titleInfo.size() - 1)
                data::selData = data::curUser.titleInfo.size() - 1;
            break;

        case HidNpadButton_StickLLeft:
        case HidNpadButton_Left:
            if(data::selData > 0)
                --data::selData;
            break;

        case HidNpadButton_StickLRight:
        case HidNpadButton_Right:
            if(data::selData < (int)data::curUser.titleInfo.size() - 1)
                ++data::selData;
            break;

        case HidNpadButton_L:
            if(data::selData - 21 > 0)
                data::selData -= 21;
            else
                data::selData = 0;
            break;

        case HidNpadButton_R:
            if(data::selData + 21 < (int)data::curUser.titleInfo.size())
                data::selData += 21;
            else
                data::selData = data::curUser.titleInfo.size() - 1;
            break;
    }
}

void ui::ttlDraw(SDL_Texture *target)
{
    updateTitleScroll();

    if(clrAdd)
    {
        clrShft += 6;
        if(clrShft >= 0x72)
            clrAdd = false;
    }
    else
    {
        clrShft -= 3;
        if(clrShft <= 0)
            clrAdd = true;
    }

    int totalTiles = titleList.size();
    int selX = 0, selY = 0;
    for(int tY = y, i = 0; i < totalTiles; tY += 144)
    {
        int endRow = i + 7;
        for(int tX = x; i < endRow; tX += 144, i++)
        {
            if(i >= totalTiles)
                break;

            if(i == data::selData && (ui::mstate == TTL_SEL|| ui::mstate == FLD_SEL))
            {
                //save x and y for selected for later so it's drawn on top.
                //I can't find anyway to change the Z for SDL.
                selX = tX;
                selY = tY;
                selRectX = tX - 24;
                selRectY = tY - 24;
            }
            else
                titleList[i].draw(target, tX, tY, false);
        }
    }

    if(ui::mstate == TTL_SEL || ui::mstate == FLD_SEL)
    {
        //Draw selected after so it's on top
        ui::drawBoundBox(target, selRectX, selRectY, 176, 176, clrShft);
        titleList[data::selData].draw(target, selX, selY, true);
        if(ui::mstate == TTL_SEL)
            gfx::drawTextf(NULL, 18, ttlHelpX, 673, &ui::txtCont, ui::titleHelp.c_str());
    }
}

//Todo make less hardcoded
void titleTile::draw(SDL_Texture *target, int x, int y, bool sel)
{
    if(sel)
    {
        unsigned xScale = w * 1.28, yScale = w * 1.28;
        if(wS < xScale)
            wS += 18;
        if(hS < yScale)
            hS += 18;
    }
    else
    {
        if(wS > w)
            wS -= 18;
        if(hS > h)
            hS -= 18;
    }

    int dX = x - ((wS - w) / 2);
    int dY = y - ((hS - h) / 2);
    gfx::texDrawStretch(target, icon, dX, dY, wS, hS);
    if(fav)
        gfx::drawTextf(target, 20, dX + 8, dY + 8, &ui::heartColor, "♥");
}
