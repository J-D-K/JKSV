#pragma once

#include "../gd.h"

namespace fs
{
    extern drive::gd *gDrive;
    void driveInit();
    void driveExit();
}