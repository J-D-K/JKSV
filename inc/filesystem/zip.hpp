#pragma once
#include <string>
#include <cstdint>
#include <minizip/zip.h>
#include <minizip/unzip.h>


namespace fs
{
    namespace io
    {
        // Copies and compresses source directory to zipFile passed
        void copyDirectoryToZip(const std::string &source, zipFile zip);
        // This is only used to write to save filesystems, so it requires journal size
        void copyZipToDirectory(unzFile unzip, const std::string &destination, const uint64_t &journalSize);
    }
}