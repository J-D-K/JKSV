#pragma once
#include <string>
#include <cstdint>

namespace fs
{
    namespace io
    {
        // Just returns size of file
        int getFileSize(const std::string &filePath);
        // These functions are all threaded/task wrapper functions
        // Copies source to destination.
        void copyFile(const std::string &source, const std::string &destination);
        // Copies source to destination, but commits to save device after journalSize bytes have been written
        void copyFileCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize);
        // Recusively copies source to destination
        void copyDirectory(const std::string &source, const std::string &destination);
        // Recursively copies source to destination 
        void copyDirectoryCommit(const std::string &source, const std::string &destination, const uint64_t &journalSize);
    }
}