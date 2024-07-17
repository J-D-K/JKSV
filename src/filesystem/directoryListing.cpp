#include <filesystem>
#include "filesystem/filesystem.hpp"
#include "stringUtil.hpp"

#include "log.hpp"

fs::directoryListing::directoryListing(const std::string &directoryPath) : 
m_DirectoryPath(directoryPath)
{
    loadListing();
}

void fs::directoryListing::loadListing(void)
{
    // Clear vector first JIC reload
    m_DirectoryList.clear();

    // Error code
    std::error_code errorCode;

    for(const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(m_DirectoryPath, errorCode))
    {
        m_DirectoryList.push_back(entry);
    }
}

int fs::directoryListing::getListingCount(void) const
{
    return m_DirectoryList.size();
}

bool fs::directoryListing::itemAtIsDirectory(int index) const
{
    return m_DirectoryList.at(index).is_directory();
}

std::string fs::directoryListing::getFullPathToItemAt(int index) const
{
    return m_DirectoryList.at(index).path().string();
}

std::string fs::directoryListing::getItemAt(int index) const
{
    // Get string first
    std::string itemString = m_DirectoryList.at(index).path().string();
    // Find last slash
    size_t lastSlash = itemString.find_last_of('/') + 1;
    // Return sub string
    return itemString.substr(lastSlash, itemString.npos);
}

std::string fs::directoryListing::getFilenameAt(int index) const
{
    return stringUtil::getFilenameFromString(m_DirectoryList.at(index).path().string());
}

std::string fs::directoryListing::getExtensionAt(int index) const
{
    return stringUtil::getExtensionFromString(m_DirectoryList.at(index).path().string());
}