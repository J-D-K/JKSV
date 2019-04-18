#include <string>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <switch.h>
#include <sys/stat.h>

#include "ui.h"
#include "data.h"
#include "file.h"
#include "util.h"
#include "ex.h"

static ui::menu userMenu, titleMenu, devMenu;
extern ui::menu folderMenu;
extern std::vector<ui::button> usrNav, ttlNav, fldNav;

namespace ui
{
    void clsUserPrep()
    {
        userMenu.reset();

        userMenu.setParams(42, 98, 424);

        for(unsigned i = 0; i < data::users.size(); i++)
            userMenu.addOpt(data::users[i].getUsername());
    }

    void clsTitlePrep(data::user& u)
    {
        titleMenu.reset();
        titleMenu.setParams(42, 98, 424);

        for(unsigned i = 0; i < u.titles.size(); i++)
            titleMenu.addOpt(u.titles[i].getTitle());
    }

    void clsFolderPrep(data::user& usr, data::titledata& dat)
    {
        folderMenu.setParams(488, 98, 762);
        folderMenu.reset();

        util::makeTitleDir(usr, dat);
        std::string scanPath = util::getTitleDir(usr, dat);

        fs::dirList list(scanPath);
        folderMenu.addOpt("New");
        for(unsigned i = 0; i < list.getCount(); i++)
            folderMenu.addOpt(list.getItem(i));

        folderMenu.adjust();
    }

    void classicUserMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        userMenu.handleInput(down, held, p);
        userMenu.draw(mnuTxt);

        for(unsigned i = 0; i < usrNav.size(); i++)
            usrNav[i].update(p);

