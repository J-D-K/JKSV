#include <string>
#include <vector>
#include <switch.h>
#include <sys/stat.h>

#include "ui.h"
#include "data.h"
#include "file.h"
#include "util.h"

static ui::menu userMenu, titleMenu;
extern ui::menu folderMenu;

namespace ui
{
    void clsUserPrep()
    {
        userMenu.reset();

        userMenu.setParams(16, 88, 424);

        for(unsigned i = 0; i < data::users.size(); i++)
            userMenu.addOpt(data::users[i].getUsername());
    }

    void clsTitlePrep(data::user& u)
    {
        titleMenu.reset();
        titleMenu.setParams(16, 88, 424);

        for(unsigned i = 0; i < u.titles.size(); i++)
            titleMenu.addOpt(u.titles[i].getTitle());
    }

    void clsFolderPrep(data::user& usr, data::titledata& dat)
    {
        folderMenu.setParams(458, 88, 806);
        folderMenu.reset();

        util::makeTitleDir(usr, dat);
        std::string scanPath = util::getTitleDir(usr, dat);

        fs::dirList list(scanPath);
        folderMenu.addOpt("New");
        for(unsigned i = 0; i < list.getCount(); i++)
            folderMenu.addOpt(list.getItem(i));
    }

    void classicUserMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        userMenu.handleInput(down, held, p);
        userMenu.draw(mnuTxt);

        if(down & KEY_A)
        {
            data::curUser = data::users[userMenu.getSelected()];
            clsTitlePrep(data::curUser);

            mstate = CLS_TTL;
        }
        else if(down & KEY_Y)
        {
            for(unsigned i = 0; i < data::users.size(); i++)
                fs::dumpAllUserSaves(data::users[i]);
        }
        else if(down & KEY_X)
        {
            std::remove(std::string(fs::getWorkDir() + "cls.txt").c_str());
            clsMode = false;
            mstate = USR_SEL;
        }
    }

    void classicTitleMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        titleMenu.handleInput(down, held, p);
        titleMenu.draw(mnuTxt);

        if(down & KEY_A)
        {
            data::curData = data::curUser.titles[titleMenu.getSelected()];

            if(fs::mountSave(data::curUser, data::curData))
            {
                util::makeTitleDir(data::curUser, data::curData);
                clsFolderPrep(data::curUser, data::curData);
                folderMenuInfo = util::getWrappedString(util::getInfoString(data::curUser, data::curData), 18, 256);

                mstate = CLS_FLD;
            }
        }
        else if(down & KEY_Y)
        {
            fs::dumpAllUserSaves(data::curUser);
        }
        else if(down & KEY_B)
            mstate = CLS_USR;
    }

    void classicFolderMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        titleMenu.draw(mnuTxt);
        folderMenu.handleInput(down, held, p);
        folderMenu.draw(mnuTxt);

        if(down & KEY_A || folderMenu.getTouchEvent() == MENU_DOUBLE_REL)
        {
            if(folderMenu.getSelected() == 0)
            {
                ui::keyboard key;
                std::string folder = util::safeString(key.getString(""));
                if(!folder.empty())
                {
                    std::string path = util::getTitleDir(data::curUser, data::curData) + "/" + folder;
                    mkdir(path.c_str(), 777);
                    path += "/";

                    std::string root = "sv:/";
                    fs::copyDirToDir(root, path);

                    folderMenuPrepare(data::curUser, data::curData);
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
                    std::string root = "sv:/";

                    fs::copyDirToDir(root, toPath);
                }
            }
        }
        else if(down & KEY_Y)
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
                ui::showMessage("Writing data to system save data is not allowed currently. It CAN brick your system.");
        }
        else if(down & KEY_X)
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

                folderMenuPrepare(data::curUser, data::curData);
            }
        }
        else if(down & KEY_MINUS)
        {
            advModePrep();
            mstate = ADV_MDE;
        }
        else if(down & KEY_B)
        {
            fsdevUnmountDevice("sv");
            if(clsMode)
                mstate = CLS_TTL;
        }
    }
}
