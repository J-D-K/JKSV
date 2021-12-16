#pragma once

#include "../gd.h"

#define JKSV_DRIVE_FOLDER "JKSV"

namespace fs
{
    extern drive::gd *gDrive;
    extern std::string jksvDriveID;

    void driveInit();
    void driveExit();
    std::string driveSignInGetAuthCode();
}