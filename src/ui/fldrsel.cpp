#include <string>
#include <vector>
#include <sys/stat.h>

#include "ui.h"
#include "uiupdate.h"
#include "util.h"

ui::menu folderMenu;

void ui::folderMenuPrepare(data::user& usr, data::titledata& dat)
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

void ui::createNewBackup(const uint64_t& held)
{
    std::string folder;

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
            util::getDateTime(util::DATE_FMT_YMD),
            util::getDateTime(util::DATE_FMT_YDM),
            util::getDateTime(util::DATE_FMT_HOYSTE),
            util::getDateTime(util::DATE_FMT_JHK),
            util::getDateTime(util::DATE_FMT_ASC),
            data::curUser.getUsernameSafe(),
            data::curData.getTitleSafe(),
            util::generateAbbrev(data::curData)
        };
        folder = util::getStringInput("", "Enter a folder name", 64, 8, dict);
    }

    if(!folder.empty())
    {
        std::string path = data::curData.getPath() + "/" + folder;
        mkdir(path.c_str(), 777);
        path += "/";
        fs::copyDirToDir("sv:/", path);

        folderMenuPrepare(data::curUser, data::curData);
    }
}

void ui::overwriteBackup(unsigned ind)
{
    fs::dirList list(data::curData.getPath());

    std::string folderName = list.getItem(ind);
    if(confirm(data::holdOver, ui::confOverwrite.c_str(), folderName.c_str()))
    {
        std::string toPath = data::curData.getPath() + folderName + "/";
        //Delete and recreate
        fs::delDir(toPath);
        mkdir(toPath.c_str(), 777);

        std::string root = "sv:/";

        fs::copyDirToDir(root, toPath);
    }
}

void ui::restoreBackup(unsigned ind)
{
    if((data::curData.getType() != FsSaveDataType_System || data::sysSaveWrite) && folderMenu.getSelected() > 0)
    {
        fs::dirList list(data::curData.getPath());

        std::string folderName = list.getItem(ind);
        if(confirm(data::holdRest, ui::confRestore.c_str(), folderName.c_str()))
        {
            if(data::autoBack)
            {
                std::string autoFolder = data::curData.getPath() + "/AUTO - " + data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_ASC);
                mkdir(autoFolder.c_str(), 777);
                autoFolder += "/";

                std::string root = "sv:/";
                fs::copyDirToDir(root, autoFolder);
            }

            std::string fromPath = data::curData.getPath() + folderName + "/";
            std::string root = "sv:/";

            fs::delDir(root);
            fsdevCommitDevice("sv");

            fs::copyDirToDirCommit(fromPath, root, "sv");

            //Rescan init folder menu if autobak to show changes
            if(data::autoBack)
                folderMenuPrepare(data::curUser, data::curData);
        }
    }
}

void ui::deleteBackup(unsigned ind)
{
    fs::dirList list(data::curData.getPath());

    std::string folderName = list.getItem(folderMenu.getSelected() - 1);
    if(ui::confirmDelete(folderName))
    {
        std::string delPath = data::curData.getPath() + folderName + "/";
        fs::delDir(delPath);
    }

    folderMenuPrepare(data::curUser, data::curData);
}

void ui::updateFolderMenu(const uint64_t& down, const uint64_t& held)
{
    folderMenu.handleInput(down, held);

    //Draw folder menu
    folderMenu.draw(ui::txtCont);

    data::curData.icon.draw(96, 98);
    drawTextWrap(folderMenuInfo.c_str(), frameBuffer, ui::shared, 60, 370, 16, ui::txtCont, 360);

    if(down & KEY_A)
    {
        if(folderMenu.getSelected() == 0)
            ui::createNewBackup(held);
        else
            ui::overwriteBackup(folderMenu.getSelected() - 1);
    }
    else if(down & KEY_Y)
        ui::restoreBackup(folderMenu.getSelected() - 1);
    else if(down & KEY_X && folderMenu.getSelected() > 0)
        ui::deleteBackup(folderMenu.getSelected() - 1);
    else if(down & KEY_MINUS)
    {
        advModePrep("sv:/", data::curData.getType(), true);
        mstate = ADV_MDE;
    }
    else if(down & KEY_ZR && data::curData.getType() != FsSaveDataType_System && confirm(true, ui::confEraseFolder.c_str(), data::curData.getTitle().c_str()))
    {
        fs::delDir("sv:/");
        fsdevCommitDevice("sv");
    }
    else if(down & KEY_B)
    {
        fs::unmountSave();
        mstate = TTL_SEL;
    }
}

