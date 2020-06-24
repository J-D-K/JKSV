#include <string>
#include <vector>
#include <curl/curl.h>

size_t writeDataString(void *buff, size_t sz, size_t cnt, void *u)
{
    std::string *json = (std::string *)u;
    json->append((const char *)buff);
    return sz * cnt;
}

std::string getJSONURL(std::string *headers, const std::string& _url)
{
    std::string ret;
    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, _url.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeDataString);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &ret);
    if(headers)
    {
        curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, writeDataString);
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
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeDataBin);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, out);
    if(curl_easy_perform(handle) == CURLE_OK)
        ret = true;

    curl_easy_cleanup(handle);
    return ret;
}
