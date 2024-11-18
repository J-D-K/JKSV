#pragma once
#include <condition_variable>
#include <cstdint>
#include <curl.h>
#include <mutex>
#include <string>
#include <vector>

namespace Curl
{
    // These structs are for passing data to the Read/Upload and Write/Download functions
    typedef struct
    {
            FILE *TargetFile;
            uint64_t *Offset;
    } UploadStruct;

    // This contains data for writing the file and threaded download.
    typedef struct
    {
            // This stuff is for threaded dowloads
            std::condition_variable BufferCondition;
            std::mutex BufferLock;
            std::vector<unsigned char> DownloadBuffer;
            bool BufferIsFull = false;
            size_t TotalDownloaded = 0;
            // This is
            std::string FilePath;
            uint64_t FileSize;
            uint64_t Offset;
    } DownloadStruct;

    // These are for writing data.
    size_t WriteDataToString(const char *Buffer, size_t ElementSize, size_t ElementCount, std::string *String);
    size_t WriteHeadersToString(const char *Buffer, size_t ElementSize, size_t ElementCount, std::vector<std::string> *StringVector);
    size_t WriteDataToBuffer(const unsigned char *Buffer, size_t ElementSize, size_t ElementCount, Curl::DownloadStruct *DownloadStruct);

    // This function is for a write thread so the the thread calling curl_easy_perform can continue downloading while the one running this one writes.
    void WriteDataBufferToFile(Curl::DownloadStruct *DownloadStruct);

    // This is for uploading
    size_t ReadDataFromFile(char *Buffer, size_t ElementSize, size_t ElementCount, Curl::UploadStruct *UploadStruct);

    // This will go through and extract data from a vector of string headers passed. Returns true on success.
    bool ExtractHeaderValue(const std::vector<std::string> &Headers, std::string_view HeaderName, std::string &ValueOut);
} // namespace Curl
