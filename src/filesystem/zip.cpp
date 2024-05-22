#include <fstream>
#include <memory>
#include <ctime>
#include <cstddef>
#include <switch.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>
#include "filesystem/directoryListing.hpp"
#include "filesystem/zip.hpp"
#include "log.hpp"

#define ZIP_BUFFER_SIZE 0x80000

void fs::io::copyDirectoryToZip(const std::string &source, zipFile zip)
{
    // Source file listing
    fs::directoryListing listing(source);

    // Total size to loop
    int listCount = listing.getListingCount();
    for(int i = 0; i < listCount; i++)
    {
        if(listing.itemAtIsDirectory(i))
        {
            std::string newSource = source + listing.getItemAt(i) + "/";
            fs::io::copyDirectoryToZip(newSource, zip);
        }
        else
        {
            // This is for file info
            std::time_t rawTime;
            std::time(&rawTime);
            std::tm *local = std::localtime(&rawTime);
            
            // This is needed to prevent unzipping warnings and errors
            zip_fileinfo zipFileInfo;
            zipFileInfo.tmz_date = 
            {
                .tm_sec = local->tm_sec,
                .tm_min = local->tm_min,
                .tm_hour = local->tm_hour,
                .tm_mday = local->tm_mday,
                .tm_mon = local->tm_mon,
                .tm_year = local->tm_year + 1900
            };

            // This is the file name in zip. The mount device needs to be removed.
            std::string filename = source + listing.getItemAt(i);
            int zipNameStart = filename.find_first_of('/') + 1;

            // Open and copy into zip
            int error = zipOpenNewFileInZip64(zip, filename.substr(zipNameStart, filename.npos).c_str(), &zipFileInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0);
            if(error == ZIP_OK)
            {
                // Open the source file
                std::string fullSource = source + listing.getItemAt(i);
                std::fstream sourceFile(fullSource, std::ios::binary);

                // File size
                sourceFile.seekg(std::ios::end);
                int fileSize = sourceFile.tellg();
                sourceFile.seekg(std::ios::beg);

                // Buffer
                std::vector<char> buffer(ZIP_BUFFER_SIZE);

                // Compress to zip
                int offset = 0;
                while(offset < fileSize)
                {
                    sourceFile.read(buffer.data(), ZIP_BUFFER_SIZE);
                    zipWriteInFileInZip(zip, buffer.data(), sourceFile.gcount());
                    offset += sourceFile.gcount();
                }
            }
            else
            {
                logger::log("Error creating file in zip: %s.", filename);
            }
        }
    }
}

void fs::io::copyZipToDirectory(unzFile unzip, const std::string &destination, const uint64_t &journalSize)
{
    std::unique_ptr<std::byte[]> buffer(new std::byte[0x80000]);
    char currentFileName[FS_MAX_PATH];
    unz_file_info64 unzipFileInfo;

    do
    {
        unzGetCurrentFileInfo64(unzip, &unzipFileInfo, currentFileName, FS_MAX_PATH, NULL, 0, NULL, 0);
        if(unzOpenCurrentFile(unzip) == UNZ_OK)
        {
            
        }
    } while(unzGoToNextFile(unzip) != UNZ_END_OF_LIST_OF_FILE);
}

