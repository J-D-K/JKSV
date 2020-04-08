#include <string>
#include <vector>
#include <cstdlib>
#include <switch.h>
#include <sys/stat.h>

#include "ui.h"
#include "data.h"
#include "file.h"
#include "util.h"
#include "ex.h"

static ui::menu userMenu, titleMenu, exMenu, optMenu;
extern ui::menu folderMenu;
extern std::vector<ui::button> usrNav, ttlNav, fldNav;

//Help text for options
static const std::string optHelp[] =
{
    "Includes all Device Save data in Account Saves.",
    "Automatically create a backup before restoring a save. Just to be safe.",
    "Apply a small overclock to 1224MHz at boot."
};

static inline void switchBool(bool& sw)
{
    sw ? sw = false : sw = true;
}

static inline std::string getBoolText(bool b)
{
    return b ? "On" : "Off";
}

namespace ui
{
    void textUserPrep()
    {
        userMenu.reset();

        userMenu.setParams(76, 98, 310);

        for(unsigned i = 0; i < data::users.size(); i++)
            userMenu.addOpt(data::users[i].getUsername());
    }

    void textTitlePrep(data::user& u)
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

    void textFolderPrep(data::user& usr, data::titledata& dat)
    {
        folderMenu.setParams(466, 98, 730);
        folderMenu.reset();

        util::makeTitleDir(usr, dat);
        std::string scanPath = util::getTitleDir(usr, dat);

        fs::dirList list(scanPath);
        folderMenu.addOpt("New");
        for(unsigned i = 0; i < list.getCount(); i++)
            folderMenu.addOpt(list.getItem(i));

        folderMenu.adjust();
    }

    void textUserMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        userMenu.handleInput(down, held, p);
        userMenu.draw(mnuTxt);

        for(unsigned i = 0; i < usrNav.size(); i++)
            usrNav[i].update(p);

