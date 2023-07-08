#pragma once

#include "../rfs.h"

#define JKSV_DRIVE_FOLDER "JKSV"

namespace fs
{
    extern rfs::IRemoteFS *rfs;
    extern std::string rfsRootID;

    void remoteInit();
    void remoteExit();

    // Google Drive
    void driveInit();
    std::string driveSignInGetAuthCode();

    // Webdav
    void webDavInit();
}