#include <string>
#include <vector>
#include <curl/curl.h>

size_t writeDataString(const char *buff, size_t sz, size_t cnt, void *u)
{
    std::string *str = (std::string *)u;
    str->append(buff, 0, sz * cnt);
    return sz * cnt;
}

size_t writeDataHead(const char *buff, size_t sz, size_t cnt, void *u)
{
    std::vector<std::string> *headers = (std::vector<std::string> *)u;
    headers->push_back(buff);
    return sz * cnt;
}

std::string getJSONURL(std::vector<std::string> *headers, const std::string& _url)
{
    std::string ret;
    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, _url.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "JKSV");
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeDataString);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &ret);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 15);
    if(headers)
    {
        curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, writeDataHead);
        curl_easy_setopt(handle, CURLOPT_HEADERDATA, headers);
    }

    if(curl_easy_perform(handle) != CURLE_OK)
        ret.clear();//JIC

    curl_easy_cleanup(handle);
    return ret;
}

size_t writeDataBin(uint8_t *buff, size_t sz, size_t cnt, void *u)
{
    size_t sizeIn = sz * cnt;
    std::vector<uint8_t> *binData = (std::vector<uint8_t> *)u;
    binData->reserve(sizeIn);
    binData->insert(binData->end(), buff, buff + sizeIn);
    return sizeIn;
}

bool getBinURL(std::vector<uint8_t> *out, const std::string& _url)
{
    bool ret = false;
    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, _url.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "JKSV");
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeDataBin);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, out);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 15);
    if(curl_easy_perform(handle) == CURLE_OK)
        ret = true;

    curl_easy_cleanup(handle);
    return ret;
}
