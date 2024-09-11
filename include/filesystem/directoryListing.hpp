#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace fs
{
    class directoryListing
    {
        public:
            directoryListing(void) = default;
            // Gets and stores a listing for directoryPath
            directoryListing(const std::string &directoryPath);
            // This is the function that actually loads the directory list. Vector is cleared at beginning.
            void loadListing(void);
            // Returns size of listing vector
            int getListingCount(void) const;
            // Returns if item at index is a folder
            bool itemAtIsDirectory(int index) const;
            // Returns full path of item at index
            std::string getFullPathToItemAt(int index) const;
            // Returns full name + extension of index
            std::string getItemAt(int index) const;
            // Returns just the file name (no extension)
            std::string getFilenameAt(int index) const;
            // Returns extension of index
            std::string getExtensionAt(int index) const;

        private:
            // Stores the path to the directory passed
            std::filesystem::path m_DirectoryPath;
            // Stores the names of items from list
            std::vector<std::filesystem::directory_entry> m_DirectoryList;
    };
}