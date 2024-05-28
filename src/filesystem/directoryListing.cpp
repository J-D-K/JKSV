#include <filesystem>
#include "filesystem/filesystem.hpp"
#include "stringUtil.hpp"

#include "log.hpp"

fs::directoryListing::directoryListing(const std::string &directoryPath) : m_DirectoryPath(directoryPath)
{
    loadListing();
}

void fs::directoryListing::loadListing(void)
{
    // Clear vector first JIC reload
    m_DirectoryList.clear();

    for(const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator{m_DirectoryPath})
    {
        m_DirectoryList.push_back(entry.path().string());
    }
}

int fs::directoryListing::getListingCount(void) const
{
    return m_DirectoryList.size();
}

bool fs::directoryListing::itemAtIsDirectory(const int &index) const
{
    return std::filesystem::is_directory(m_DirectoryList.at(index));
}

std::string fs::directoryListing::getFullPathToItemAt(const int &index) const
{
    return m_DirectoryList.at(index);
}

std::string fs::directoryListing::getItemAt(const int &index) const
{
    size_t lastSlash = m_DirectoryList.at(index).find_last_of('/') + 1;
    return m_DirectoryList.at(index).substr(lastSlash, m_DirectoryList.at(index).npos);
}

std::string fs::directoryListing::getFilenameAt(const int &index) const
{
    return stringUtil::getFilenameFromString(m_DirectoryList.at(index));
}

std::string fs::directoryListing::getExtensionAt(const int &index) const
{
    return stringUtil::getExtensionFromString(m_DirectoryList.at(index));
}