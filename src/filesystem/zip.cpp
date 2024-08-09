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
#include "ui/ui.hpp"
#include "stringUtil.hpp"
#include "log.hpp"

namespace
{
    // Buffer size for compressing/decompressing
    const int ZIP_BUFFER_SIZE = 0x80000;
    // String names needed from UI
    const std::string STRING_ADDING_TO_ZIP = "threadStatusAddingFileToZip";
    const std::string STRING_DECOMPRESSING_FROM_ZIP = "threadStatusDecompressingFile";
}

void fs::zip::copyDirectoryToZip(const std::string &source, zipFile zip, sys::progressTask *task)
{
    // Source file listing
    fs::directoryListing listing(source);

    // Total size to loop
    int listCount = listing.getListingCount();
    for(int i = 0; i < listCount; i++)
    {
        if(listing.itemAtIsDirectory(i))
        {
            // New source
            std::string newSource = source + listing.getItemAt(i) + "/";
            // Recursive zipping
            fs::zip::copyDirectoryToZip(newSource, zip, task);
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
                // Get file size quick
                int fileSize = fs::io::getFileSize(filename);

                // Open the source file for reading
                std::ifstream sourceFile(filename, std::ios::binary);

                if(task != nullptr)
                {
                    // Set thread status
                    std::string zipStatusString = stringUtil::getFormattedString(ui::strings::getCString(STRING_ADDING_TO_ZIP, 0), filename.c_str());
                    task->setThreadStatus(zipStatusString);
                    // Reset and set max
                    task->reset();
                    task->setMax(fileSize);
                }
                
                // Buffer
                std::vector<char> buffer(ZIP_BUFFER_SIZE);

                // Compress to zip
                int offset = 0;
                while(offset < fileSize)
                {
                    sourceFile.read(buffer.data(), ZIP_BUFFER_SIZE);
                    zipWriteInFileInZip(zip, buffer.data(), sourceFile.gcount());
                    offset += sourceFile.gcount();

                    if(task != nullptr)
                    {
                        task->updateProgress(offset);
                    }
                }
            }
            else
            {
                logger::log("Error creating file in zip: %s.", filename);
            }
        }
    }
}

void fs::zip::copyZipToDirectory(unzFile unzip, const std::string &destination, uint64_t journalSize, sys::progressTask *task)
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

            // Update status
            if(task != nullptr)
            {
                std::string decompressingStatusString = stringUtil::getFormattedString(ui::strings::getCString(STRING_DECOMPRESSING_FROM_ZIP, 0), currentFileName.data());
                task->setThreadStatus(decompressingStatusString);

                // Progress 
                task->reset();
                task->setMax(unzipFileInfo.uncompressed_size);
            }

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

                if(task != nullptr)
                {
                    // Only kind of way to do this with unzip
                    task->updateProgress(task->getProgress() + bytesRead);
                }
            }
        }
    } while(unzGoToNextFile(unzip) != UNZ_END_OF_LIST_OF_FILE);
    fs::commitSaveData();
}

uint64_t fs::zip::getZipTotalFileSize(unzFile unzip)
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

bool fs::zip::zipFileIsNotEmpty(unzFile unzip)
{
    // Just return if we can get first file
    return unzGoToFirstFile(unzip) == UNZ_OK;
}