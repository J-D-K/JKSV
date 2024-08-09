#pragma once
#include <condition_variable>
#include <mutex>
#include <memory>
#include <string>
#include <cstdint>

#include "system/progressTask.hpp"

namespace fs
{
    namespace io
    {
        // Default buffer size used for transfering files
        static const int FILE_BUFFER_SIZE = 0x400000;
        // Struct for threads reading/writing threads. Here because other parts of JKSV can share the writeFunction
        typedef struct
        {
            // File's size
            uint64_t fileSize;
            // Mutex
            std::mutex bufferLock;
            // Conditional for making sure buffer is empty/full
            std::condition_variable bufferIsReady;
            // Bool to check
            bool bufferIsFull = false;
            // Buffer shared for transfer
            std::vector<char> buffer;
            // Whether file needs to be commited on write
            bool commitWrite = false;
            // Journal size
            uint64_t journalSize = 0;
            // Current offset
            uint64_t currentOffset = 0;
        } threadStruct;

        // These are functions for threaded copying that other parts of JKSV can use.
        void readThreadFunction(const std::string &source, std::shared_ptr<threadStruct> sharedStruct);
        void writeThreadFunction(const std::string &destination, std::shared_ptr<threadStruct> sharedStruct, sys::progressTask *task);
        // Just returns size of file
        int getFileSize(const std::string &filePath);
        // These functions can be passed a sys::progressArgs struct to have task status and progress updated. If it's not needed, use nullptr
        // Copies source to destination.
        void copyFile(const std::string &source, const std::string &destination, sys::progressTask *task);
        // Copies source to destination, but commits to save device after journalSize bytes have been written
        void copyFileCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize, sys::progressTask *task);
        // Recusively copies source to destination
        void copyDirectory(const std::string &source, const std::string &destination, sys::progressTask *task);
        // Recursively copies source to destination
        void copyDirectoryCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize, sys::progressTask *task);
        // Deletes directory recursively. std::remove_all was causing too many issues on Switch for me.
        void deleteDirectoryRecursively(const std::string &directoryPath);
    }
}