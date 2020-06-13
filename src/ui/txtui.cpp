#include <string>
#include <vector>
#include <cstdlib>
#include <switch.h>
#include <sys/stat.h>

#include "ui.h"
#include "data.h"
#include "file.h"
#include "util.h"

static ui::menu userMenu, titleMenu, exMenu, optMenu;
extern ui::menu folderMenu;

static inline void switchBool(bool& sw)
{
    sw ? sw = false : sw = true;
}

static inline std::string getBoolText(bool b)
{
    return b ? ui::on : ui::off;
}

void ui::textUserPrep()
{
    userMenu.reset();

    userMenu.setParams(76, 98, 310);

    for(unsigned i = 0; i < data::users.size(); i++)
        userMenu.addOpt(data::users[i].getUsername());
}

void ui::textTitlePrep(data::user& u)
{
    titleMenu.reset();
    titleMenu.setParams(76, 98, 310);

    for(unsigned i = 0; i < u.titles.size(); i++)
    {
        if(u.titles[i].getFav())
            titleMenu.addOpt("♥ " + u.titles[i].getTitle());
        else
            titleMenu.addOpt(u.titles[i].getTitle());
    }
}

void ui::textUserMenuUpdate(const uint64_t& down, const uint64_t& held)
{
    userMenu.handleInput(down, held);
    userMenu.draw(ui::txtCont);

    data::selUser = userMenu.getSelected();

    if(down & KEY_A)
    {
        if(data::curUser.titles.size() > 0)
        {
            textTitlePrep(data::curUser);
            ui::changeState(TXT_TTL);
        }
        else
            ui::showPopup(POP_FRAME_DEFAULT, ui::noSavesFound.c_str(), data::curUser.getUsername().c_str());
    }
    else if(down & KEY_Y)
    {
        for(unsigned i = 0; i < data::users.size(); i++)
            fs::dumpAllUserSaves(data::users[i]);
    }
    else if(down & KEY_X)
    {
        ui::textMode = false;
        ui::changeState(USR_SEL);
    }
    else if(down & KEY_ZR)
    {
        fs::unmountSave();
        ui::exMenuPrep();
        ui::changeState(EX_MNU);
    }
    else if(down & KEY_MINUS)
        ui::changeState(OPT_MNU);
}

void ui::textTitleMenuUpdate(const uint64_t& down, const uint64_t& held)
{
    titleMenu.handleInput(down, held);
    titleMenu.draw(ui::txtCont);

    data::selData = titleMenu.getSelected();

    if(down & KEY_A)
    {
        if(fs::mountSave(data::curUser, data::curData))
        {
            folderMenuPrepare(data::curUser, data::curData);
            ui::changeState(TXT_FLD);
        }
    }
    else if(down & KEY_Y)
    {
        fs::dumpAllUserSaves(data::curUser);
    }
    else if(down & KEY_MINUS)
    {
        if(ui::confirm(false, ui::confBlacklist.c_str(), data::curData.getTitle().c_str()))
            data::blacklistAdd(data::curData);

        textTitlePrep(data::curUser);
    }
    else if(down & KEY_L)
    {
        if(--data::selUser < 0)
            data::selUser = data::users.size() - 1;
        textTitlePrep(data::curUser);
        ui::showPopup(POP_FRAME_DEFAULT, data::curUser.getUsername().c_str());
    }
    else if(down & KEY_R)
    {
        if(++data::selUser > (int)data::users.size() - 1)
            data::selUser = 0;
        textTitlePrep(data::curUser);
        ui::showPopup(POP_FRAME_DEFAULT, data::curUser.getUsername().c_str());
    }
    else if(down & KEY_X)
    {
        data::favoriteTitle(data::curData);
        textTitlePrep(data::curUser);
    }
    else if(down & KEY_ZR)
    {
        if(data::curData.getType() == FsSaveDataType_System)
            ui::showMessage("*NO*", "Deleting system save archives is disabled.");
        else if(confirm(true, ui::confEraseNand.c_str(), data::curData.getTitle().c_str()))
        {
            fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId_User, data::curData.getSaveID());
            data::loadUsersTitles(false);
            ui::textTitlePrep(data::curUser);
        }
    }
    else if(down & KEY_B)
    {
        data::selData = 0;
        ui::changeState(TXT_USR);
    }
}

