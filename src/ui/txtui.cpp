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

//Help text for options
static const std::string optHelpStrings[] =
{
    "Includes all Device Save data in Account Saves.",
    "Automatically create a backup before restoring a save. Just to be safe.",
    "Apply a small overclock to 1224MHz at boot. This is to help the text UI mode run smoothly.",
    "Whether or not holding \ue0e0 is required when deleting save folders and files.",
    "Whether or not holding \ue0e0 is required when restoring saves to games.",
    "Whether or not holding \ue0e0 is required when overwriting save folders.",
    "When on, JKSV will only show save data that can be opened. When off, JKSV shows everything.",
    "Includes system save data tied to accounts.",
    "Controls whether or not system saves can be restored/overwritten. *THIS CAN BE EXTREMELY DANGEROUS*.",
    "Changes the UI to a text menu based one like the 3DS version of JKSV.",
    "Directly uses the Switch's FS commands to copy files instead of stdio.",
    "Skips the user selection screen and jumps straight the first account's titles."
};

static inline void switchBool(bool& sw)
{
    sw ? sw = false : sw = true;
}

static inline std::string getBoolText(bool b)
{
    return b ? ui::on : ui::off;
}

void textFolderPrep(data::user& usr, data::titledata& dat)
{
    folderMenu.setParams(466, 98, 730);
    folderMenu.reset();

    dat.createDir();

    fs::dirList list(dat.getPath());
    folderMenu.addOpt("New");
    for(unsigned i = 0; i < list.getCount(); i++)
        folderMenu.addOpt(list.getItem(i));

    folderMenu.adjust();
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

    if(down & KEY_A)
    {
        if(data::users[userMenu.getSelected()].titles.size() > 0)
        {
            data::curUser = data::users[userMenu.getSelected()];
            textTitlePrep(data::curUser);
            mstate = TXT_TTL;
        }
        else
            ui::showPopup("No saves available for " + data::users[userMenu.getSelected()].getUsername() + ".", POP_FRAME_DEFAULT);
    }
    else if(down & KEY_Y)
    {
        for(unsigned i = 0; i < data::users.size(); i++)
            fs::dumpAllUserSaves(data::users[i]);
    }
    else if(down & KEY_X)
    {
        ui::textMode = false;
        mstate = USR_SEL;
    }
    else if(down & KEY_ZR)
    {
        fs::unmountSave();
        ui::exMenuPrep();
        ui::mstate = EX_MNU;
    }
    else if(down & KEY_MINUS)
        ui::mstate = OPT_MNU;
}

void ui::textTitleMenuUpdate(const uint64_t& down, const uint64_t& held)
{
    titleMenu.handleInput(down, held);
    titleMenu.draw(ui::txtCont);

    if(down & KEY_A)
    {
        data::curData = data::curUser.titles[titleMenu.getSelected()];

        if(fs::mountSave(data::curUser, data::curData))
        {
            textFolderPrep(data::curUser, data::curData);
            mstate = TXT_FLD;
        }
    }
    else if(down & KEY_Y)
    {
        fs::dumpAllUserSaves(data::curUser);
    }
    else if(down & KEY_MINUS)
    {
        if(ui::confirm(false, ui::confBlacklist.c_str(), data::curUser.titles[titleMenu.getSelected()].getTitle().c_str()))
            data::blacklistAdd(data::curUser, data::curUser.titles[titleMenu.getSelected()]);

        textTitlePrep(data::curUser);
    }
    else if(down & KEY_L)
    {
        if(--data::selUser < 0)
            data::selUser = data::users.size() - 1;

        data::curUser = data::users[data::selUser];
        textTitlePrep(data::curUser);

        ui::showPopup(data::curUser.getUsername(), POP_FRAME_DEFAULT);
    }
    else if(down & KEY_R)
    {
        if(++data::selUser > (int)data::users.size() - 1)
            data::selUser = 0;

        data::curUser = data::users[data::selUser];
        textTitlePrep(data::curUser);

        ui::showPopup(data::curUser.getUsername(), POP_FRAME_DEFAULT);
    }
    else if(down & KEY_X)
    {
        unsigned sel = titleMenu.getSelected();
        if(!data::curUser.titles[sel].getFav())
            data::favoriteAdd(data::curUser, data::curUser.titles[sel]);
        else
            data::favoriteRemove(data::curUser, data::curUser.titles[sel]);

        textTitlePrep(data::curUser);
    }
    else if(down & KEY_ZR)
    {
        data::titledata tempData = data::curUser.titles[titleMenu.getSelected()];
        if(tempData.getType() == FsSaveDataType_System)
            ui::showMessage("*NO*", "Deleting system save archives is disabled.");
        else if(confirm(true, ui::confEraseNand.c_str(), tempData.getTitle().c_str()))
        {
            fsDeleteSaveDataFileSystemBySaveDataSpaceId(FsSaveDataSpaceId_User, tempData.getSaveID());

            data::loadUsersTitles(false);
            data::curUser = data::users[data::selUser];
            ui::textTitlePrep(data::curUser);
        }
    }
    else if(down & KEY_B)
        mstate = TXT_USR;
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
    else if(down & KEY_Y)
        restoreBackup(folderMenu.getSelected() - 1);
    else if(down & KEY_X)
        deleteBackup(folderMenu.getSelected() - 1);
    else if(down & KEY_MINUS)
    {
        advModePrep("sv:/", true);
        mstate = ADV_MDE;
    }
    else if(down & KEY_ZR && data::curData.getType() != FsSaveDataType_System && confirm(true, ui::confEraseFolder.c_str(), data::curData.getTitle().c_str()))
    {
        fs::delDir("sv:/");
        fsdevCommitDevice("sv");
    }
    else if(down & KEY_B)
    {
        fsdevUnmountDevice("sv");
        mstate = TXT_TTL;
    }
}

