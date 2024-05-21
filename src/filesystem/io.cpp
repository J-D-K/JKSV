#include <switch.h>
#include <fstream>
#include <filesystem>
#include <cstddef>
#include "filesystem/filesystem.hpp"
#include "filesystem/directoryListing.hpp"
#include "filesystem/io.hpp"
#include "system/task.hpp"
#include "system/taskArgs.hpp"
#include "log.hpp"

#define FILE_BUFFER_SIZE 0x80000

// Generic error for opening files
static const char *s_ErrorOpening = "Error opening files for reading and writing.";

// This is the struct passed as shared_ptr for task functionss
struct copyArgs : sys::taskArgs
{
    std::string source, destination, device;
    bool commit = false;
};

int fs::io::getFileSize(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    file.seekg(std::ios::end);
    return file.tellg();
}

void fs::io::copyFile(const std::string &source, const std::string &destination)
{
    std::ifstream sourceFile(source, std::ios::binary);
    std::ofstream destinationFile(destination, std::ios::binary);

    if(sourceFile.is_open() == false || destinationFile.is_open() == false)
    {
        logger::log(s_ErrorOpening);
        return;
    }

    // Get file size since it's already opened and we can't use getFileSize
    sourceFile.seekg(std::ios::end);
    int sourceSize = sourceFile.tellg();
    sourceFile.seekg(std::ios::beg);

    // Buffer
    std::unique_ptr<std::byte[]> buffer(new std::byte[FILE_BUFFER_SIZE]);

    int i = 0;
    while(i < sourceSize)
    {
        sourceFile.read(reinterpret_cast<char *>(buffer.get()), FILE_BUFFER_SIZE);
        destinationFile.write(reinterpret_cast<char *>(buffer.get()), sourceFile.gcount());
        i += sourceFile.gcount();
    }
}

void fs::io::copyFileCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize)
{
    std::ifstream sourceFile(source, std::ios::binary);
    std::ofstream destinationFile(destination, std::ios::binary);

    if(sourceFile.is_open() == false || destinationFile.is_open() == false)
    {
        logger::log(s_ErrorOpening);
        return;
    }

    sourceFile.seekg(std::ios::end);
    int sourceSize = sourceFile.tellg();
    sourceFile.seekg(std::ios::beg);

    std::unique_ptr<std::byte[]> buffer(new std::byte[FILE_BUFFER_SIZE]);

    // This is for keeping track of write size because of journaling
    uint64_t writeCount = 0;

    int i = 0;
    while(i < sourceSize)
    {
        sourceFile.read(reinterpret_cast<char *>(buffer.get()), FILE_BUFFER_SIZE);
        destinationFile.write(reinterpret_cast<char *>(buffer.get()), sourceFile.gcount());

        writeCount += sourceFile.gcount();
        i += sourceFile.gcount();

        // Close, commit, reopen
        if(writeCount >= journalSize - 0x200000)
        {
            destinationFile.close();
            fsdevCommitDevice(fs::DEFAULT_SAVE_MOUNT_DEVICE);
            destinationFile.open(destination, std::ios::binary);
            destinationFile.seekp(i, std::ios::beg);
        }
    }
    fsdevCommitDevice(fs::DEFAULT_SAVE_MOUNT_DEVICE);
}

void fs::io::copyDirectory(const std::string &source, const std::string &destination)
{
    fs::directoryListing list(source);

    int listCount = list.getListingCount();
    for(int i = 0; i < listCount; i++)
    {
        if(list.itemAtIsDirectory(i))
        {
            std::string newSource = source + list.getItemAt(i) + "/";
            std::string newDestination = destination + list.getItemAt(i) + "/"; 
            std::filesystem::create_directory(newDestination.substr(0, newDestination.npos - 1));
            fs::io::copyDirectory(newSource, newDestination);
        }
        else
        {
            std::string fullSource = source + list.getItemAt(i);
            std::string fullDestination = destination + list.getItemAt(i);
            fs::io::copyFile(fullSource, fullDestination);
        }
    }
}

void fs::io::copyDirectoryCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize)
{
    fs::directoryListing list(source);

    int listCount = list.getListingCount();
    for(int i = 0; i < listCount; i++)
    {
        if(list.itemAtIsDirectory(i))
        {
            std::string newSource = source + list.getItemAt(i) + "/";
            std::string newDestination = destination + list.getItemAt(i) + "/";
            std::filesystem::create_directory(newDestination.substr(0, newDestination.npos - 1));
            fs::io::copyDirectoryCommit(newSource, newDestination, journalSize);
        }
        else
        {
            std::string fullSource = source + list.getItemAt(i);
            std::string fullDestination = destination + list.getItemAt(i);
            fs::io::copyFileCommit(fullSource, fullDestination, journalSize);
        }
    }
}