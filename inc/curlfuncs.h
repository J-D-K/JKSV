#ifndef CURLFUNCS_H
#define CURLFUNCS_H

#include <string>
#include <vector>

std::string getJSONURL(std::vector<std::string> *headers, const std::string& _url);
bool getBinURL(std::vector<uint8_t> *out, const std::string& _url);

#endif // CURLFUNCS_H
