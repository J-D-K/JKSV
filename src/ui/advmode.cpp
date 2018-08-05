#include <string>
#include <vector>
#include <sys/stat.h>

#include "ui.h"
#include "miscui.h"
#include "uiupdate.h"
#include "util.h"
#include "file.h"

//Text menus
static ui::menu saveMenu, sdMenu, copyMenu;

//Paths + wrapped string paths
static std::string savePath, sdPath, saveWrap, sdWrap;

//Adv Ctrl mode
static int advMenuCtrl, advPrev;

//Dir listings
fs::dirList saveList(""), sdList("sdmc:/");

//Performs copy menu operations. To big to stuff into case IMO.
void performCopyMenuOps()
{
    switch(copyMenu.getSelected())
    {
        //Copy
        case 0:
            {
                switch(advPrev)
                {
                    //save
                    case 0:
                        if(saveMenu.getSelected() == 0)
                        {
                            //Copy current open dir
                            if(ui::confirmTransfer(savePath, sdPath))
                                fs::copyDirToDir(savePath, sdPath);
                        }
                        else if(saveMenu.getSelected() > 1)
                        {
                            //Forget '..'
                            int saveSel = saveMenu.getSelected() - 2;
                            if(saveList.isDir(saveSel))
                            {
                                //Dir we're copying from
                                std::string fromPath = savePath + saveList.getItem(saveSel) + "/";

                                //Where we're copying to
                                std::string toPath = sdPath + saveList.getItem(saveSel);
                                if(ui::confirmTransfer(fromPath, toPath))
                                {
                                    //make dir on sd
                                    mkdir(toPath.c_str(), 777);
                                    toPath += "/";

                                    fs::copyDirToDir(fromPath, toPath);
                                }
                            }
                            else
                            {
                                //just copy file
                                std::string fromPath = savePath + saveList.getItem(saveSel);
                                std::string toPath = sdPath + saveList.getItem(saveSel);
                                if(ui::confirmTransfer(fromPath, toPath))
                                    fs::copyFile(fromPath, toPath);
                            }
                        }
                        break;

                    case 1:
                        if(data::curData.getType() != FsSaveDataType_SystemSaveData)
                        {
                            if(sdMenu.getSelected() == 0)
                            {
                                if(ui::confirmTransfer(sdPath, savePath))
                                    fs::copyDirToDirCommit(sdPath, savePath, "sv");
                            }
                            else if(sdMenu.getSelected() > 1)
                            {
                                //Same as above, but reverse
                                int sdSel = sdMenu.getSelected() - 2;
                                if(sdList.isDir(sdSel))
                                {
                                    std::string fromPath = sdPath + sdList.getItem(sdSel) + "/";

                                    std::string toPath = savePath + sdList.getItem(sdSel);

                                    if(ui::confirmTransfer(fromPath, toPath))
                                    {
                                        mkdir(toPath.c_str(), 777);
                                        toPath += "/";

                                        fs::copyDirToDirCommit(fromPath, toPath, "sv");
                                    }
                                }
                                else
                                {
                                    std::string fromPath = sdPath + sdList.getItem(sdSel);
                                    std::string toPath = savePath + sdList.getItem(sdSel);
                                    if(ui::confirmTransfer(fromPath, toPath))
                                        fs::copyFileCommit(fromPath, toPath, "sv");
                                }
                            }
                        }
                        break;
                }
            }
            break;

        //delete
        case 1:
            {
                switch(advPrev)
                {
                    //save menu
                    case 0:
                        if(data::curData.getType() != FsSaveDataType_SystemSaveData)
                        {
                            if(saveMenu.getSelected() == 0)
                            {
                                if(ui::confirmDelete(savePath))
                                {
                                    fs::delDir(savePath);
                                    fsdevCommitDevice("sv");
                                }
                            }
                            else if(saveMenu.getSelected() > 1)
                            {
                                int saveSel = saveMenu.getSelected() - 2;
                                if(saveList.isDir(saveSel))
                                {
                                    std::string delPath = savePath + saveList.getItem(saveSel) + "/";
                                    if(ui::confirmDelete(delPath))
                                        fs::delDir(delPath);
                                }
                                else
                                {
                                    std::string delPath = savePath + saveList.getItem(saveSel);
                                    if(ui::confirmDelete(delPath))
                                        std::remove(delPath.c_str());
                                }
                                fsdevCommitDevice("sv");
                            }
                        }
                        break;

                    //sd
                    case 1:
                        if(sdMenu.getSelected() == 0)
                        {
                            if(ui::confirmDelete(sdPath) && sdPath != "sdmc:/")
                                fs::delDir(sdPath);
                        }
                        else if(sdMenu.getSelected() > 1)
                        {
                            int sdSel = sdMenu.getSelected() - 2;
                            if(sdList.isDir(sdSel))
                            {
                                std::string delPath = sdPath + sdList.getItem(sdSel) + "/";
                                if(ui::confirmDelete(delPath))
                                    fs::delDir(delPath);
                            }
                            else
                            {
                                std::string delPath = sdPath + sdList.getItem(sdSel);
                                if(ui::confirmDelete(delPath))
                                    std::remove(delPath.c_str());
                            }
                        }
                        break;
                }
            }
            break;

        //Rename
        case 2:
            switch(advPrev)
            {
                //save
                case 0:
                    {
                        if(saveMenu.getSelected() > 1 && data::curData.getType() != FsSaveDataType_SystemSaveData)
                        {
                            int selSave = saveMenu.getSelected() - 2;
                            ui::keyboard getName;
                            std::string newName = getName.getString(saveList.getItem(selSave));
                            if(!newName.empty())
                            {
                                std::string b4Path = savePath + saveList.getItem(selSave);
                                std::string newPath = savePath + newName;

                                std::rename(b4Path.c_str(), newPath.c_str());
                                fsdevCommitDevice("sv");
                            }
                        }
                    }
                    break;

                //sd
                case 1:
                    {
                        if(sdMenu.getSelected() > 1)
                        {
                            int sdSel = sdMenu.getSelected() - 2;
                            ui::keyboard getName;
                            std::string newName = getName.getString(sdList.getItem(sdSel));
                            if(!newName.empty())
                            {
                                std::string b4Path = sdPath + sdList.getItem(sdSel);
                                std::string newPath = sdPath + newName;

                                std::rename(b4Path.c_str(), newPath.c_str());
                            }
                        }
                    }
                    break;
            }
            break;

        //Mkdir
        case 3:
            {
                switch(advPrev)
                {
                    //save
                    case 0:
                        {
                            if(data::curData.getType() != FsSaveDataType_SystemSaveData)
                            {
                                ui::keyboard getFolder;
                                std::string newFolder = util::safeString(getFolder.getString(""));
                                if(!newFolder.empty())
                                {
                                    std::string folderPath = savePath + newFolder;
                                    mkdir(folderPath.c_str(), 777);
                                    fsdevCommitDevice("sv");
                                }
                            }
                        }
                        break;

                    //sd
                    case 1:
                        {
                            ui::keyboard getFolder;
                            std::string newFolder = getFolder.getString("");
                            if(!newFolder.empty())
                            {
                                std::string folderPath = sdPath + newFolder;
                                mkdir(folderPath.c_str(), 777);
                            }
                        }
                        break;
                }
            }
            break;

        //Props
        case 4:
            {
                switch(advPrev)
                {
                    //sv
                    case 0:
                        {
                            if(saveMenu.getSelected() > 1)
                            {
                                int sel = saveMenu.getSelected() - 2;
                                std::string fullPath = savePath + saveList.getItem(sel);
                                std::string props = fs::getFileProps(fullPath);
                                if(!props.empty())
                                    ui::showMessage(props);
                            }
                        }
                        break;

                    case 1:
                        {
                            if(sdMenu.getSelected() > 1)
                            {
                                int sel = sdMenu.getSelected() - 2;
                                std::string fullPath = sdPath + sdList.getItem(sel);
                                std::string props = fs::getFileProps(fullPath);
                                if(!props.empty())
                                    ui::showMessage(props);
                            }
                        }
                        break;
                }
            }
            break;

        //back
        case 5:
            advMenuCtrl = advPrev;
            break;

    }

    //update lists + menus
    sdList.rescan();
    saveList.rescan();
    util::copyDirListToMenu(sdList, sdMenu);
    util::copyDirListToMenu(saveList, saveMenu);
}

