#pragma once

#include <curl/curl.h>
#include <json-c/json.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "curlfuncs.h"

#define HEADER_CONTENT_TYPE_APP_JSON "Content-Type: application/json; charset=UTF-8"
#define HEADER_AUTHORIZATION "Authorization: Bearer "

#define MIMETYPE_FOLDER "application/vnd.google-apps.folder"

namespace drive
{
    typedef struct 
    {
        std::string name, id, mimeType;
        unsigned int size;
        std::vector<std::string> parents;
    } gdDirItem;

    class gd
    {
        public:
            gd(const std::string& _clientID, const std::string& _secretID, const std::string& _authCode, const std::string& _rToken);

            void exhangeAuthCode(const std::string& _authCode);
            bool hasToken() { return token.empty() == false; }
            void refreshToken();
            bool tokenIsValid();
            //Drive query parameters are appened to the default
            void loadDriveList(const std::string& _qParams);

            void debugWriteList();
            
            bool createDir(const std::string& _dirName);
            bool dirExists(const std::string& _dirName);
            void setRootDir(const std::string& _dirID)
            { 
                rootDir = _dirID; 
                parentDir = _dirID; 
                loadDriveList(""); 
            }
            void returnToRoot(){ parentDir = rootDir; loadDriveList(""); }
            void chDir(const std::string& _dirID) { parentDir = _dirID; loadDriveList(""); }
            
            bool fileExists(const std::string& _filename);
            void uploadFile(const std::string& _filename, curlFuncs::curlUpArgs *_upload);
            void updateFile(const std::string& _fileID, curlFuncs::curlUpArgs *_upload);
            void downloadFile(const std::string& _fileID, curlFuncs::curlDlArgs *_download);
            void deleteFile(const std::string& _fileID);

            std::string getClientID() const { return clientID; }
            std::string getClientSecret() const { return secretID; }
            std::string getRefreshToken() const { return rToken; }
            std::string getFileID(const std::string& name);

            size_t getDriveListCount() const { return driveList.size(); }
            drive::gdDirItem *getDirItemAt(unsigned int _ind) { return &driveList[_ind]; }

        private:
            std::vector<gdDirItem> driveList;
            std::string clientID, secretID, token, rToken;
            std::string rootDir = "", parentDir = "";
    };
}