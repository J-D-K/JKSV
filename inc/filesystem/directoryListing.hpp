#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace fs
{
    class directoryListing
    {
        public:
            // Gets and stores a listing for directoryPath
            directoryListing(const std::string &directoryPath);
            // This is the function that actually loads the directory list. Vector is cleared at beginning.
            void loadListing(void);
            // Returns size of listing vector
            int getListingCount(void) const;
            // Returns if item at index is a folder
            bool itemAtIsDirectory(const int &index) const;
            // Returns full path of item at index
            std::string getFullPathToItemAt(const int &index) const;
            // Returns full name + extension of index
            std::string getItemAt(const int &index) const;
            // Returns just the file name (no extension)
            std::string getFilenameAt(const int &index) const;
            // Returns extension of index
            std::string getExtensionAt(const int &index) const;

        private:
            // Stores the path to the directory passed
            std::string m_DirectoryPath;
            // Stores the names of items from list
            std::vector<std::filesystem::directory_entry> m_DirectoryList;
    };
}