namespace ui
{
    void advCopyMenuPrep()
    {
        copyMenu.addOpt("Copy From");
        copyMenu.addOpt("Delete");
        copyMenu.addOpt("Rename");
        copyMenu.addOpt("Make Dir");
        copyMenu.addOpt("Properties");
        copyMenu.addOpt("Back");
    }

    void advModePrep()
    {
        saveMenu.setParams(16, 88, 616);
        sdMenu.setParams(648, 88, 616);
        copyMenu.setParams(472, 278, 304);

        savePath = "sv:/";
        sdPath   = "sdmc:/";

        saveWrap = "sv:/";
        sdWrap   = "sdmc:/";

        saveList.reassign(savePath);
        sdList.reassign(sdPath);

        util::copyDirListToMenu(saveList, saveMenu);
        util::copyDirListToMenu(sdList, sdMenu);

        advMenuCtrl = 0;
    }

    void updateAdvMode(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        //0 = save; 1 = sd; 2 = cpy
        switch(advMenuCtrl)
        {
            case 0:
                saveMenu.handleInput(down, held, p);
                break;

            case 1:
                sdMenu.handleInput(down, held, p);
                break;

            case 2:
                copyMenu.handleInput(down, held, p);
                break;
        }

        saveMenu.draw(mnuTxt);
        sdMenu.draw(mnuTxt);

        drawText(saveWrap.c_str(), frameBuffer, ui::shared, 16, 668, 14, mnuTxt);
        drawText(sdWrap.c_str(), frameBuffer, ui::shared, 656, 668, 14, mnuTxt);

        //OH BOY HERE WE GO
        if(down & KEY_A)
        {
            switch(advMenuCtrl)
            {
                //save
                case 0:
                    {
                        int saveSel = saveMenu.getSelected();
                        if(saveSel == 1 && savePath != "sv:/")
                        {
                            util::removeLastFolderFromString(savePath);
                            saveWrap = util::getWrappedString(savePath, 14, 600);

                            saveList.reassign(savePath);
                            util::copyDirListToMenu(saveList, saveMenu);
                        }
                        else if(saveSel > 1 && saveList.isDir(saveSel - 2))
                        {
                            savePath += saveList.getItem(saveSel - 2) + "/";
                            saveWrap = util::getWrappedString(savePath, 14, 600);

                            saveList.reassign(savePath);
                            util::copyDirListToMenu(saveList, saveMenu);
                        }
                    }
                    break;

                //sd
                case 1:
                    {
                        int sdSel = sdMenu.getSelected();
                        if(sdSel == 1 && sdPath != "sdmc:/")
                        {
                            util::removeLastFolderFromString(sdPath);
                            sdWrap = util::getWrappedString(sdPath, 14, 600);

                            sdList.reassign(sdPath);
                            util::copyDirListToMenu(sdList, sdMenu);
                        }
                        else if(sdSel > 1 && sdList.isDir(sdSel - 2))
                        {
                            sdPath += sdList.getItem(sdSel - 2) + "/";
                            sdWrap  = util::getWrappedString(sdPath, 14, 600);

                            sdList.reassign(sdPath);
                            util::copyDirListToMenu(sdList, sdMenu);
                        }
                    }
                    break;

                //advanced mode
                case 2:
                    performCopyMenuOps();
                    break;
            }
        }
        else if(down & KEY_B)
        {
            //save
            if(advMenuCtrl == 0 && savePath != "sv:/")
            {
                util::removeLastFolderFromString(savePath);
                saveWrap = util::getWrappedString(savePath, 14, 600);

                saveList.reassign(savePath);
                util::copyDirListToMenu(saveList, saveMenu);
            }
            //sd
            else if(advMenuCtrl == 1 && sdPath != "sdmc:/")
            {
                util::removeLastFolderFromString(sdPath);
                sdWrap = util::getWrappedString(sdPath, 14, 600);

                sdList.reassign(sdPath);
                util::copyDirListToMenu(sdList, sdMenu);
            }
            else if(advMenuCtrl == 2)
                advMenuCtrl = advPrev;
        }
        else if(down & KEY_X)
        {
            if(advMenuCtrl == 2)
            {
                advMenuCtrl = advPrev;
            }
            else
            {
                advPrev = advMenuCtrl;
                advMenuCtrl = 2;
            }
        }
        else if(down & KEY_ZL || down & KEY_ZR)
        {
            if(advMenuCtrl == 0 || advMenuCtrl == 1)
            {
                if(advMenuCtrl == 0)
                    advMenuCtrl = 1;
                else
                    advMenuCtrl = 0;
            }
        }
        else if(down & KEY_MINUS)
        {
            if(ui::clsMode)
                mstate = CLS_FLD;
            else
                mstate = FLD_SEL;
        }

        //draw copy menu if it's supposed to be up
        if(advMenuCtrl == 2)
        {
            ui::drawTextbox(464, 236, 320, 268);

            switch(advPrev)
            {
                case 0:
                    drawText("SAVE", frameBuffer, ui::shared, 472, 250, 18,txtClr);
                    break;

                case 1:
                    drawText("SDMC", frameBuffer, ui::shared, 472, 250, 18, txtClr);
                    break;
            }
            copyMenu.draw(txtClr);
        }
    }
}
