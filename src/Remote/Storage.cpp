#include "Remote/Storage.hpp"
#include <algorithm>

namespace
{
    // This is the user agent JKSV uses.
    const char *USER_AGENT_STRING = "JKSV";
    // This is the buffer size for uploading.
    constexpr unsigned int UPLOAD_BUFFER_SIZE = 0x40000;
} // namespace


Remote::Storage::Storage(void)
{
    m_CurlHandle = curl_easy_init();
}

Remote::Storage::~Storage()
{
    curl_easy_cleanup(m_CurlHandle);
}

bool Remote::Storage::IsInitialized(void) const
{
    return m_IsInitialized;
}

bool Remote::Storage::DirectoryExists(std::string_view DirectoryName, std::string_view ParentID)
{
    auto FindDir = Storage::FindDirectory(DirectoryName, ParentID);

    if (FindDir == m_RemoteList.end())
    {
        return false;
    }
    return true;
}

bool Remote::Storage::FileExists(std::string_view FileName, std::string_view ParentID)
{
    auto FindFile = Storage::FindFile(FileName, ParentID);

    if (FindFile == m_RemoteList.end())
    {
        return false;
    }
    return true;
}

bool Remote::Storage::GetDirectoryID(std::string_view DirectoryName, std::string_view ParentID, std::string &IDOut)
{
    auto FindDir = Storage::FindDirectory(DirectoryName, ParentID);

    if (FindDir == m_RemoteList.end())
    {
        return false;
    }
    IDOut = FindDir->GetItemID();
    return true;
}

bool Remote::Storage::GetFileID(std::string_view FileName, std::string_view ParentID, std::string &IDOut)
{
    auto FindFile = Storage::FindFile(FileName, ParentID);

    if (FindFile == m_RemoteList.end())
    {
        return false;
    }
    IDOut = FindFile->GetItemID();
    return true;
}

auto Remote::Storage::FindDirectory(std::string_view DirectoryName, std::string_view ParentID)
{
    return std::find_if(m_RemoteList.begin(), m_RemoteList.end(), [DirectoryName, ParentID](const Remote::StorageItem &Item) {
        return Item.IsDirectory() && Item.GetParentID() == ParentID && Item.GetItemName() == DirectoryName;
    });
}

auto Remote::Storage::FindFile(std::string_view FileName, std::string_view ParentID)
{
    return std::find_if(m_RemoteList.begin(), m_RemoteList.end(), [FileName, ParentID](const Remote::StorageItem &Item) {
        return !Item.IsDirectory() && Item.GetParentID() == ParentID && Item.GetItemName() == FileName;
    });
}

void Remote::Storage::CurlPreparePost(void)
{
    curl_easy_reset(m_CurlHandle);
    curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPPOST, 1);
    curl_easy_setopt(m_CurlHandle, CURLOPT_USERAGENT, USER_AGENT_STRING);
}

void Remote::Storage::CurlPrepareGet(void)
{
    curl_easy_reset(m_CurlHandle);
    curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(m_CurlHandle, CURLOPT_USERAGENT, USER_AGENT_STRING);
}

void Remote::Storage::CurlPrepareUpload(void)
{
    curl_easy_reset(m_CurlHandle);
    curl_easy_setopt(m_CurlHandle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(m_CurlHandle, CURLOPT_USERAGENT, USER_AGENT_STRING);
    curl_easy_setopt(m_CurlHandle, CURLOPT_UPLOAD_BUFFERSIZE, UPLOAD_BUFFER_SIZE);
}

void Remote::Storage::CurlPreparePatch(void)
{
    curl_easy_reset(m_CurlHandle);
    curl_easy_setopt(m_CurlHandle, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(m_CurlHandle, CURLOPT_USERAGENT, USER_AGENT_STRING);
}

void Remote::Storage::CurlPrepareDelete(void)
{
    curl_easy_recv(m_CurlHandle);
    curl_easy_setopt(m_CurlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(m_CurlHandle, CURLOPT_USERAGENT, USER_AGENT_STRING);
}

bool Remote::Storage::CurlPerform(void)
{
    return curl_easy_perform(m_CurlHandle) == CURLE_OK;
}
