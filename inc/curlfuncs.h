#pragma once

#include <string>
#include <vector>

#define HEADER_ERROR "ERROR"

namespace curlFuncs
{
    typedef struct
    {
        FILE *f;
        uint64_t *o;
    } curlUpArgs;

    typedef struct
    {
        std::string path;
        unsigned int size;
        uint64_t *o;
    } curlDlArgs;

    size_t writeDataString(const char *buff, size_t sz, size_t cnt, void *u);
    size_t writeHeaders(const char *buff, size_t sz, size_t cnt, void *u);
    size_t readDataFile(char *buff, size_t sz, size_t cnt, void *u);
    size_t readDataBuffer(char *buff, size_t sz, size_t cnt, void *u);
    size_t writeDataFile(const char *buff, size_t sz, size_t cnt, void *u);
    size_t writeDataBuffer(const char *buff, size_t sz, size_t cnt, void *u);

    std::string getHeader(const std::string& _name, std::vector<std::string> *h);

    //Shortcuts/legacy
    std::string getJSONURL(std::vector<std::string> *headers, const std::string& url);
    bool getBinURL(std::vector<uint8_t> *out, const std::string& url);
}
