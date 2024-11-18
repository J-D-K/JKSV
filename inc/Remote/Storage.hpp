#pragma once
#include "Curl/Curl.hpp"
#include "Remote/StorageItem.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Remote
{
    class Storage
    {
        public:
            // This allocates the curl handle.
            Storage(void);
            // This frees the handle.
            virtual ~Storage();
            // This returns if everything initialized properly.
            bool IsInitialized(void) const;
            // If these all shared the same struct, why did they need to be virtual?
            bool DirectoryExists(std::string_view DirectoryName, std::string_view ParentID);
            bool FileExists(std::string_view FileName, std::string_view ParentID);
            bool GetDirectoryID(std::string_view DirectoryName, std::string_view ParentID, std::string &IDOut);
            bool GetFileID(std::string_view FileName, std::string_view ParentID, std::string &IDOut);
            // All derived classes need these implemented so they can be called without casting needed.
            virtual bool CreateDirectory(std::string_view DirectoryName, std::string_view ParentID) = 0;
            virtual bool UploadFile(std::string_view FileName, std::string_view ParentID, std::shared_ptr<Curl::UploadStruct> UploadStruct) = 0;
            virtual bool UpdateFile(std::string_view FileID, std::shared_ptr<Curl::UploadStruct> UploadStruct) = 0;
            virtual bool DownloadFile(std::string_view FileID, std::shared_ptr<Curl::DownloadStruct> DownloadStruct) = 0;
            virtual bool DeleteFile(std::string_view FileID) = 0;

        protected:
            // This is so we know for sure whether init was succeful.
            bool m_IsInitialized = false;
            // Curl handle.
            CURL *m_CurlHandle = NULL;
            // This is the current listing. I tried to figure out a way to use maps instead, but nope.
            std::vector<Remote::StorageItem> m_RemoteList;
            // These functions might not have any use outside of here.
            auto FindDirectory(std::string_view DirectoryName, std::string_view ParentID);
            auto FindFile(std::string_view FileName, std::string_view ParentID);
            // These prepare the curl handle to save myself loads of uneeded typing. I haven't had enough time to be 100% sure these will be useful for WebDav too.
            void CurlPreparePost(void);
            void CurlPrepareGet(void);
            void CurlPrepareUpload(void);
            void CurlPreparePatch(void);
            void CurlPrepareDelete(void);
            // This calls perform and returns whether it was successful or not.
            bool CurlPerform(void);
    };
} // namespace Remote
