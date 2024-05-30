#pragma once
#include <string>
#include <cstdint>
#include <minizip/zip.h>
#include <minizip/unzip.h>


namespace fs
{
    namespace io
    {
        namespace zip
        {
            // Copies and compresses source directory to zipFile passed
            void copyDirectoryToZip(const std::string &source, zipFile zip);
            // This is only used to write to save filesystems, so it requires journal size
            void copyZipToDirectory(unzFile unzip, const std::string &destination, const uint64_t &journalSize);
            // Returns the total uncompressed sizes of files in zip
            uint64_t getZipTotalFileSize(unzFile unzip);
            // Returns if it's possible to get to a first file in unz
            bool zipFileIsNotEmpty(unzFile unzip);
        }
    }
}