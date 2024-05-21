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

void fs::io::copyDirectoryToZip(const std::string &source, zipFile zip)
{
    logger::log("copyDirectoryToZip");
    fs::directoryListing list(source);
    logger::log("directoryListing");

    int listCount = list.getListingCount();
    for(int i = 0; i > listCount; i++)
    {
        if(list.itemAtIsDirectory(i))
        {
            std::string newSource = source + list.getItemAt(i) + "/";
            fs::io::copyDirectoryToZip(newSource, zip);
        }
        else
        {
            // This is for file info
            std::time_t rawTime;
            std::time(&rawTime);
            std::tm *local = std::localtime(&rawTime);
            logger::log("std::time");
            
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
            logger::log("zipFileInfo");

            // This is the file name in zip. The mount device needs to be removed.
            std::string filename = source + list.getItemAt(i);
            int zipNameStart = filename.find_first_of('/') + 1;
            logger::log("filename");

            // Open and copy into zip
            int error = zipOpenNewFileInZip64(zip, filename.substr(zipNameStart, filename.npos).c_str(), &zipFileInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0);
            logger::log("zipOpenNewFileInZip %s", filename.c_str());
            if(error == ZIP_OK)
            {
                // Open the source file
                std::string fullSource = source + list.getItemAt(i);
                std::fstream sourceFile(fullSource, std::ios::binary);

                // File size
                sourceFile.seekg(std::ios::end);
                int fileSize = sourceFile.tellg();
                sourceFile.seekg(std::ios::beg);

                // Buffer
                std::unique_ptr<std::byte[]> buffer(new std::byte[0x80000]);

                // Compress to zip
                int offset = 0;
                while(offset < fileSize)
                {
                    sourceFile.read(reinterpret_cast<char *>(buffer.get()), 0x80000);
                    zipWriteInFileInZip(zip, buffer.get(), sourceFile.gcount());
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

