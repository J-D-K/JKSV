#include <fstream>
#include <filesystem>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <switch.h>
#include "filesystem/filesystem.hpp"
#include "system/task.hpp"
#include "system/taskArgs.hpp"
#include "ui/ui.hpp"
#include "stringUtil.hpp"
#include "log.hpp"

namespace
{
    // String names needed from UI to update thread status
    const std::string FILE_COPYING_STRING = "threadStatusCopyingFile";
}

// Read thread function
void fs::io::readThreadFunction(const std::string &source, std::shared_ptr<threadStruct> sharedStruct)
{
    // Bytes read
    uint64_t bytesRead = 0;
    // File stream
    std::ifstream sourceFile(source, std::ios::binary);
    // Local buffer for reading
    std::vector<char> localBuffer(fs::io::FILE_BUFFER_SIZE);

    // Loop until file is fully read
    while(bytesRead < sharedStruct->fileSize)
    {
        // Read to localBuffer
        sourceFile.read(localBuffer.data(), fs::io::FILE_BUFFER_SIZE);

        // Wait for shared buffer to be emptied by write thread
        std::unique_lock sharedBufferLock(sharedStruct->bufferLock);
        sharedStruct->bufferIsReady.wait(sharedBufferLock, [sharedStruct] { return sharedStruct->bufferIsFull == false; });

        // Copy localBuffer to shared buffer
        sharedStruct->buffer.assign(localBuffer.begin(), localBuffer.begin() + sourceFile.gcount());

        // Set condition so write thread can do its thing
        sharedStruct->bufferIsFull = true;

        // Unlock mutex
        sharedBufferLock.unlock();

        // Notify
        sharedStruct->bufferIsReady.notify_one();

        // Update bytes read
        bytesRead += sourceFile.gcount();
    }
}

// File writing thread function
void fs::io::writeThreadFunction(const std::string &destination, std::shared_ptr<threadStruct> sharedStruct, sys::progressTask *task)
{
    // Keep track of bytes written
    uint64_t bytesWritten = 0;
    // Keep track of commits if needed
    uint64_t journalCount = 0;
    // File stream
    std::ofstream destinationFile(destination, std::ios::binary);
    // Local buffer
    std::vector<char> localBuffer;

    // Keep looping until entire file is done
    while(bytesWritten < sharedStruct->fileSize)
    {
        // Wait for buffer to be full
        std::unique_lock sharedBufferLock(sharedStruct->bufferLock);
        sharedStruct->bufferIsReady.wait(sharedBufferLock, [sharedStruct] { return sharedStruct->bufferIsFull == true; });

        // Copy shared buffer to localBuffer so read thread can continue
        localBuffer.assign(sharedStruct->buffer.begin(), sharedStruct->buffer.end());

        // Update offet
        sharedStruct->currentOffset += localBuffer.size();

        // Signal
        sharedStruct->bufferIsFull = false;

        // Unlock
        sharedBufferLock.unlock();

        // Notify
        sharedStruct->bufferIsReady.notify_one();

        // Check to see if commit is needed first
        if(sharedStruct->commitWrite == true && journalCount + localBuffer.size() >= sharedStruct->journalSize)
        {
            // Close file
            destinationFile.close();
            // Commit to save device
            fs::commitSaveData();
            // Reopen destination
            destinationFile.open(destination, std::ios::binary);
            // Seek to end
            destinationFile.seekp(std::ios::end);
            // Reset journal counting
            journalCount = 0;
        }
        // Write
        destinationFile.write(localBuffer.data(), localBuffer.size());
        // Update bytesWritten and journalCount
        bytesWritten += localBuffer.size();
        journalCount += localBuffer.size();
        // Update progress if passed
        if(task != nullptr)
        {
            task->updateProgress(task->getProgress() + localBuffer.size());
        }
    }

    // Close destination first
    destinationFile.close();

    // One last commit just in case
    if(sharedStruct->commitWrite)
    {
        fs::commitSaveData();
    }
}

int fs::io::getFileSize(const std::string &filePath)
{
    // Open file for reading to prevent problems
    std::ifstream file(filePath, std::ios::binary);
    // Seek to end
    file.seekg(0, std::ios::end);
    // Return offset. ifstream destructor should take care of closing
    return file.tellg();
}

