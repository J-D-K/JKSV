#pragma once

#include <curl/curl.h>
#include <string>
#include <tinyxml2.h>

#include "rfs.h"

namespace rfs {

    // Note: Everything declared an "id" is the full path component from the origin to the resource starting with a "/".
    // Note: Directories ALWAYS have a trailing / while files NEVER have a trailing /
    // e.g. /<basePath>/JKSV/
    // e.g. /<basePath>/JKSV/<title-id>/
    // e.g. /<basePath>/JKSV/<title-id>/<file>
    // e.g. /
    // other string arguments never have any leading or trailing "/"
    class WebDav : public IRemoteFS {
    private:
        CURL* curl;
        std::string origin;
        std::string basePath;
        std::string username;
        std::string password;


        std::vector<RfsItem> parseXMLResponse(const std::string& xml);
        bool resourceExists(const std::string& id);
        std::string appendResourceToParentId(const std::string& resourceName, const std::string& parentId, bool isDir);
        std::string getNamespacePrefix(tinyxml2::XMLElement* root, const std::string& nsURI);

    public:
        WebDav(const std::string& origin,
               const std::string& username,
               const std::string& password);
        ~WebDav();

        bool createDir(const std::string& dirName, const std::string& parentId);
        bool dirExists(const std::string& dirName, const std::string& parentId);

        bool fileExists(const std::string& filename, const std::string& parentId);
        void uploadFile(const std::string& filename, const std::string& parentId, curlFuncs::curlUpArgs *_upload);
        void updateFile(const std::string& fileID, curlFuncs::curlUpArgs *_upload);
        void downloadFile(const std::string& fileID, curlFuncs::curlDlArgs *_download);
        void deleteFile(const std::string& fileID);

        std::string getFileID(const std::string& name, const std::string& parentId);
        std::string getDirID(const std::string& dirName, const std::string& parentId);

        std::vector<RfsItem> getListWithParent(const std::string& _parent);
        std::string getDisplayNameFromURL(const std::string &url);
    };
}
