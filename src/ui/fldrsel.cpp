#include <string>
#include <vector>
#include <sys/stat.h>

#include "ui.h"
#include "uiupdate.h"
#include "util.h"

ui::menu folderMenu;
static fs::dirList fldList;

void ui::folderMenuPrepare(data::user& usr, data::titledata& dat)
{
    folderMenu.setParams(466, 98, 730);
    folderMenu.reset();

    dat.createDir();

    fldList.reassign(dat.getPath());
    fs::loadPathFilters(dat.getPath() + "pathFilters.txt");
    folderMenu.addOpt("New");
    for(unsigned i = 0; i < fldList.getCount(); i++)
        folderMenu.addOpt(fldList.getItem(i));

    folderMenu.adjust();
}

void ui::createNewBackup(const uint64_t& held)
{
    std::string out;

    if(held & HidNpadButton_R)
        out = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD);
    else if(held & HidNpadButton_L)
        out = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YDM);
    else if(held & HidNpadButton_ZL)
        out = data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_HOYSTE);
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
        out = util::getStringInput("", "Enter a name", 64, 8, dict);
    }

    if(!out.empty())
    {
        std::string path = data::curData.getPath() + "/" + out;
        switch(data::zip)
        {
            case true:
                {
                    path += ".zip";
                    zipFile zip = zipOpen(path.c_str(), 0);
                    fs::copyDirToZip("sv:/", &zip);
                    zipClose(zip, NULL);
                }
                break;

            case false:
                {
                    mkdir(path.c_str(), 777);
                    path += "/";
                    fs::copyDirToDir("sv:/", path);
                }
                break;
        }
        folderMenuPrepare(data::curUser, data::curData);
    }
}

void ui::overwriteBackup(unsigned ind)
{
    std::string itemName = fldList.getItem(ind);
    if(confirm(data::holdOver, ui::confOverwrite.c_str(), itemName.c_str()))
    {
        if(fldList.isDir(ind))
        {
            std::string toPath = data::curData.getPath() + itemName + "/";
            //Delete and recreate
            fs::delDir(toPath);
            mkdir(toPath.c_str(), 777);
            fs::copyDirToDir("sv:/", toPath);
        }
        else if(!fldList.isDir(ind) && fldList.getItemExt(ind) == "zip")
        {
            std::string toPath = data::curData.getPath() + itemName;
            fs::delfile(toPath);
            zipFile zip = zipOpen(toPath.c_str(), 0);
            fs::copyDirToZip("sv:/", &zip);
            zipClose(zip, NULL);
        }
    }
}

void ui::restoreBackup(unsigned ind)
{
    std::string itemName = fldList.getItem(ind);
    if((data::curData.getType() != FsSaveDataType_System || data::sysSaveWrite) && folderMenu.getSelected() > 0 && confirm(data::holdRest, ui::confRestore.c_str(), itemName.c_str()))
    {
        if(data::autoBack)
        {
            switch(data::zip)
            {
                case true:
                    {
                        std::string autoZip = data::curData.getPath() + "/AUTO " + data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + ".zip";
                        zipFile zip = zipOpen(autoZip.c_str(), 0);
                        fs::copyDirToZip("sv:/", &zip);
                        zipClose(zip, NULL);
                    }
                    break;

                case false:
                    {
                        std::string autoFolder = data::curData.getPath() + "/AUTO - " + data::curUser.getUsernameSafe() + " - " + util::getDateTime(util::DATE_FMT_YMD) + "/";
                        mkdir(autoFolder.substr(0, autoFolder.length() - 1).c_str(), 777);
                        fs::copyDirToDir("sv:/", autoFolder);
                    }
                    break;
            }
        }

        if(fldList.isDir(ind))
        {
            fs::wipeSave();
            std::string fromPath = data::curData.getPath() + itemName + "/";
            fs::copyDirToDirCommit(fromPath, "sv:/", "sv");
        }
        else if(!fldList.isDir(ind) && fldList.getItemExt(ind) == "zip")
        {
            fs::wipeSave();
            std::string path = data::curData.getPath() + itemName;
            unzFile unz = unzOpen(path.c_str());
            fs::copyZipToDir(&unz, "sv:/", "sv");
            unzClose(unz);
        }
        else
        {
            //Just copy file over
            std::string fromPath = data::curData.getPath() + itemName;
            std::string toPath = "sv:/" + itemName;
            fs::copyFileCommit(fromPath, toPath, "sv");
        }
    }
    if(data::autoBack)
        folderMenuPrepare(data::curUser, data::curData);
}

void ui::deleteBackup(unsigned ind)
{
    std::string itemName = fldList.getItem(folderMenu.getSelected() - 1);
    if(ui::confirmDelete(itemName))
    {
        if(fldList.isDir(ind))
        {
            std::string delPath = data::curData.getPath() + itemName + "/";
            fs::delDir(delPath);
        }
        else
        {
            std::string delPath = data::curData.getPath() + itemName;
            fs::delfile(delPath);
        }
    }
    folderMenuPrepare(data::curUser, data::curData);
}

void ui::drawFolderMenu()
{
    data::curData.drawIcon(true, 96, 98);
    drawTextWrap(folderMenuInfo.c_str(), frameBuffer, ui::shared, 60, 370, 16, ui::txtCont, 360);
    folderMenu.draw(ui::txtCont);
}

void ui::updateFolderMenu(const uint64_t& down, const uint64_t& held)
{
    folderMenu.handleInput(down, held);

    switch(down)
    {
        case HidNpadButton_A:
            if(folderMenu.getSelected() == 0)
                ui::createNewBackup(held);
            else
                ui::overwriteBackup(folderMenu.getSelected() - 1);
            break;

        case HidNpadButton_B:
            fs::unmountSave();
            fs::freePathFilters();
            ui::changeState(TTL_SEL);
            break;

        case HidNpadButton_X:
            if(folderMenu.getSelected() > 0)
                ui::deleteBackup(folderMenu.getSelected() - 1);
            break;

        case HidNpadButton_Y:
            if(folderMenu.getSelected() > 0)
                ui::restoreBackup(folderMenu.getSelected() - 1);
            break;

        case HidNpadButton_ZR:
            if(data::curData.getType() != FsSaveDataType_System && confirm(true, ui::confEraseFolder.c_str(), data::curData.getTitle().c_str()))
            {
                fs::delDir("sv:/");
                fsdevCommitDevice("sv");
            }
            break;

        case HidNpadButton_Minus:
            advModePrep("sv:/", data::curData.getType(), true);
            ui::changeState(ADV_MDE);
            break;
    }
}