void ui::textFolderMenuUpdate(const uint64_t& down, const uint64_t& held)
{
    titleMenu.draw(ui::txtCont);
    folderMenu.handleInput(down, held);
    folderMenu.draw(ui::txtCont);

    if(down & KEY_A)
    {
        if(folderMenu.getSelected() == 0)
            createNewBackup(held);
        else
            overwriteBackup(folderMenu.getSelected() - 1);
    }
    else if(down & KEY_Y && folderMenu.getSelected() > 0)
        restoreBackup(folderMenu.getSelected() - 1);
    else if(down & KEY_X && folderMenu.getSelected() > 0)
        deleteBackup(folderMenu.getSelected() - 1);
    else if(down & KEY_MINUS)
    {
        advModePrep("sv:/", data::curData.getType(), true);
        ui::changeState(ADV_MDE);
    }
    else if(down & KEY_ZR && data::curData.getType() != FsSaveDataType_System && confirm(true, ui::confEraseFolder.c_str(), data::curData.getTitle().c_str()))
    {
        fs::delDir("sv:/");
        fsdevCommitDevice("sv");
    }
    else if(down & KEY_B)
    {
        fsdevUnmountDevice("sv");
        ui::changeState(TXT_TTL);
    }
}

void ui::exMenuPrep()
{
    exMenu.reset();
    exMenu.setParams(76, 98, 310);
    for(unsigned i = 0; i < 11; i++)
        exMenu.addOpt(ui::exMenuStr[i]);
}

void ui::updateExMenu(const uint64_t& down, const uint64_t& held)
{
    exMenu.handleInput(down, held);

    if(down & KEY_A)
    {
        fsdevUnmountDevice("sv");
        FsFileSystem sv;
        data::curData.setType(FsSaveDataType_System);
        switch(exMenu.getSelected())
        {
            case 0:
                data::curData.setType(FsSaveDataType_Bcat);
                advModePrep("sdmc:/", data::curData.getType(), false);
                ui::changeState(ADV_MDE);
                break;

            case 1:
                fsdevUnmountDevice("sv");
                fsOpenBisFileSystem(&sv, FsBisPartitionId_CalibrationFile, "");
                fsdevMountDevice("prodInfo-f", sv);
                advModePrep("profInfo-f:/", FsSaveDataType_System, false);
                ui::changeState(ADV_MDE);
                break;

            case 2:
                fsOpenBisFileSystem(&sv, FsBisPartitionId_SafeMode, "");
                fsdevMountDevice("safe", sv);
                advModePrep("safe:/", FsSaveDataType_System, false);
                ui::changeState(ADV_MDE);
                break;

            case 3:
                fsOpenBisFileSystem(&sv, FsBisPartitionId_System, "");
                fsdevMountDevice("sys", sv);
                advModePrep("sys:/", FsSaveDataType_System, false);
                ui::changeState(ADV_MDE);
                break;

            case 4:
                fsOpenBisFileSystem(&sv, FsBisPartitionId_User, "");
                fsdevMountDevice("user", sv);
                advModePrep("user:/", FsSaveDataType_System, false);
                ui::changeState(ADV_MDE);
                break;

            case 5:
                {
                    fsOpenBisFileSystem(&sv, FsBisPartitionId_System, "");
                    fsdevMountDevice("sv", sv);
                    std::string delPath = "sv:/Contents/placehld/";

                    fs::dirList plcHld(delPath);
                    for(unsigned i = 0; i < plcHld.getCount(); i++)
                    {
                        std::string fullPath = delPath + plcHld.getItem(i);
                        std::remove(fullPath.c_str());
                    }
                    fsdevUnmountDevice("sv");

                    if(ui::confirm(false, "Restart?"))
                    {
                        bpcInitialize();
                        bpcRebootSystem();
                    }
                }
                break;

            case 6:
                {
                    std::string idStr = util::getStringInput("0100000000000000", "Enter Process ID", 18, 0, NULL);
                    if(!idStr.empty())
                    {
                        uint64_t termID = std::strtoull(idStr.c_str(), NULL, 16);
                        if(R_SUCCEEDED(pmshellTerminateProgram(termID)))
                            ui::showMessage("Success!", "Process %s successfully shutdown.", idStr.c_str());
                    }
                }
                break;

            case 7:
                {
                    std::string idStr = util::getStringInput("8000000000000000", "Enter Sys Save ID", 18, 0, NULL);
                    uint64_t mountID = std::strtoull(idStr.c_str(), NULL, 16);
                    if(R_SUCCEEDED(fsOpen_SystemSaveData(&sv, FsSaveDataSpaceId_System, mountID, (AccountUid) {0})))
                    {
                        fsdevMountDevice("sv", sv);
                        advModePrep("sv:/", data::curData.getType(), true);
                        ui::changeState(ADV_MDE);
                    }
                }
                break;

            case 8:
                data::loadUsersTitles(true);
                break;

            case 9:
                {
                    FsFileSystem tromfs;
                    //Result res = romfsMountFromCurrentProcess("tromfs"); << Works too, but is kinda weird
                    if(R_SUCCEEDED(util::fsOpenDataFileSystemByCurrentProcess(&tromfs)))
                    {
                        fsdevMountDevice("tromfs", tromfs);
                        advModePrep("tromfs:/", FsSaveDataType_Account, false);
                        ui::changeState(ADV_MDE);
                    }
                }
                break;

            case 10:
                {
                    fs::logClose();
                    remove(std::string(fs::getWorkDir() + "log.txt").c_str());
                    std::string zipName = "JKSV - " + util::getDateTime(util::DATE_FMT_ASC) + ".zip";
                    zipFile jksv = zipOpen(std::string("sdmc:/" + zipName).c_str(), 0);
                    fs::copyDirToZip(fs::getWorkDir(), &jksv);
                    zipClose(jksv, NULL);
                    rename(std::string("sdmc:/" + zipName).c_str(), std::string(fs::getWorkDir() + zipName).c_str());
                    fs::logOpen();
                }
                break;
        }
    }
    else if(down & KEY_B)
    {
        fsdevUnmountDevice("sv");
        ui::changeState(ui::textMode ? TXT_USR : USR_SEL);
        prevState = USR_SEL;
    }

    exMenu.draw(ui::txtCont);
}

