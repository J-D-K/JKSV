#include "webdav.h"
#include "fs.h"
#include "tinyxml2.h"

rfs::WebDav::WebDav(const std::string& origin, const std::string& username, const std::string& password)
    : origin(origin), username(username), password(password)
{
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
        if (!username.empty())
            curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());

        if (!password.empty())
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    }
}

rfs::WebDav::~WebDav() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
}

bool rfs::WebDav::resourceExists(const std::string& id) {
    CURL* local_curl = curl_easy_duphandle(curl);

    // we expect id to be properly escaped and starting with a /
    std::string fullUrl = origin + id;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Depth: 0");

    curl_easy_setopt(local_curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(local_curl, CURLOPT_CUSTOMREQUEST, "PROPFIND");
    curl_easy_setopt(local_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(local_curl, CURLOPT_NOBODY, 1L); // do not include the response body

    CURLcode res = curl_easy_perform(local_curl);

    curl_slist_free_all(headers); // free the custom headers

    bool ret = false;
    if(res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(local_curl, CURLINFO_RESPONSE_CODE, &response_code);
        if(response_code == 207) { // 207 Multi-Status is a successful response for PROPFIND
            ret = true;
        }
    } else {
        fs::logWrite("WebDav: directory exists failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(local_curl);

    return ret;
}

// parent ID can never be empty
std::string rfs::WebDav::appendResourceToParentId(const std::string& resourceName, const std::string& parentId, bool isDir) {
    char *escaped = curl_easy_escape(curl, resourceName.c_str(), 0);
    // we always expect parent to be properly URL encoded.
    std::string ret = parentId + std::string(escaped) + (isDir ? "/" : "");
    curl_free(escaped);
    return ret;
}



bool rfs::WebDav::createDir(const std::string& dirName, const std::string& parentId) {
    CURL* local_curl = curl_easy_duphandle(curl);

    std::string urlPath = appendResourceToParentId(dirName, parentId, true);
    std::string fullUrl = origin + urlPath;

    fs::logWrite("WebDav: Create directory at %s\n", fullUrl.c_str());

    curl_easy_setopt(local_curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(local_curl, CURLOPT_CUSTOMREQUEST, "MKCOL");

    CURLcode res = curl_easy_perform(local_curl);

    if(res != CURLE_OK) {
        fs::logWrite("WebDav: directory creation failed: %s\n", curl_easy_strerror(res));
    }

    bool ret = res == CURLE_OK;

    curl_easy_cleanup(local_curl);

    return ret;
}

// we always expect parent to be properly URL encoded.
void rfs::WebDav::uploadFile(const std::string& filename, const std::string& parentId, curlFuncs::curlUpArgs *_upload) {
    std::string fileId = appendResourceToParentId(filename, parentId, false);
    updateFile(fileId, _upload);
}
void rfs::WebDav::updateFile(const std::string& _fileID, curlFuncs::curlUpArgs *_upload) {
    // for webdav, same as upload
    CURL* local_curl = curl_easy_duphandle(curl);

    std::string fullUrl = origin + _fileID;

    curl_easy_setopt(local_curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(local_curl, CURLOPT_UPLOAD, 1L); // implicit PUT
    curl_easy_setopt(local_curl, CURLOPT_READFUNCTION, curlFuncs::readDataFile);
    curl_easy_setopt(local_curl, CURLOPT_READDATA, _upload);
    curl_easy_setopt(local_curl, CURLOPT_UPLOAD_BUFFERSIZE, UPLOAD_BUFFER_SIZE);
    curl_easy_setopt(local_curl, CURLOPT_UPLOAD, 1);


    CURLcode res = curl_easy_perform(local_curl);
    if(res != CURLE_OK) {
        fs::logWrite("WebDav: file upload failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(local_curl); // Clean up the CURL handle
}
void rfs::WebDav::downloadFile(const std::string& _fileID, curlFuncs::curlDlArgs *_download) {
    //Downloading is threaded because it's too slow otherwise
    dlWriteThreadStruct dlWrite;
    dlWrite.cfa = _download;

    Thread writeThread;
    threadCreate(&writeThread, writeThread_t, &dlWrite, NULL, 0x8000, 0x2B, 2);


    CURL* local_curl = curl_easy_duphandle(curl);

    std::string fullUrl = origin + _fileID;
    curl_easy_setopt(local_curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(local_curl, CURLOPT_WRITEFUNCTION, writeDataBufferThreaded);
    curl_easy_setopt(local_curl, CURLOPT_WRITEDATA, &dlWrite);
    threadStart(&writeThread);

    CURLcode res = curl_easy_perform(local_curl);

    // Copied from gd.cpp implementation.
    // TODO: Not sure how a thread helps if this parent waits here.
    threadWaitForExit(&writeThread);
    threadClose(&writeThread);

    if(res != CURLE_OK) {
        fs::logWrite("WebDav: file download failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(local_curl);
}
void rfs::WebDav::deleteFile(const std::string& _fileID) {
    CURL* local_curl = curl_easy_duphandle(curl);

    std::string fullUrl = origin + _fileID;
    curl_easy_setopt(local_curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(local_curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    CURLcode res = curl_easy_perform(local_curl);
    if(res != CURLE_OK) {
        fs::logWrite("WebDav: file deletion failed: %s\n", curl_easy_strerror(res));
    }

    curl_easy_cleanup(local_curl);
}

bool rfs::WebDav::dirExists(const std::string& dirName, const std::string& parentId) {
    std::string urlPath = getDirID(dirName, parentId);
    return resourceExists(urlPath);
}

bool rfs::WebDav::fileExists(const std::string& filename, const std::string& parentId) {
    std::string urlPath = appendResourceToParentId(filename, parentId, false);
    return resourceExists(urlPath);
}

std::string rfs::WebDav::getFileID(const std::string& filename, const std::string& parentId) {
    return appendResourceToParentId(filename, parentId, false);
}

std::string rfs::WebDav::getDirID(const std::string& dirName, const std::string& parentId) {
    return appendResourceToParentId(dirName, parentId, true);
}

std::vector<rfs::RfsItem> rfs::WebDav::getListWithParent(const std::string& _parentId) {
    std::vector<rfs::RfsItem> list;

    CURL* local_curl = curl_easy_duphandle(curl);

    // we expect _resource to be properly escaped
    std::string fullUrl = origin + _parentId;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Depth: 1");

    std::string responseString;

    curl_easy_setopt(local_curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(local_curl, CURLOPT_CUSTOMREQUEST, "PROPFIND");
    curl_easy_setopt(local_curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(local_curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(local_curl, CURLOPT_WRITEDATA, &responseString);

    CURLcode res = curl_easy_perform(local_curl);

    if(res == CURLE_OK) {
        long response_code;
        curl_easy_getinfo(local_curl, CURLINFO_RESPONSE_CODE, &response_code);
        if(response_code == 207) { // 207 Multi-Status is a successful response for PROPFIND
            fs::logWrite("WebDav: Response from WebDav. Parsing.\n");
            std::vector<rfs::RfsItem> items = parseXMLResponse(responseString);

            // insert into array
            // TODO: Filter for zip?
            list.insert(list.end(), items.begin(), items.end());
        }
    } else {
        fs::logWrite("WebDav: directory listing failed: %s\n", curl_easy_strerror(res));
    }

    curl_slist_free_all(headers); // free the custom headers
    curl_easy_cleanup(local_curl);
    return list;
}

// Helper
std::string rfs::WebDav::getNamespacePrefix(tinyxml2::XMLElement* root, const std::string& nsURI) {
    for(const tinyxml2::XMLAttribute* attr = root->FirstAttribute(); attr; attr = attr->Next()) {
        std::string name = attr->Name();
        std::string value = attr->Value();
        if(value == nsURI) {
            auto pos = name.find(':');
            if(pos != std::string::npos) {
                return name.substr(pos + 1);
            } else {
                return "";  // Default namespace (no prefix)
            }
        }
    }
    return "";  // No namespace found
}

std::vector<rfs::RfsItem> rfs::WebDav::parseXMLResponse(const std::string& xml) {
    std::vector<RfsItem> items;
    tinyxml2::XMLDocument doc;

    if(doc.Parse(xml.c_str()) != tinyxml2::XML_SUCCESS) {
        fs::logWrite("WebDav: Failed to parse XML from Server\n");
        return items;
    }

    // Get the root element
    tinyxml2::XMLElement *root = doc.RootElement();
    std::string nsPrefix = getNamespacePrefix(root, "DAV:");
    nsPrefix = !nsPrefix.empty() ? nsPrefix + ":" : nsPrefix;  // Append colon if non-empty

    fs::logWrite("WebDav: Parsing response, using prefix: %s\n", nsPrefix.c_str());

    // Loop through the responses
    tinyxml2::XMLElement* responseElem = root->FirstChildElement((nsPrefix + "response").c_str());

    std::string parentId;

    while (responseElem) {
        RfsItem item;
        item.size = 0;

        tinyxml2::XMLElement* hrefElem = responseElem->FirstChildElement((nsPrefix + "href").c_str());
        if (hrefElem) {
            std::string hrefText = hrefElem->GetText();
            // href can be absolute URI or relative reference. ALWAYS convert to relative reference
            if(hrefText.find(origin) == 0) {
                hrefText = hrefText.substr(origin.length());
            }
            item.id = hrefText;
            item.parent = parentId;
        }

        tinyxml2::XMLElement* propstatElem = responseElem->FirstChildElement((nsPrefix + "propstat").c_str());
        if (propstatElem) {
            tinyxml2::XMLElement* propElem = propstatElem->FirstChildElement((nsPrefix + "prop").c_str());
            if (propElem) {
                tinyxml2::XMLElement* displaynameElem = propElem->FirstChildElement((nsPrefix + "displayname").c_str());
                if (displaynameElem) {
                    item.name = displaynameElem->GetText();
                } else {
                    // Fallback to name from href
                    item.name = getDisplayNameFromURL(item.id);
                }

                tinyxml2::XMLElement* resourcetypeElem = propElem->FirstChildElement((nsPrefix + "resourcetype").c_str());
                if (resourcetypeElem) {
                    item.isDir = resourcetypeElem->FirstChildElement((nsPrefix + "collection").c_str()) != nullptr;
                }

                tinyxml2::XMLElement* contentLengthElem = propElem->FirstChildElement((nsPrefix + "getcontentlength").c_str());
                if (contentLengthElem) {
                    const char* sizeStr = contentLengthElem->GetText();
                    if (sizeStr) {
                        item.size = std::stoi(sizeStr);
                    }
                }
            }
        }

        responseElem = responseElem->NextSiblingElement((nsPrefix + "response").c_str());

        // first Item is always the parent.
        if (parentId.empty()) {
            parentId = item.id;
            continue; // do not push parent to list (we are only interested in the children)
        }

        items.push_back(item);
    }

    return items;
}

// Function to extract and URL decode the filename from the URL
std::string rfs::WebDav::getDisplayNameFromURL(const std::string &url) {
    // Find the position of the last '/'
    size_t pos = url.find_last_of('/');
    if (pos == std::string::npos) {
        // If '/' is not found, return the whole URL as it is
        return url;
    }

    // Extract the filename from the URL
    std::string encodedFilename = url.substr(pos + 1);

    // URL decode the filename
    int outlength;
    char *decodedFilename = curl_easy_unescape(curl, encodedFilename.c_str(), encodedFilename.length(), &outlength);
    std::string result(decodedFilename, outlength);

    // Free the memory allocated by curl_easy_unescape
    curl_free(decodedFilename);

    return result;
}
