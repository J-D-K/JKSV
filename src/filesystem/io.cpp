#include <switch.h>
#include <fstream>
#include <filesystem>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <vector>
#include "filesystem/filesystem.hpp"
#include "system/task.hpp"
#include "system/taskArgs.hpp"
#include "log.hpp"

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
void fs::io::writeThreadFunction(const std::string &destination, std::shared_ptr<threadStruct> sharedStruct)
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
    }
    // One last commit just in case
    fs::commitSaveData();
}

int fs::io::getFileSize(const std::string &filePath)
{
    // Open file for reading to prevent problems
    std::ifstream file(filePath, std::ios::binary);
    // Seek to end
    file.seekg(std::ios::end);
    // Return offset. ifstream destructor should take care of closing
    return file.tellg();
}

void fs::io::copyFile(const std::string &source, const std::string &destination)
{
    // Create shared thread struct
    std::shared_ptr<threadStruct> sharedStruct = std::make_shared<threadStruct>();

    // Everything else is set by default
    sharedStruct->fileSize = fs::io::getFileSize(source);

    // Read & write thread
    std::thread readThread(fs::io::readThreadFunction, source, sharedStruct);
    std::thread writeThread(fs::io::writeThreadFunction, destination, sharedStruct);

    // Wait for finish
    readThread.join();
    writeThread.join();
}

void fs::io::copyFileCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize)
{
    // Shared struct
    std::shared_ptr<threadStruct> sharedStruct = std::make_shared<threadStruct>();

    // Set vars
    sharedStruct->fileSize = fs::io::getFileSize(source);
    sharedStruct->commitWrite = true;
    sharedStruct->journalSize = journalSize;

    // Threads
    std::thread readThread(fs::io::readThreadFunction, source, sharedStruct);
    std::thread writeThread(fs::io::writeThreadFunction, destination, sharedStruct);

    // Wait
    readThread.join();
    writeThread.join();
}

void fs::io::copyDirectory(const std::string &source, const std::string &destination)
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
            fs::io::copyDirectory(newSource, newDestination);
        }
        else
        {
            // Full source and destination directories
            std::string fullSource = source + list.getItemAt(i);
            std::string fullDestination = destination + list.getItemAt(i);

            // Copy file
            fs::io::copyFile(fullSource, fullDestination);
        }
    }
}

void fs::io::copyDirectoryCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize)
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
            fs::io::copyDirectoryCommit(newSource, newDestination, journalSize);
        }
        else
        {
            // Full path to source and destination files
            std::string fullSource = source + list.getItemAt(i);
            std::string fullDestination = destination + list.getItemAt(i);

            // Copy it and commit it to save
            fs::io::copyFileCommit(fullSource, fullDestination, journalSize);
        }
    }
}

void fs::io::deleteDirectoryRecursively(const std::string &target)
{
    // Get directory listing
    fs::directoryListing listing(target);
    // Get item count
    int listingCount = listing.getListingCount();
    // Error code to stop from throwing exceptions on failure to delete.
    std::error_code errorCode;

    // Loop through
    for(int i = 0; i < listingCount; i++)
    {
        if(listing.itemAtIsDirectory(i))
        {
            // New target path
            std::string newTarget = listing.getFullPathToItemAt(i) + "/";
            // Call self to continue
            fs::io::deleteDirectoryRecursively(newTarget);
            // Delete directory
            std::filesystem::remove(listing.getFullPathToItemAt(i), errorCode);
        }
        else
        {
            // Just delete file
            std::filesystem::remove(listing.getFullPathToItemAt(i), errorCode);
        }
    }
    // Finally delete last directory
    std::filesystem::remove(target, errorCode);
}