        if(down & KEY_A || usrNav[0].getEvent() == BUTTON_RELEASED)
        {
            data::curUser = data::users[userMenu.getSelected()];
            clsTitlePrep(data::curUser);

            mstate = CLS_TTL;
        }
        else if(down & KEY_Y || usrNav[1].getEvent() == BUTTON_RELEASED)
        {
            for(unsigned i = 0; i < data::users.size(); i++)
                fs::dumpAllUserSaves(data::users[i]);
        }
        else if(down & KEY_X || usrNav[2].getEvent() == BUTTON_RELEASED)
        {
            std::remove(std::string(fs::getWorkDir() + "cls.txt").c_str());
            clsMode = false;
            mstate = USR_SEL;
        }
        else if(down & KEY_MINUS || usrNav[3].getEvent() == BUTTON_RELEASED)
        {
            fsdevUnmountDevice("sv");
            ui::exMenuPrep();
            ui::mstate = EX_MNU;
        }
    }

    void classicTitleMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
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
                clsFolderPrep(data::curUser, data::curData);
                folderMenuInfo = util::getInfoString(data::curUser, data::curData);

                mstate = CLS_FLD;
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
            if(ui::confirm(confStr))
                data::blacklistAdd(data::curUser, data::curUser.titles[titleMenu.getSelected()]);
        }
        else if(down & KEY_B || ttlNav[3].getEvent() == BUTTON_RELEASED)
            mstate = CLS_USR;
    }

    void classicFolderMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
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
                if(held & KEY_R)
                    folder = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
                else if(held & KEY_L)
                    folder = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YDM);
                else if(held & KEY_ZL)
                    folder = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_HOYSTE);
                else
                {
                    const std::string dict[] =
                    {
                        util::getDateTime(util::DATE_FMT_YMD).c_str(),
                        util::getDateTime(util::DATE_FMT_YDM).c_str(),
                        util::getDateTime(util::DATE_FMT_HOYSTE).c_str(),
                        data::curUser.getUsernameSafe().c_str(),
                        data::curData.getTitle().length() < 24 ? data::curData.getTitleSafe() : util::generateAbbrev(data::curData)
                    };
                    folder = util::getStringInput("", "New Folder", 64, 5, dict);
                }
                if(!folder.empty())
                {
                    std::string path = util::getTitleDir(data::curUser, data::curData) + "/" + folder;
                    mkdir(path.c_str(), 777);
                    path += "/";

                    std::string root = "sv:/";
                    fs::copyDirToDir(root, path);

                    clsFolderPrep(data::curUser, data::curData);
                }
            }
            else
            {
                std::string scanPath = util::getTitleDir(data::curUser, data::curData);
                fs::dirList list(scanPath);

                std::string folderName = list.getItem(folderMenu.getSelected() - 1);
                if(confirm("Are you sure you want to overwrite \"" + folderName + "\"?"))
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
            if(data::curData.getType() != FsSaveDataType_SystemSaveData)
            {
                if(folderMenu.getSelected() > 0)
                {
                    std::string scanPath = util::getTitleDir(data::curUser, data::curData);
                    fs::dirList list(scanPath);

                    std::string folderName = list.getItem(folderMenu.getSelected() - 1);
                    if(confirm("Are you sure you want to restore \"" + folderName + "\"?"))
                    {
                        std::string fromPath = util::getTitleDir(data::curUser, data::curData) + folderName + "/";
                        std::string root = "sv:/";

                        fs::delDir(root);
                        fsdevCommitDevice("sv");

                        fs::copyDirToDirCommit(fromPath, root, "sv");
                    }
                }
            }
            else
                ui::showMessage("Writing data to system save data is not allowed currently. It CAN brick your system.", "Sorry, bro:");
        }
        else if(down & KEY_X || fldNav[2].getEvent() == BUTTON_RELEASED)
        {
            if(folderMenu.getSelected() > 0)
            {
                std::string scanPath = util::getTitleDir(data::curUser, data::curData);
                fs::dirList list(scanPath);

                std::string folderName = list.getItem(folderMenu.getSelected() - 1);
                if(confirm("Are you sure you want to delete \"" + folderName + "\"?"))
                {
                    std::string delPath = scanPath + folderName + "/";
                    fs::delDir(delPath);
                }

                clsFolderPrep(data::curUser, data::curData);
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
            mstate = CLS_TTL;
        }
    }

    void exMenuPrep()
    {
        devMenu.reset();
        devMenu.setParams(42, 98, 424);
        devMenu.addOpt("SD to SD Browser");
        devMenu.addOpt("Bis: PRODINFOF");
        devMenu.addOpt("Bis: SAFE");
        devMenu.addOpt("Bis: SYSTEM");
        devMenu.addOpt("Bis: USER");
        devMenu.addOpt("NAND Backup (exFat)");
        devMenu.addOpt("NAND Backup (FAT32)");
        devMenu.addOpt("Remove Downloaded Update");
        devMenu.addOpt("Terminate Process ID");
        devMenu.addOpt("Mount System Save ID");
        devMenu.addOpt("Mount Process RomFS");
    }

    void updateExMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        devMenu.handleInput(down, held, p);

        if(down & KEY_A)
        {
            FsFileSystem sv;
            data::curData.setType(FsSaveDataType_SystemSaveData);
            switch(devMenu.getSelected())
            {
                case 0:
                    data::curData.setType(FsSaveDataType_SaveData);
                    fsdevUnmountDevice("sv");
                    advModePrep("sdmc:/", false);
                    mstate = ADV_MDE;
                    prevState = EX_MNU;
                    break;

                case 1:
                    fsdevUnmountDevice("sv");
                    fsOpenBisFileSystem(&sv, 28, "");
                    fsdevMountDevice("prodInfo-f", sv);

                    advModePrep("profInfo-f:/", false);
                    mstate = ADV_MDE;
                    prevState = EX_MNU;
                    break;

                case 2:
                    fsdevUnmountDevice("sv");
                    fsOpenBisFileSystem(&sv, 29, "");
                    fsdevMountDevice("safe", sv);

                    advModePrep("safe:/", false);
                    mstate = ADV_MDE;
                    prevState = EX_MNU;
                    break;

                case 3:
                    fsdevUnmountDevice("sv");
                    fsOpenBisFileSystem(&sv, 31, "");
                    fsdevMountDevice("sys", sv);

                    advModePrep("sys:/", false);
                    mstate = ADV_MDE;
                    prevState = EX_MNU;
                    break;

                case 4:
                    fsdevUnmountDevice("sv");
                    fsOpenBisFileSystem(&sv, 30, "");
                    fsdevMountDevice("user", sv);

                    advModePrep("user:/", false);
                    mstate = ADV_MDE;
                    prevState = EX_MNU;
                    break;

                case 5:
                    {
                        fsdevUnmountDevice("sv");

                        FsStorage nand;
                        fsOpenBisStorage(&nand, 20);
                        uint64_t nandSize = 0, offset = 0;
                        fsStorageGetSize(&nand, &nandSize);

                        std::fstream nandOut("sdmc:/JKSV/nand.bin", std::ios::out | std::ios::binary);

                        size_t nandBuffSize = 1024 * 1024 * 4;
                        uint8_t *nandBuff = new uint8_t[nandBuffSize];

                        progBar nandProg(nandSize);

                        while(offset < nandSize)
                        {
                            size_t readLen = 0;
                            if(offset + nandBuffSize < nandSize)
                                readLen = nandBuffSize;
                            else
                                readLen = nandSize - offset;

                            if(R_SUCCEEDED(fsStorageRead(&nand, offset, nandBuff, readLen)))
                            {
                                nandOut.write((char *)nandBuff, readLen);
                                offset += readLen;
                            }
                            else
                            {
                                ui::showMessage("Something went wrong while dumping your NAND.", "*ERROR*");
                                break;
                            }
                            gfxBeginFrame();
                            nandProg.update(offset);
                            nandProg.draw("", "Copying NAND");
                            gfxEndFrame();
                        }

                        delete[] nandBuff;
                        nandOut.close();
                        fsStorageClose(&nand);
                    }
                    break;

                case 6:
                    {
                        unsigned fcount = 1;
                        fsdevUnmountDevice("sv");

                        FsStorage nand;
                        fsOpenBisStorage(&nand, 20);
                        uint64_t nandSize = 0, offset = 0;
                        fsStorageGetSize(&nand, &nandSize);

                        std::fstream nandOut("sdmc:/JKSV/nand.bin.00", std::ios::out | std::ios::binary);

                        size_t nandBuffSize = 1024 * 1024 * 3;
                        uint8_t *nandBuff = new uint8_t[nandBuffSize];

                        progBar nandProg(nandSize);

                        while(offset < nandSize)
                        {
                            size_t readLen = 0;
                            if(offset + nandBuffSize < nandSize)
                                readLen = nandBuffSize;
                            else
                                readLen = nandSize - offset;

                            if(R_SUCCEEDED(fsStorageRead(&nand, offset, nandBuff, readLen)))
                            {
                                if((size_t)nandOut.tellp() + readLen >= 0x100000000)
                                {
                                    nandOut.close();
                                    char newPath[128];
                                    sprintf(newPath, "sdmc:/JKSV/nand.bin.%02d", fcount++);
                                    nandOut.open(newPath, std::ios::out | std::ios::binary);
                                }
                                nandOut.write((char *)nandBuff, readLen);
                                offset += readLen;
                            }
                            else
                            {
                                ui::showMessage("Something went wrong while dumping your NAND.", "*ERROR*");
                                break;
                            }

                            gfxBeginFrame();
                            nandProg.update(offset);
                            nandProg.draw("", "Copying NAND");
                            gfxEndFrame();
                        }

                        delete[] nandBuff;
                        nandOut.close();
                        fsStorageClose(&nand);
                    }
                    break;

                case 7:
                    {
                        fsdevUnmountDevice("sv");
                        fsOpenBisFileSystem(&sv, 31, "");
                        fsdevMountDevice("sv", sv);
                        std::string delPath = "sv:/Contents/placehld/";

                        fs::dirList plcHld(delPath);
                        for(unsigned i = 0; i < plcHld.getCount(); i++)
                        {
                            std::string fullPath = delPath + plcHld.getItem(i);
                            std::remove(fullPath.c_str());
                        }
                        fsdevUnmountDevice("sv");

                        if(ui::confirm("System needs to be restarted for nag to go away. Reboot now?"))
                        {
                            bpcInitialize();
                            bpcRebootSystem();
                        }
                    }
                    break;

                case 8:
                    {
                        fsdevUnmountDevice("sv");
                        std::string idStr = util::getStringInput("0100000000000000", "Enter Process ID", 18, 0, NULL);
                        if(!idStr.empty())
                        {
                            uint64_t termID = std::strtoull(idStr.c_str(), NULL, 16);
                            pmshellInitialize();
                            if(R_SUCCEEDED(pmshellTerminateProcessByTitleId(termID)))
                                ui::showMessage("Process " + idStr + " successfully shutdown.", "Success!");
                            pmshellExit();
                        }
                    }
                    break;

                case 9:
                    {
                        fsdevUnmountDevice("sv");
                        std::string idStr = util::getStringInput("8000000000000000", "Enter Sys Save ID", 18, 0, NULL);
                        uint64_t mountID = std::strtoull(idStr.c_str(), NULL, 16);
                        if(R_SUCCEEDED(fsMount_SystemSaveData(&sv, mountID)))
                        {
                            fsdevMountDevice("sv", sv);
                            advModePrep("sv:/", true);
                            data::curData.setType(FsSaveDataType_SystemSaveData);
                            prevState = EX_MNU;
                            mstate = ADV_MDE;
                        }
                    }
                    break;

                case 10:
                    {
                        fsdevUnmountDevice("sv");
                        FsFileSystem tromfs;
                        Result res = fsOpenDataFileSystemByCurrentProcess(&tromfs);
                        if(R_SUCCEEDED(res))
                        {
                            fsdevMountDevice("tromfs", tromfs);
                            advModePrep("tromfs:/", false);
                            data::curData.setType(FsSaveDataType_SystemSaveData);
                            ui::mstate = ADV_MDE;
                            ui::prevState = EX_MNU;
                        }
                    }
            }
        }
        else if(down & KEY_B)
        {
            fsdevUnmountDevice("sv");
            if(ui::clsMode)
                mstate = CLS_USR;
            else
                mstate = USR_SEL;

            prevState = USR_SEL;
        }

        devMenu.draw(mnuTxt);
    }
}
