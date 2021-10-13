#pragma once

#include <string>
#include <minizip/zip.h>
#include <minizip/unzip.h>

#include "type.h"

namespace fs
{
    //threadInfo is optional and only used when threaded versions are used
    void copyDirToZip(const std::string& src, zipFile dst, bool trimPath, int trimPlaces, threadInfo *t);
    void copyDirToZipThreaded(const std::string& src, zipFile dst, bool trimPath, int trimPlaces);
    void copyZipToDir(unzFile src, const std::string& dst, const std::string& dev, threadInfo *t);
    void copyZipToDirThreaded(unzFile src, const std::string& dst, const std::string& dev);
    uint64_t getZipTotalSize(unzFile unz);
    bool zipNotEmpty(unzFile unz);
}
