#include "Curl/Curl.hpp"
#include "util.h"

namespace
{
    constexpr size_t DOWNLOAD_BUFFER_MAX = 0x600000;
}

size_t Curl::WriteDataToString(const char *Buffer, size_t ElementSize, size_t ElementCount, std::string *String)
{
    *String += Buffer;
    return ElementSize * ElementCount;
}

size_t Curl::WriteHeadersToString(const char *Buffer, size_t ElementSize, size_t ElementCount, std::vector<std::string> *StringVector)
{
    StringVector->push_back(Buffer);
    return ElementSize * ElementCount;
}

size_t Curl::WriteDataToBuffer(const unsigned char *Buffer, size_t ElementSize, size_t ElementCount, Curl::DownloadStruct *DownloadStruct)
{
    // Append incoming data to the end of the vector.
    DownloadStruct->DownloadBuffer.insert(DownloadStruct->DownloadBuffer.end(), Buffer);
    // If we've gotten to or exeeded the max size of the buffer or whole file.
    if (DownloadStruct->DownloadBuffer.size() >= DOWNLOAD_BUFFER_MAX || DownloadStruct->TotalDownloaded >= DownloadStruct->FileSize)
    {
        // This should get interesting... Set and notify other thread buffer is ready to go. I don't know if this will work how I want yet...
        DownloadStruct->BufferIsFull = true;
        DownloadStruct->BufferCondition.notify_one();
        // Wait for the mutex to be released here to be sure the buffer was read and copied?
        std::unique_lock<std::mutex> BufferLock(DownloadStruct->BufferLock);
        DownloadStruct->BufferCondition.wait(BufferLock, [DownloadStruct]() { return DownloadStruct->BufferIsFull == false; });
    }
    // Update total downloaded.
    DownloadStruct->TotalDownloaded += ElementSize * ElementCount;
    // Write thread function updates total read since it's more crucial there.
    return ElementSize * ElementCount;
}

void Curl::WriteDataBufferToFile(Curl::DownloadStruct *DownloadStruct)
{
    // This has its own local buffer to copy and write from.
    std::vector<unsigned char> LocalBuffer;
    // Total amount of data written.
    size_t TotalDataWritten = 0;
    // File we're writing data to.
    std::unique_ptr<std::FILE, decltype(&std::fclose)> TargetFile(std::fopen(DownloadStruct->FilePath.c_str(), "wb"), std::fclose);
    while (TotalDataWritten < DownloadStruct->FileSize)
    {
        // Scoped so lock releases after notify_one.
        {
            std::unique_lock<std::mutex> BufferLock(DownloadStruct->BufferLock);
            DownloadStruct->BufferCondition.wait(BufferLock, [DownloadStruct]() { return DownloadStruct->BufferIsFull; });
            // Copy download buffer to local one.
            LocalBuffer.assign(DownloadStruct->DownloadBuffer.begin(), DownloadStruct->DownloadBuffer.end());
            // Clear download buffer.
            DownloadStruct->DownloadBuffer.clear();
            // Set var
            DownloadStruct->BufferIsFull = false;
            // Notify
            DownloadStruct->BufferCondition.notify_one();
        }
        TotalDataWritten += std::fwrite(LocalBuffer.data(), sizeof(unsigned char), LocalBuffer.size(), TargetFile.get());
    }
}

size_t Curl::ReadDataFromFile(char *Buffer, size_t ElementSize, size_t ElementCount, Curl::UploadStruct *UploadStruct)
{
    size_t BytesRead = fread(Buffer, ElementSize, ElementCount, UploadStruct->TargetFile);

    *UploadStruct->Offset += BytesRead;

    return BytesRead;
}

bool Curl::ExtractHeaderValue(const std::vector<std::string> &Headers, std::string_view HeaderName, std::string &ValueOut)
{
    for (size_t i = 0; i < Headers.size(); i++)
    {
        size_t ColonPosition = Headers[i].find_first_of(':');
        if (ColonPosition != Headers[i].npos && Headers[i].substr(0, ColonPosition) == HeaderName)
        {
            ValueOut = Headers[i].substr(ColonPosition + 2);
            util::stripChar('\n', ValueOut);
            util::stripChar('\r', ValueOut);
            return true;
        }
    }
    return false;
}
