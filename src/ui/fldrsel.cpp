#include <string>
#include <vector>
#include <sys/stat.h>

#include "ui.h"
#include "uiupdate.h"
#include "util.h"

ui::menu folderMenu;

extern std::vector<ui::button> fldNav;

namespace ui
{
    void folderMenuPrepare(data::user& usr, data::titledata& dat)
    {
        folderMenu.setParams(340, 88, 924);
        folderMenu.reset();

        util::makeTitleDir(usr, dat);
        std::string scanPath = util::getTitleDir(usr, dat);

        fs::dirList list(scanPath);
        folderMenu.addOpt("New");
        for(unsigned i = 0; i < list.getCount(); i++)
            folderMenu.addOpt(list.getItem(i));
    }

    void updateFolderMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p)
    {
        folderMenu.handleInput(down, held, p);

        //Update nav
        for(unsigned i = 0; i < fldNav.size(); i++)
            fldNav[i].update(p);

        //Draw folder menu
        folderMenu.draw(mnuTxt);

        data::curData.icon.draw(48, 88);
        drawText(folderMenuInfo.c_str(), frameBuffer, ui::shared, 32, 360, 18, ui::mnuTxt);

        if(down & KEY_A || fldNav[0].getEvent() == BUTTON_RELEASED || folderMenu.getTouchEvent() == MENU_DOUBLE_REL)
        {
            if(folderMenu.getSelected() == 0)
            {
                std::string folder;
                //Add back 3DS shortcut thing
                if(held & KEY_R || held & KEY_L)
                    folder = data::curUser.getUsernameSafe() + " - " + util::getDateTime();
                else
                {
                    ui::keyboard key;
                    folder = util::safeString(key.getString(""));
                }
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
                ui::showMessage("Writing data to system save data is not allowed currently. It CAN brick your system.");
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

                folderMenuPrepare(data::curUser, data::curData);
            }
        }
        else if(down & KEY_MINUS)
        {
            advModePrep();
            mstate = ADV_MDE;
        }
        else if(down & KEY_B || fldNav[3].getEvent() == BUTTON_RELEASED)
        {
            fsdevUnmountDevice("sv");
            mstate = TTL_SEL;
        }
    }
}
