#pragma once

#include <curl/curl.h>
#include <json-c/json.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "curlfuncs.h"
#include "rfs.h"

#define HEADER_CONTENT_TYPE_APP_JSON "Content-Type: application/json; charset=UTF-8"
#define HEADER_AUTHORIZATION "Authorization: Bearer "

#define MIMETYPE_FOLDER "application/vnd.google-apps.folder"

namespace drive
{
    class gd : public rfs::IRemoteFS
    {
        public:
            void setClientID(const std::string& _clientID) { clientID = _clientID; }
            void setClientSecret(const std::string& _clientSecret) { secretID = _clientSecret; }
            void setRefreshToken(const std::string& _refreshToken) { rToken = _refreshToken; }

            bool exhangeAuthCode(const std::string& _authCode);
            bool hasToken() { return token.empty() == false; }
            bool refreshToken();
            bool tokenIsValid();
            
            void clearDriveList() { driveList.clear(); }
            // TODO: This also gets files that do not belong to JKSV
            void driveListInit(const std::string& _q);
            void driveListAppend(const std::string& _q);
            std::vector<rfs::RfsItem> getListWithParent(const std::string& _parent);
            void debugWriteList();
            
            bool createDir(const std::string& _dirName, const std::string& _parent);
            // TODO: This is problematic, because multiple files could share the same name without a parent.
            bool dirExists(const std::string& _dirName);
            bool dirExists(const std::string& _dirName, const std::string& _parent);

            bool fileExists(const std::string& _filename, const std::string& _parent);
            void uploadFile(const std::string& _filename, const std::string& _parent, curlFuncs::curlUpArgs *_upload);
            void updateFile(const std::string& _fileID, curlFuncs::curlUpArgs *_upload);
            void downloadFile(const std::string& _fileID, curlFuncs::curlDlArgs *_download);
            void deleteFile(const std::string& _fileID);

            std::string getClientID() const { return clientID; }
            std::string getClientSecret() const { return secretID; }
            std::string getRefreshToken() const { return rToken; }

            std::string getFileID(const std::string& _name, const std::string& _parent);
            // TODO: This is problematic, because multiple files could share the same name without a parent.
            std::string getDirID(const std::string& _name);
            std::string getDirID(const std::string& _name, const std::string& _parent);

            size_t getDriveListCount() const { return driveList.size(); }
            rfs::RfsItem *getItemAt(unsigned int _ind) { return &driveList[_ind]; }

        private:
            std::vector<rfs::RfsItem> driveList;
            std::string clientID, secretID, token, rToken;
    };
}