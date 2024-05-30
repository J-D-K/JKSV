#include <array>
#include <fstream>
#include <filesystem>
#include <memory>
#include <ctime>
#include <cstddef>
#include <switch.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>
#include "filesystem/filesystem.hpp"
#include "log.hpp"

#define ZIP_BUFFER_SIZE 0x80000

void fs::io::zip::copyDirectoryToZip(const std::string &source, zipFile zip)
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
            fs::io::zip::copyDirectoryToZip(newSource, zip);
        }
        else
        {
            // This is for file info
            std::time_t rawTime;
            std::time(&rawTime);
            std::tm *local = std::localtime(&rawTime);
            
            // This is needed to prevent unzipping warnings and errors
            zip_fileinfo zipFileInfo = 
            {
                .tmz_date = 
                {
                   .tm_sec = local->tm_sec,
                   .tm_min = local->tm_min,
                   .tm_hour = local->tm_hour,
                   .tm_mday = local->tm_mday,
                   .tm_mon = local->tm_mon,
                   .tm_year = local->tm_year + 1900
                },
                .dosDate = 0,
                .internal_fa = 0,
                .external_fa = 0
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
                std::ifstream sourceFile(fullSource, std::ios::binary);

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

void fs::io::zip::copyZipToDirectory(unzFile unzip, const std::string &destination, const uint64_t &journalSize)
{
    // Buffer to decompress to
    std::vector<char> buffer(ZIP_BUFFER_SIZE);
    // Filename of file being decompressed
    std::array<char, FS_MAX_PATH> currentFileName;
    // This contains CRC etc. I don't use it but it's needed.
    unz_file_info64 unzipFileInfo;
    // Keep track of journaling space commits
    uint64_t journalCount = 0;
    // Number of bytes read/decompressed
    uint32_t bytesRead = 0;
    do
    {
        unzGetCurrentFileInfo64(unzip, &unzipFileInfo, currentFileName.data(), FS_MAX_PATH, NULL, 0, NULL, 0);
        if(unzOpenCurrentFile(unzip) == UNZ_OK)
        {
            // Full path to output
            std::string fullDestination = destination + currentFileName.data();
            // Make sure full path to exists
            std::filesystem::create_directories(fullDestination.substr(0, fullDestination.find_last_of('/') + 1));
            // File stream
            std::ofstream destinationFile(fullDestination, std::ios::binary);

            while((bytesRead = unzReadCurrentFile(unzip, buffer.data(), ZIP_BUFFER_SIZE)) > 0)
            {
                if(journalCount + bytesRead >= journalSize)
                {
                    // 0 out journal
                    journalCount = 0;
                    // Close file
                    destinationFile.close();
                    // Commit data
                    fs::commitSaveData();
                    // Reopen
                    destinationFile.open(fullDestination, std::ios::binary);
                    // Seek to end
                    destinationFile.seekp(std::ios::end);
                }
                // Write
                destinationFile.write(buffer.data(), bytesRead);
                // Update journal
                journalCount += bytesRead;
            }
        }
    } while(unzGoToNextFile(unzip) != UNZ_END_OF_LIST_OF_FILE);
    fs::commitSaveData();
}

uint64_t fs::io::zip::getZipTotalFileSize(unzFile unzip)
{
    // Total size to return
    uint64_t totalSize = 0;
    if(unzGoToFirstFile(unzip) == UNZ_OK)
    {
        // Needed to get size
        unz_file_info64 fileInfo = { 0 };
        std::array<char, FS_MAX_PATH> filename;

        // Loop through zip file
        do
        {
            // Get info
            unzGetCurrentFileInfo64(unzip, &fileInfo, filename.data(), FS_MAX_PATH, NULL, 0, NULL, 0);
            // Add size to total size
            totalSize += fileInfo.uncompressed_size;
        } while (unzGoToNextFile(unzip) != UNZ_END_OF_LIST_OF_FILE);
        // Reset to beginning of zip
        unzGoToFirstFile(unzip);
    }
    return totalSize;
}

bool fs::io::zip::zipFileIsNotEmpty(unzFile unzip)
{
    // Just return if we can get first file
    return unzGoToFirstFile(unzip) == UNZ_OK;
}