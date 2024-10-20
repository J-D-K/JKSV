#pragma once

#include <string>
#include "curlfuncs.h"
#include <mutex>

#define UPLOAD_BUFFER_SIZE 0x8000
#define DOWNLOAD_BUFFER_SIZE 0xC00000
#define USER_AGENT "JKSV"

namespace rfs {

    typedef struct
    {
        std::string name, id, parent;
        bool isDir = false;
        unsigned int size;
    } RfsItem;

    class IRemoteFS
    {
    public:
        virtual ~IRemoteFS() {}  // Virtual destructor to allow correct deletion through the base class pointer

        virtual bool createDir(const std::string& _dirName, const std::string& _parent) = 0;
        virtual bool dirExists(const std::string& _dirName, const std::string& _parent) = 0;
        virtual bool fileExists(const std::string& _filename, const std::string& _parent) = 0;
        virtual void uploadFile(const std::string& _filename, const std::string& _parent, curlFuncs::curlUpArgs *_upload) = 0;
        virtual void updateFile(const std::string& _fileID, curlFuncs::curlUpArgs *_upload) = 0;
        virtual void downloadFile(const std::string& _fileID, curlFuncs::curlDlArgs *_download) = 0;
        virtual void deleteFile(const std::string& _fileID) = 0;

        virtual std::string getFileID(const std::string& _name, const std::string& _parent) = 0;
        virtual std::string getDirID(const std::string& _name, const std::string& _parent) = 0;

        virtual std::vector<RfsItem> getListWithParent(const std::string& _parent) = 0;
    };

    // Shared multi-threading definitions
    typedef struct
    {
        curlFuncs::curlDlArgs *cfa;
        std::mutex dataLock;
        std::condition_variable cond;
        std::vector<uint8_t> sharedBuffer;
        bool bufferFull = false;
        unsigned int downloaded = 0;
    } dlWriteThreadStruct;

    extern std::vector<uint8_t> downloadBuffer;
    void writeThread_t(void *a);
    size_t writeDataBufferThreaded(uint8_t *buff, size_t sz, size_t cnt, void *u);
}