void ui::optMenuInit()
{
    optMenu.setParams(76, 98, 310);
    for(unsigned i = 0; i < 13; i++)
        optMenu.addOpt(ui::optMenuStr[i]);
}

void ui::updateOptMenu(const uint64_t& down, const uint64_t& held)
{
    optMenu.handleInput(down, held);

    //Update Menu Options to reflect changes
    optMenu.editOpt(0, optMenuStr[0] + getBoolText(data::incDev));
    optMenu.editOpt(1, optMenuStr[1] + getBoolText(data::autoBack));
    optMenu.editOpt(2, optMenuStr[2] + getBoolText(data::ovrClk));
    optMenu.editOpt(3, optMenuStr[3] + getBoolText(data::holdDel));
    optMenu.editOpt(4, optMenuStr[4] + getBoolText(data::holdRest));
    optMenu.editOpt(5, optMenuStr[5] + getBoolText(data::holdOver));
    optMenu.editOpt(6, optMenuStr[6] + getBoolText(data::forceMount));
    optMenu.editOpt(7, optMenuStr[7] + getBoolText(data::accSysSave));
    optMenu.editOpt(8, optMenuStr[8] + getBoolText(data::sysSaveWrite));
    optMenu.editOpt(9, optMenuStr[9] + getBoolText(ui::textMode));
    optMenu.editOpt(10, optMenuStr[10] + getBoolText(data::directFsCmd));
    optMenu.editOpt(11, optMenuStr[11] + getBoolText(data::skipUser));
    optMenu.editOpt(12, optMenuStr[12] + getBoolText(data::zip));

    if(down & KEY_A)
    {
        switch(optMenu.getSelected())
        {
            case 0:
                switchBool(data::incDev);
                break;

            case 1:
                switchBool(data::autoBack);
                break;

            case 2:
                switchBool(data::ovrClk);
                break;

            case 3:
                switchBool(data::holdDel);
                break;

            case 4:
                switchBool(data::holdRest);
                break;

            case 5:
                switchBool(data::holdOver);
                break;

            case 6:
                switchBool(data::forceMount);
                break;

            case 7:
                switchBool(data::accSysSave);
                break;

            case 8:
                switchBool(data::sysSaveWrite);
                break;

            case 9:
                switchBool(ui::textMode);
                break;

            case 10:
                switchBool(data::directFsCmd);
                break;

            case 11:
                switchBool(data::skipUser);
                break;

            case 12:
                switchBool(data::zip);
                break;
        }
    }
    else if(down & KEY_X)
        data::restoreDefaultConfig();
    else if(down & KEY_B)
        ui::changeState(ui::textMode ? TXT_USR : USR_SEL);

    optMenu.draw(ui::txtCont);
    drawTextWrap(ui::optMenuExp[optMenu.getSelected()].c_str(), frameBuffer, ui::shared, 466, 98, 18, ui::txtCont, 730);
}