void fs::io::copyFile(const std::string &source, const std::string &destination, sys::progressTask *task)
{
    // Shared pointer for threads.
    std::shared_ptr<threadStruct> sharedStruct = std::make_shared<threadStruct>();
    // Get source file size 
    sharedStruct->fileSize = fs::io::getFileSize(source);

    // Set status if available
    if(task != nullptr)
    {
        // Get status string
        std::string statusCopyingFile = stringUtil::getFormattedString(ui::strings::getCString(FILE_COPYING_STRING, 0), source.c_str());
        // Set task thread status.
        task->setThreadStatus(statusCopyingFile);
        // Reset and set max for progress
        task->reset();
        task->setMax(sharedStruct->fileSize);
    }

    // Read & write thread
    std::thread readThread(fs::io::readThreadFunction, source, sharedStruct);
    std::thread writeThread(fs::io::writeThreadFunction, destination, sharedStruct, task);

    // Wait for finish
    readThread.join();
    writeThread.join();
}

void fs::io::copyFileCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize, sys::progressTask *task)
{
    // Shared pointer for threads
    std::shared_ptr<threadStruct> sharedStruct = std::make_shared<threadStruct>();
    // Source size
    sharedStruct->fileSize = fs::io::getFileSize(source);
    // Commit on write, journal size
    sharedStruct->commitWrite = true;
    sharedStruct->journalSize = journalSize;

    // Set status if passed
    if(task != nullptr)
    {
        // Thread status
        std::string statusCopyingFile = stringUtil::getFormattedString(ui::strings::getCString(FILE_COPYING_STRING, 0), source.c_str());
        task->setThreadStatus(statusCopyingFile);
        // Reset, set max
        task->reset();
        task->setMax(sharedStruct->fileSize);
    }

    // Threads
    std::thread readThread(fs::io::readThreadFunction, source, sharedStruct);
    std::thread writeThread(fs::io::writeThreadFunction, destination, sharedStruct, task);

    // Wait
    readThread.join();
    writeThread.join();
}

void fs::io::copyDirectory(const std::string &source, const std::string &destination, sys::progressTask *task)
{
    // Get source directory listing
    fs::directoryListing list(source);

    // Get count and loop through list
    int listCount = list.getListingCount();
    for(int i = 0; i < listCount; i++)
    {
        if(list.itemAtIsDirectory(i))
        {
            // New source and destination directories
            std::string newSource = source + list.getItemAt(i) + "/";
            std::string newDestination = destination + list.getItemAt(i) + "/"; 

            // Create all of the directories in the path JIC
            std::filesystem::create_directories(newDestination);

            // Recusive copy using new source and destination
            fs::io::copyDirectory(newSource, newDestination, task);
        }
        else
        {
            // Full source and destination directories
            std::string fullSource = source + list.getItemAt(i);
            std::string fullDestination = destination + list.getItemAt(i);

            // Copy file
            fs::io::copyFile(fullSource, fullDestination, task);
        }
    }
}

void fs::io::copyDirectoryCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize, sys::progressTask *task)
{
    // Source listing
    fs::directoryListing list(source);

    // Loop through it
    int listCount = list.getListingCount();
    for(int i = 0; i < listCount; i++)
    {
        if(list.itemAtIsDirectory(i))
        {
            // New source and destination with item appended to end with trailing slash
            std::string newSource = source + list.getItemAt(i) + "/";
            std::string newDestination = destination + list.getItemAt(i) + "/";

            // Create new directory in destination
            std::filesystem::create_directory(newDestination.substr(0, newDestination.npos - 1));

            // Recursive 
            fs::io::copyDirectoryCommit(newSource, newDestination, journalSize, task);
        }
        else
        {
            // Full path to source and destination files
            std::string fullSource = source + list.getItemAt(i);
            std::string fullDestination = destination + list.getItemAt(i);

            // Copy it and commit it to save
            fs::io::copyFileCommit(fullSource, fullDestination, journalSize, task);
        }
    }
}

void fs::io::deleteDirectoryRecursively(const std::string &directoryPath)
{
    // Error code for remove
    std::error_code errorCode;

    // Get directory listing
    fs::directoryListing listing(directoryPath);

    // Count for looping
    int listingSize = listing.getListingCount();
    for(int i = 0; i < listingSize; i++)
    {
        if(listing.itemAtIsDirectory(i))
        {
            // New target directory
            std::string newDirectoryPath = directoryPath + listing.getItemAt(i) + "/";
            // Recursion
            fs::io::deleteDirectoryRecursively(newDirectoryPath);
        }
        else
        {
            // Full target path
            std::string targetFile = directoryPath + listing.getItemAt(i);
            // Delete it
            std::filesystem::remove(targetFile, errorCode);
        }
    }
    // Delete base directory if possible.
    std::filesystem::remove(directoryPath, errorCode);
}