        if(down & KEY_A || usrNav[0].getEvent() == BUTTON_RELEASED)
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
        else if(down & KEY_Y || usrNav[1].getEvent() == BUTTON_RELEASED)
        {
            for(unsigned i = 0; i < data::users.size(); i++)
                fs::dumpAllUserSaves(data::users[i]);
        }
        else if(down & KEY_X || usrNav[2].getEvent() == BUTTON_RELEASED)
        {
            std::remove(std::string(fs::getWorkDir() + "cls.txt").c_str());
            ui::textMode = false;
            mstate = USR_SEL;
        }
        else if(down & KEY_MINUS || usrNav[3].getEvent() == BUTTON_RELEASED)
        {
            fsdevUnmountDevice("sv");
            ui::exMenuPrep();
            ui::mstate = EX_MNU;
        }
        else if(down & KEY_ZR)
        {
            ui::mstate = OPT_MNU;
        }
    }

    void textTitleMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        titleMenu.handleInput(down, held, p);
        titleMenu.draw(mnuTxt);

        for(unsigned i = 0; i < ttlNav.size(); i++)
            ttlNav[i].update(p);

        if(down & KEY_A || ttlNav[0].getEvent() == BUTTON_RELEASED)
        {
            data::curData = data::curUser.titles[titleMenu.getSelected()];

            if(fs::mountSave(data::curUser, data::curData))
            {
                util::makeTitleDir(data::curUser, data::curData);
                textFolderPrep(data::curUser, data::curData);

                mstate = TXT_FLD;
            }
        }
        else if(down & KEY_Y || ttlNav[1].getEvent() == BUTTON_RELEASED)
        {
            fs::dumpAllUserSaves(data::curUser);
        }
        else if(down & KEY_X || ttlNav[2].getEvent() == BUTTON_RELEASED)
        {
            std::string confStr = "Are you 100% sure you want to add \"" + data::curUser.titles[titleMenu.getSelected()].getTitle() + \
                                  "\" to your blacklist?";
            if(ui::confirm(confStr, false))
                data::blacklistAdd(data::curUser, data::curUser.titles[titleMenu.getSelected()]);

            textTitlePrep(data::curUser);
        }
        else if(down & KEY_L)
        {
            data::selUser--;
            if(data::selUser < 0)
                data::selUser = data::users.size() -1;

            data::curUser = data::users[data::selUser];
            textTitlePrep(data::curUser);

            ui::showPopup(data::curUser.getUsername(), POP_FRAME_DEFAULT);
        }
        else if(down & KEY_R)
        {
            data::selUser++;
            if(data::selUser > (int)data::users.size() - 1)
                data::selUser = 0;

            data::curUser = data::users[data::selUser];
            textTitlePrep(data::curUser);

            ui::showPopup(data::curUser.getUsername(), POP_FRAME_DEFAULT);
        }
        else if(down & KEY_MINUS)
        {
            unsigned sel = titleMenu.getSelected();
            if(!data::curUser.titles[sel].getFav())
                data::favoriteAdd(data::curUser, data::curUser.titles[sel]);
            else
                data::favoriteRemove(data::curUser, data::curUser.titles[sel]);

            textTitlePrep(data::curUser);
        }
        else if(down & KEY_B || ttlNav[3].getEvent() == BUTTON_RELEASED)
            mstate = TXT_USR;
    }

    void textFolderMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        titleMenu.draw(mnuTxt);
        folderMenu.handleInput(down, held, p);
        folderMenu.draw(mnuTxt);

        for(unsigned i = 0; i < fldNav.size(); i++)
            fldNav[i].update(p);

        if(down & KEY_A || fldNav[0].getEvent() == BUTTON_RELEASED || folderMenu.getTouchEvent() == MENU_DOUBLE_REL)
        {
            if(folderMenu.getSelected() == 0)
            {
                std::string folder;
                //Add back 3DS shortcut thing
                if(held & KEY_R || data::isAppletMode())
                    folder = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
                else if(held & KEY_L)
                    folder = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YDM);
                else if(held & KEY_ZL)
                    folder = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_HOYSTE);
                else
                {
                    const std::string dict[] =
                    {
                        util::getDateTime(util::DATE_FMT_YMD),
                        util::getDateTime(util::DATE_FMT_YDM),
                        util::getDateTime(util::DATE_FMT_HOYSTE),
                        util::getDateTime(util::DATE_FMT_JHK),
                        data::curUser.getUsernameSafe().c_str(),
                        data::curData.getTitle().length() < 24 ? data::curData.getTitleSafe() : util::generateAbbrev(data::curData)
                    };
                    folder = util::getStringInput("", "New Folder", 64, 6, dict);
                }
                if(!folder.empty())
                {
                    std::string path = util::getTitleDir(data::curUser, data::curData) + "/" + folder;
                    mkdir(path.c_str(), 777);
                    path += "/";

                    std::string root = "sv:/";
                    fs::copyDirToDir(root, path);

                    textFolderPrep(data::curUser, data::curData);
                }
            }
            else
            {
                std::string scanPath = util::getTitleDir(data::curUser, data::curData);
                fs::dirList list(scanPath);

                std::string folderName = list.getItem(folderMenu.getSelected() - 1);
                if(confirm("Are you sure you want to overwrite \"" + folderName + "\"?", true))
                {
                    std::string toPath = util::getTitleDir(data::curUser, data::curData) + folderName + "/";
                    //Delete and recreate
                    fs::delDir(toPath);
                    mkdir(toPath.c_str(), 777);

                    std::string root = "sv:/";

                    fs::copyDirToDir(root, toPath);
                }
            }
        }
        else if(down & KEY_Y || fldNav[1].getEvent() == BUTTON_RELEASED)
        {
            if(data::curData.getType() != FsSaveDataType_System && folderMenu.getSelected() > 0)
            {
                std::string scanPath = util::getTitleDir(data::curUser, data::curData);
                fs::dirList list(scanPath);

                std::string folderName = list.getItem(folderMenu.getSelected() - 1);
                if(confirm("Are you sure you want to restore \"" + folderName + "\"?", true))
                {
                    if(data::autoBack)
                    {
                        std::string autoFolder = util::getTitleDir(data::curUser, data::curData) + "/AUTO - " + data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
                        mkdir(autoFolder.c_str(), 777);
                        autoFolder += "/";

                        std::string root = "sv:/";
                        fs::copyDirToDir(root, autoFolder);
                    }

                    std::string fromPath = util::getTitleDir(data::curUser, data::curData) + folderName + "/";
                    std::string root = "sv:/";

                    fs::delDir(root);
                    fsdevCommitDevice("sv");

                    fs::copyDirToDirCommit(fromPath, root, "sv");

                    if(data::autoBack)
                        folderMenuPrepare(data::curUser, data::curData);
                }
            }
        }
        else if(down & KEY_X || fldNav[2].getEvent() == BUTTON_RELEASED)
        {
            if(folderMenu.getSelected() > 0)
            {
                std::string scanPath = util::getTitleDir(data::curUser, data::curData);
                fs::dirList list(scanPath);

                std::string folderName = list.getItem(folderMenu.getSelected() - 1);
                if(confirm("Are you sure you want to delete \"" + folderName + "\"?", true))
                {
                    std::string delPath = scanPath + folderName + "/";
                    fs::delDir(delPath);
                }

                textFolderPrep(data::curUser, data::curData);
            }
        }
        else if(down & KEY_MINUS)
        {
            advModePrep("sv:/", true);
            mstate = ADV_MDE;
        }
        else if(down & KEY_B || fldNav[3].getEvent() == BUTTON_RELEASED)
        {
            fsdevUnmountDevice("sv");
            mstate = TXT_TTL;
        }
    }

    void exMenuPrep()
    {
        exMenu.reset();
        exMenu.setParams(76, 98, 310);
        exMenu.addOpt("SD to SD Browser");
        exMenu.addOpt("Bis: PRODINFOF");
        exMenu.addOpt("Bis: SAFE");
        exMenu.addOpt("Bis: SYSTEM");
        exMenu.addOpt("Bis: USER");
        exMenu.addOpt("Remove Downloaded Update");
        exMenu.addOpt("Terminate Process ID");
        exMenu.addOpt("Mount System Save ID");
        exMenu.addOpt("Rescan Titles");
        exMenu.addOpt("Mount Process RomFS");
    }

    void updateExMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        exMenu.handleInput(down, held, p);

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

                        if(ui::confirm("System needs to be restarted for nag to go away. Reboot now?", false))
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
                            pmshellInitialize();
                            if(R_SUCCEEDED(pmshellTerminateProgram(termID)))
                                ui::showMessage("Process " + idStr + " successfully shutdown.", "Success!");
                            pmshellExit();
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
                    data::init();
                    break;

                case 9:
                    {
                        FsFileSystem tromfs;
                        Result res = fsOpenDataFileSystemByCurrentProcess(&tromfs);
                        if(R_SUCCEEDED(res))
                        {
                            fsdevMountDevice("tromfs", tromfs);
                            advModePrep("tromfs:/", false);
                            data::curData.setType(FsSaveDataType_SystemBcat);
                            ui::mstate = ADV_MDE;
                            ui::prevState = EX_MNU;
                        }
                    }
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

        exMenu.draw(mnuTxt);
    }

    void optMenuInit()
    {
        optMenu.setParams(76, 98, 310);
        optMenu.addOpt("Inc. Dev Sv");
        optMenu.addOpt("Auto Backup");
        optMenu.addOpt("OverClock");
    }

    void updateOptMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        optMenu.handleInput(down, held, p);

        //Update Menu Options to reflect changes
        optMenu.editOpt(0, "Include Dev Sv: " + getBoolText(data::incDev));
        optMenu.editOpt(1, "Auto Backup: " + getBoolText(data::autoBack));
        optMenu.editOpt(2, "Overclock: " + getBoolText(data::ovrClk));

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
            }
        }
        else if(down & KEY_B)
        {
            ui::mstate = ui::textMode ? TXT_USR : USR_SEL;
        }

        optMenu.draw(ui::mnuTxt);
        drawTextWrap(optHelp[optMenu.getSelected()].c_str(), frameBuffer, ui::shared, 466, 98, 24, ui::mnuTxt, 730);
    }
}
