#pragma once
#include <string>
#include <cstdint>

#include <minizip/zip.h>
#include <minizip/unzip.h>

#include "system/progressTask.hpp"

namespace fs
{
    namespace zip
    {
        // Copies and compresses source directory to zipFile passed
        void copyDirectoryToZip(const std::string &source, zipFile zip, sys::progressTask *task);
        // This is only used to write to save filesystems, so it requires journal size
        void copyZipToDirectory(unzFile unzip, const std::string &destination, uint64_t journalSize, sys::progressTask *task);
        // Returns the total uncompressed sizes of files in zip
        uint64_t getZipTotalFileSize(unzFile unzip);
        // Returns if it's possible to get to a first file in unz
        bool zipFileIsNotEmpty(unzFile unzip);
    }
}