void ui::exMenuPrep()
{
    exMenu.reset();
    exMenu.setParams(76, 98, 310);
    for(unsigned i = 0; i < 10; i++)
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
                advModePrep("sdmc:/", false);
                mstate = ADV_MDE;
                prevState = EX_MNU;
                break;

            case 1:
                fsdevUnmountDevice("sv");
                fsOpenBisFileSystem(&sv, FsBisPartitionId_CalibrationFile, "");
                fsdevMountDevice("prodInfo-f", sv);

                advModePrep("profInfo-f:/", false);
                mstate = ADV_MDE;
                prevState = EX_MNU;
                break;

            case 2:
                fsOpenBisFileSystem(&sv, FsBisPartitionId_SafeMode, "");
                fsdevMountDevice("safe", sv);

                advModePrep("safe:/", false);
                mstate = ADV_MDE;
                prevState = EX_MNU;
                break;

            case 3:
                fsOpenBisFileSystem(&sv, FsBisPartitionId_System, "");
                fsdevMountDevice("sys", sv);

                advModePrep("sys:/", false);
                mstate = ADV_MDE;
                prevState = EX_MNU;
                break;

            case 4:
                fsOpenBisFileSystem(&sv, FsBisPartitionId_User, "");
                fsdevMountDevice("user", sv);

                advModePrep("user:/", false);
                mstate = ADV_MDE;
                prevState = EX_MNU;
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
                        advModePrep("sv:/", true);
                        data::curData.setType(FsSaveDataType_SystemBcat);
                        prevState = EX_MNU;
                        mstate = ADV_MDE;
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
                        advModePrep("tromfs:/", false);
                        data::curData.setType(FsSaveDataType_System);
                        ui::mstate = ADV_MDE;
                        ui::prevState = EX_MNU;
                    }
                }
                break;
        }
    }
    else if(down & KEY_B)
    {
        fsdevUnmountDevice("sv");
        if(ui::textMode)
            mstate = TXT_USR;
        else
            mstate = USR_SEL;

        prevState = USR_SEL;
    }

    exMenu.draw(ui::txtCont);
}

void ui::optMenuInit()
{
    optMenu.setParams(76, 98, 310);
    for(unsigned i = 0; i < 12; i++)
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
        }
    }
    else if(down & KEY_B)
        ui::mstate = ui::textMode ? TXT_USR : USR_SEL;

    optMenu.draw(ui::txtCont);
    drawTextWrap(optHelpStrings[optMenu.getSelected()].c_str(), frameBuffer, ui::shared, 466, 98, 18, ui::txtCont, 730);
}

