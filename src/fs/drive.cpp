#include "drive.h"
#include "cfg.h"
#include "ui.h"

drive::gd *fs::gDrive = NULL;

void fs::driveInit()
{
    if(!cfg::driveClientID.empty() && !cfg::driveClientSecret.empty())
    {
        fs::gDrive = new drive::gd(cfg::driveClientID, cfg::driveClientSecret, cfg::driveAuthCode, cfg::driveRefreshToken);
        if(!fs::gDrive->hasToken())
        {
            delete fs::gDrive;
            fs::gDrive = NULL;
            ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popDriveFailed", 0));
        }
        else
        {
            fs::gDrive->loadDriveList("name = 'JKSV'");

            if(!fs::gDrive->dirExists("JKSV"))
                fs::gDrive->createDir("JKSV");

            std::string jksvID = fs::gDrive->getFileID("JKSV");
            fs::gDrive->setRootDir(jksvID);

            ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popDriveStarted", 0));
        }
    }
}

void fs::driveExit()
{
    if(fs::gDrive)
    {
        //Need to save for config if first run
        cfg::driveRefreshToken = fs::gDrive->getRefreshToken();
        delete gDrive;
    }
}