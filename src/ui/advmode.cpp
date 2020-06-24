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
static std::string savePath, sdPath, dev;

//Adv Ctrl mode
static int advMenuCtrl, advPrev;

//Dir listings
fs::dirList saveList, sdList;

static bool commit = false;

static FsSaveDataType type = FsSaveDataType_System;

static inline bool sysSaveCheck()
{
    return data::sysSaveWrite || type != FsSaveDataType_System;
}

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
                                if(ui::confirmTransfer(fromPath, toPath) && fs::hasFreeSpace(toPath, fs::fsize(fromPath)))
                                    fs::copyFile(fromPath, toPath);
                            }
                        }
                        break;

                    case 1:
                        if(sysSaveCheck() && sdMenu.getSelected() == 0)
                        {
                            if(ui::confirmTransfer(sdPath, savePath))
                                commit ? fs::copyDirToDirCommit(sdPath, savePath, dev) : fs::copyDirToDir(sdPath, savePath);
                        }
                        else if(sysSaveCheck() && sdMenu.getSelected() > 1)
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

                                    commit ? fs::copyDirToDirCommit(fromPath, toPath, dev) : fs::copyDirToDir(fromPath, toPath);
                                }
                            }
                            else
                            {
                                std::string fromPath = sdPath + sdList.getItem(sdSel);
                                std::string toPath = savePath + sdList.getItem(sdSel);
                                if(ui::confirmTransfer(fromPath, toPath) && fs::hasFreeSpace(toPath, fs::fsize(fromPath)))
                                    commit ? fs::copyFileCommit(fromPath, toPath, dev) : fs::copyFile(fromPath, toPath);
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
                        if(sysSaveCheck())
                        {
                            if(saveMenu.getSelected() == 0)
                            {
                                if(ui::confirmDelete(savePath))
                                {
                                    fs::delDir(savePath);
                                    fsdevCommitDevice(dev.c_str());
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
                                if(commit)
                                    fsdevCommitDevice(dev.c_str());
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
                        if(sysSaveCheck() && saveMenu.getSelected() > 1)
                        {
                            int selSave = saveMenu.getSelected() - 2;
                            std::string newName = util::getStringInput(saveList.getItem(selSave), "Rename", 256, 0, NULL);
                            if(!newName.empty())
                            {
                                std::string b4Path = savePath + saveList.getItem(selSave);
                                std::string newPath = savePath + newName;

                                std::rename(b4Path.c_str(), newPath.c_str());
                                if(commit)
                                {
                                    fsdevCommitDevice(dev.c_str());
                                }
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
                            std::string newName = util::getStringInput(sdList.getItem(sdSel), "Rename", 256, 0, NULL);
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
                            if(sysSaveCheck())
                            {
                                std::string newFolder = util::getStringInput("", "New Folder", 256, 0, NULL);
                                if(!newFolder.empty())
                                {
                                    std::string folderPath = savePath + newFolder;
                                    mkdir(folderPath.c_str(), 777);
                                    if(commit)
                                    {
                                        fsdevCommitDevice(dev.c_str());
                                    }
                                }
                            }
                        }
                        break;

                    //sd
                    case 1:
                        {
                            std::string newFolder = util::getStringInput("", "New Folder", 256, 0, NULL);
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
                                if(fs::isDir(fullPath))
                                {
                                    uint32_t dirCnt = 0, fileCnt = 0;
                                    uint64_t fileSize = 0;
                                    fullPath += "/";

                                    fs::getDirProps(fullPath, dirCnt, fileCnt, fileSize);
                                    ui::showMessage("Folder Properties", "#%s#:\n%u Folders\n%u Files\nTotal Size: %.2fMB", fullPath.c_str(), dirCnt, fileCnt, (float)((float)fileSize / 1024.0f / 1024.0f));
                                }
                                else
                                {
                                    std::string fprops = fs::getFileProps(fullPath);
                                    if(!fprops.empty())
                                        ui::showMessage("File Properties:", fprops.c_str());
                                }
                            }
                        }
                        break;

                    case 1:
                        {
                            if(sdMenu.getSelected() > 1)
                            {
                                int sel = sdMenu.getSelected() - 2;
                                std::string fullPath = sdPath + sdList.getItem(sel);
                                if(fs::isDir(fullPath))
                                {
                                    uint32_t dirCnt = 0, fileCnt = 0;
                                    uint64_t fileSize = 0;
                                    fullPath += "/";

                                    fs::getDirProps(fullPath, dirCnt, fileCnt, fileSize);
                                    ui::showMessage("Folder Properties", "#%s#:\n%u Folders\n%u Files\nTotal Size: %.2fMB", fullPath.c_str(), dirCnt, fileCnt, (float)((float)fileSize / 1024.0f / 1024.0f));
                                }
                                else
                                {
                                    std::string fprops = fs::getFileProps(fullPath);
                                    if(!fprops.empty())
                                        ui::showMessage("File Properties:", fprops.c_str());
                                }
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


void ui::advCopyMenuPrep()
{
    for(unsigned i = 0; i < 6; i++)
        copyMenu.addOpt(ui::advMenuStr[i]);
}

void ui::advModePrep(const std::string& svDev, const FsSaveDataType& _type, bool commitOnWrite)
{
    commit = commitOnWrite;
    type = _type;

    saveMenu.setParams(30, 98, 602);
    sdMenu.setParams(648, 98, 602);
    copyMenu.setParams(472, 278, 304);

    savePath = svDev, dev = svDev;
    sdPath   = "sdmc:/";

    saveList.reassign(savePath);
    sdList.reassign(sdPath);

    util::copyDirListToMenu(saveList, saveMenu);
    util::copyDirListToMenu(sdList, sdMenu);

    advMenuCtrl = 0;
}

void ui::drawAdvMode()
{
    saveMenu.draw(ui::txtCont);
    sdMenu.draw(ui::txtCont);

    drawTextWrap(savePath.c_str(), frameBuffer, ui::shared, 30, 654, 14, ui::txtCont, 600);
    drawTextWrap(sdPath.c_str(), frameBuffer, ui::shared, 640, 654, 14, ui::txtCont, 600);

    //draw copy menu if it's supposed to be up
    if(advMenuCtrl == 2)
    {
        switch(advPrev)
        {
            case 0:
                copyMenu.setParams(176, 278, 304);
                copyMenu.editOpt(0, advMenuStr[0] + "sdmc");
                ui::drawTextbox(frameBuffer, 168, 236, 320, 268);
                drawText(dev.c_str(), frameBuffer, ui::shared, 176, 250, 18, ui::txtDiag);
                break;

            case 1:
                copyMenu.setParams(816, 278, 304);
                copyMenu.editOpt(0, advMenuStr[0] + dev);
                ui::drawTextbox(frameBuffer, 808, 236, 320, 268);
                drawText("SDMC", frameBuffer, ui::shared, 816, 250, 18, ui::txtDiag);
                break;
        }
        copyMenu.draw(ui::txtDiag);
    }
}

void ui::updateAdvMode(const uint64_t& down, const uint64_t& held)
{
    //0 = save; 1 = sd; 2 = cpy
    switch(advMenuCtrl)
    {
        case 0:
            saveMenu.handleInput(down, held);
            break;

        case 1:
            sdMenu.handleInput(down, held);
            break;

        case 2:
            copyMenu.handleInput(down, held);
            break;
    }

    //OH BOY HERE WE GO
    if(down & KEY_A)
    {
        switch(advMenuCtrl)
        {
            //save
            case 0:
                {
                    int saveSel = saveMenu.getSelected();
                    if(saveSel == 1 && savePath != dev)
                    {
                        util::removeLastFolderFromString(savePath);

                        saveList.reassign(savePath);
                        util::copyDirListToMenu(saveList, saveMenu);
                    }
                    else if(saveSel > 1 && saveList.isDir(saveSel - 2))
                    {
                        savePath += saveList.getItem(saveSel - 2) + "/";

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

                        sdList.reassign(sdPath);
                        util::copyDirListToMenu(sdList, sdMenu);
                    }
                    else if(sdSel > 1 && sdList.isDir(sdSel - 2))
                    {
                        sdPath += sdList.getItem(sdSel - 2) + "/";

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
        if(advMenuCtrl == 0 && savePath != dev)
        {
            util::removeLastFolderFromString(savePath);

            saveList.reassign(savePath);
            util::copyDirListToMenu(saveList, saveMenu);
        }
        //sd
        else if(advMenuCtrl == 1 && sdPath != "sdmc:/")
        {
            util::removeLastFolderFromString(sdPath);

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
            advMenuCtrl = advMenuCtrl == 0 ? 1 : 0;
    }
    else if(down & KEY_MINUS)
    {
        switch(prevState)
        {
            case EX_MNU:
                mstate = EX_MNU;
                break;

            case TTL_SEL:
                mstate = TTL_SEL;
                break;

            default:
                mstate = ui::textMode ? TXT_FLD : FLD_SEL;
                break;
        }
    }
}

