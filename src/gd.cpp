#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "gd.h"
#include "fs.h"
#include "curlfuncs.h"
#include "util.h"

/*
Google Drive code for JKSV.
Still major WIP
*/

#define DRIVE_DEFAULT_PARAMS_AND_QUERY "?fields=files(name,id,mimeType,size,parents)&pageSize=1000&q=trashed=false\%20and\%20\%27me\%27\%20in\%20owners"

#define tokenURL "https://oauth2.googleapis.com/token"
#define tokenCheckURL "https://oauth2.googleapis.com/tokeninfo"
#define driveURL "https://www.googleapis.com/drive/v3/files"
#define driveUploadURL "https://www.googleapis.com/upload/drive/v3/files"

static inline void writeDriveError(const std::string& _function, const std::string& _message)
{
    fs::logWrite("Drive/%s: %s\n", _function.c_str(), _message.c_str());
}

static inline void writeCurlError(const std::string& _function, int _cerror)
{
    fs::logWrite("Drive/%s: CURL returned error %i\n", _function.c_str(), _cerror);
}

bool drive::gd::exhangeAuthCode(const std::string& _authCode)
{
    // Header
    curl_slist *postHeader = NULL;
    postHeader = curl_slist_append(postHeader, HEADER_CONTENT_TYPE_APP_JSON);

    // Post json
    json_object *post = json_object_new_object();
    json_object *clientIDString = json_object_new_string(clientID.c_str());
    json_object *secretIDString = json_object_new_string(secretID.c_str());
    json_object *authCodeString = json_object_new_string(_authCode.c_str());
    json_object *redirectUriString = json_object_new_string("urn:ietf:wg:oauth:2.0:oob:auto");
    json_object *grantTypeString = json_object_new_string("authorization_code");
    json_object_object_add(post, "client_id", clientIDString);
    json_object_object_add(post, "client_secret", secretIDString);
    json_object_object_add(post, "code", authCodeString);
    json_object_object_add(post, "redirect_uri", redirectUriString);
    json_object_object_add(post, "grant_type", grantTypeString);

    // Curl Request
    std::string *jsonResp = new std::string;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeader);
    curl_easy_setopt(curl, CURLOPT_URL, tokenURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_get_string(post));
    
    int error = curl_easy_perform(curl);

    json_object *respParse = json_tokener_parse(jsonResp->c_str());
    if (error == CURLE_OK)
    {
        json_object *accessToken = json_object_object_get(respParse, "access_token");
        json_object *refreshToken = json_object_object_get(respParse, "refresh_token");

        if(accessToken && refreshToken)
        {
            token = json_object_get_string(accessToken);
            rToken = json_object_get_string(refreshToken);
        }
        else
            writeDriveError("exchangeAuthCode", jsonResp->c_str());
    }
    else
        writeCurlError("exchangeAuthCode", error);

    delete jsonResp;
    json_object_put(post);
    json_object_put(respParse);
    curl_slist_free_all(postHeader);
    curl_easy_cleanup(curl);

    return true;
}

bool drive::gd::refreshToken()
{
    bool ret = false;

    // Header
    curl_slist *header = NULL;
    header = curl_slist_append(header, HEADER_CONTENT_TYPE_APP_JSON);

    // Post Json
    json_object *post = json_object_new_object();
    json_object *clientIDString = json_object_new_string(clientID.c_str());
    json_object *secretIDString = json_object_new_string(secretID.c_str());
    json_object *refreshTokenString = json_object_new_string(rToken.c_str());
    json_object *grantTypeString = json_object_new_string("refresh_token");
    json_object_object_add(post, "client_id", clientIDString);
    json_object_object_add(post, "client_secret", secretIDString);
    json_object_object_add(post, "refresh_token", refreshTokenString);
    json_object_object_add(post, "grant_type", grantTypeString);

    // Curl
    std::string *jsonResp = new std::string;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(curl, CURLOPT_URL, tokenURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_get_string(post));
    int error = curl_easy_perform(curl);

    json_object *parse = json_tokener_parse(jsonResp->c_str());
    if (error == CURLE_OK)
    {
        json_object *accessToken, *error;
        json_object_object_get_ex(parse, "access_token", &accessToken);
        json_object_object_get_ex(parse, "error", &error);

        if(accessToken)
        {
            token = json_object_get_string(accessToken);
            ret = true;
        }
        else if(error)
            writeDriveError("refreshToken", jsonResp->c_str());
    }

    delete jsonResp;
    json_object_put(post);
    json_object_put(parse);
    curl_slist_free_all(header);
    curl_easy_cleanup(curl);

    return ret;
}

bool drive::gd::tokenIsValid()
{
    bool ret = false;

    std::string url = tokenCheckURL;
    url.append("?access_token=" + token);

    CURL *curl = curl_easy_init();
    std::string *jsonResp = new std::string;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);

    int error = curl_easy_perform(curl);
    json_object *parse = json_tokener_parse(jsonResp->c_str());
    if (error == CURLE_OK)
    {
        json_object *checkError;
        json_object_object_get_ex(parse, "error", &checkError);
        if(!checkError)
            ret = true;
    }

    delete jsonResp;
    json_object_put(parse);
    curl_easy_cleanup(curl);
    return ret;
}

static int requestList(const std::string& _url, const std::string& _token, std::string *_respOut)
{
    int ret = 0;

    // Headers needed
    curl_slist *postHeaders = NULL;
    postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + _token).c_str());

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, _respOut);
    ret = curl_easy_perform(curl);


    curl_slist_free_all(postHeaders);
    curl_easy_cleanup(curl);

    return ret;
}

static void processList(const std::string& _json, std::vector<rfs::RfsItem>& _drvl, bool _clear)
{
    if(_clear)
        _drvl.clear();

    json_object *parse = json_tokener_parse(_json.c_str()), *fileArray;
    json_object_object_get_ex(parse, "files", &fileArray);
    if(fileArray)
    {
        size_t arrayLength = json_object_array_length(fileArray);
        _drvl.reserve(_drvl.size() + arrayLength);
        for(unsigned i = 0; i < arrayLength; i++)
        {
            json_object *idString, *nameString, *mimeTypeString, *size, *parentArray;
            json_object *curFile = json_object_array_get_idx(fileArray, i);
            json_object_object_get_ex(curFile, "id", &idString);
            json_object_object_get_ex(curFile, "name", &nameString);
            json_object_object_get_ex(curFile, "mimeType", &mimeTypeString);
            json_object_object_get_ex(curFile, "size", &size);
            json_object_object_get_ex(curFile, "parents", &parentArray);

            rfs::RfsItem newDirItem;
            newDirItem.name = json_object_get_string(nameString);
            newDirItem.id = json_object_get_string(idString);
            newDirItem.size = json_object_get_int(size);
            if(strcmp(json_object_get_string(mimeTypeString), MIMETYPE_FOLDER) == 0)
                newDirItem.isDir = true;

            if (parentArray)
            {
                size_t parentCount = json_object_array_length(parentArray);
                //There can only be 1 parent, but it's held in an array...
                for (unsigned j = 0; j < parentCount; j++)
                {
                    json_object *parent = json_object_array_get_idx(parentArray, j);
                    newDirItem.parent = json_object_get_string(parent);
                }
            }
            _drvl.push_back(newDirItem);
        }
    }
    json_object_put(parse);
}

void drive::gd::driveListInit(const std::string& _q)
{
    if(!tokenIsValid())
        refreshToken();

    // Request url with specific fields needed.
    std::string url = std::string(driveURL) + std::string(DRIVE_DEFAULT_PARAMS_AND_QUERY);
    if(!_q.empty())
    {
        char *qEsc = curl_easy_escape(NULL, _q.c_str(), _q.length());
        url.append(std::string("\%20and\%20") + std::string(qEsc));
        curl_free(qEsc);
    }
    
    std::string jsonResp;
    int error = requestList(url, token, &jsonResp);
    if(error == CURLE_OK)
        processList(jsonResp, driveList, true);
    else
        writeCurlError("driveListInit", error);
}

void drive::gd::driveListAppend(const std::string& _q)
{
    if(!tokenIsValid())
        refreshToken();

    std::string url = std::string(driveURL) + std::string(DRIVE_DEFAULT_PARAMS_AND_QUERY); 
    if(!_q.empty())
    {
        char *qEsc = curl_easy_escape(NULL, _q.c_str(), _q.length());
        url.append(std::string("\%20and\%20") + std::string(qEsc));
        curl_free(qEsc);
    }

    std::string jsonResp;
    int error = requestList(url, token, &jsonResp);
    if(error == CURLE_OK)
        processList(jsonResp, driveList, false);
    else
        writeCurlError("driveListAppend", error);
}

std::vector<rfs::RfsItem> drive::gd::getListWithParent(const std::string& _parent) {
    std::vector<rfs::RfsItem> filtered;
    for(unsigned i = 0; i < driveList.size(); i++)
    {
        if(driveList[i].parent == _parent)
            filtered.push_back(driveList[i]);
    }
    return filtered;
}

void drive::gd::debugWriteList()
{
    for(auto& di : driveList)
    {
        fs::logWrite("%s\n\t%s\n", di.name.c_str(), di.id.c_str());
        if(!di.parent.empty())
            fs::logWrite("\t%s\n", di.parent.c_str());
    }
}

bool drive::gd::createDir(const std::string& _dirName, const std::string& _parent)
{
    if(!tokenIsValid())
        refreshToken();

    bool ret = true;

    // Headers to use
    curl_slist *postHeaders = NULL;
    postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());
    postHeaders = curl_slist_append(postHeaders, HEADER_CONTENT_TYPE_APP_JSON);

    // JSON To Post
    json_object *post = json_object_new_object();
    json_object *nameString = json_object_new_string(_dirName.c_str());
    json_object *mimeTypeString = json_object_new_string(MIMETYPE_FOLDER);
    json_object_object_add(post, "name", nameString);
    json_object_object_add(post, "mimeType", mimeTypeString);
    if (!_parent.empty())
    {
        json_object *parentsArray = json_object_new_array();
        json_object *parentString = json_object_new_string(_parent.c_str());
        json_object_array_add(parentsArray, parentString);
        json_object_object_add(post, "parents", parentsArray);
    }

    // Curl Request
    std::string *jsonResp = new std::string;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, driveURL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_get_string(post));
    int error = curl_easy_perform(curl);
    
    json_object *respParse = json_tokener_parse(jsonResp->c_str()), *checkError;
    json_object_object_get_ex(respParse, "error", &checkError);
    if (error == CURLE_OK && !checkError)
    {
        //Append it to list
        json_object *id;
        json_object_object_get_ex(respParse, "id", &id);

        rfs::RfsItem newDir;
        newDir.name = _dirName;
        newDir.id = json_object_get_string(id);
        newDir.isDir = true;
        newDir.size = 0;
        newDir.parent = _parent;
        driveList.push_back(newDir);
    }
    else
        ret = false;

    delete jsonResp;
    json_object_put(post);
    json_object_put(respParse);
    curl_slist_free_all(postHeaders);
    curl_easy_cleanup(curl);
    return ret;
}

bool drive::gd::dirExists(const std::string& _dirName)
{
    for(unsigned i = 0; i < driveList.size(); i++)
    {
        if(driveList[i].isDir && driveList[i].name == _dirName)
            return true;
    }
    return false;
}

bool drive::gd::dirExists(const std::string& _dirName, const std::string& _parent)
{
    for(unsigned i = 0; i < driveList.size(); i++)
    {
        if(driveList[i].isDir && driveList[i].name == _dirName && driveList[i].parent == _parent)
            return true;
    }
    return false;
}

bool drive::gd::fileExists(const std::string& _filename, const std::string& _parent)
{
    for(unsigned i = 0; i < driveList.size(); i++)
    {
        if(!driveList[i].isDir && driveList[i].name == _filename && driveList[i].parent == _parent)
            return true;
    }
    return false;
}

void drive::gd::uploadFile(const std::string& _filename, const std::string& _parent, curlFuncs::curlUpArgs *_upload)
{
    if(!tokenIsValid())
        refreshToken();

    std::string url = driveUploadURL;
    url.append("?uploadType=resumable");

    // Headers
    curl_slist *postHeaders = NULL;
    postHeaders = curl_slist_append(postHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());
    postHeaders = curl_slist_append(postHeaders, HEADER_CONTENT_TYPE_APP_JSON);

    // Post JSON
    json_object *post = json_object_new_object();
    json_object *nameString = json_object_new_string(_filename.c_str());
    json_object_object_add(post, "name", nameString);
    if (!_parent.empty())
    {
        json_object *parentArray = json_object_new_array();
        json_object *parentString = json_object_new_string(_parent.c_str());
        json_object_array_add(parentArray, parentString);
        json_object_object_add(post, "parents", parentArray);
    }

    // Curl upload request
    std::string *jsonResp = new std::string;
    std::vector<std::string> *headers = new std::vector<std::string>;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, postHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlFuncs::writeHeaders);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_get_string(post));
    
    int error = curl_easy_perform(curl);
    std::string location = curlFuncs::getHeader("Location", headers);
    if (error == CURLE_OK && location != HEADER_ERROR)
    {
        CURL *curlUp = curl_easy_init();
        curl_easy_setopt(curlUp, CURLOPT_PUT, 1);
        curl_easy_setopt(curlUp, CURLOPT_URL, location.c_str());
        curl_easy_setopt(curlUp, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
        curl_easy_setopt(curlUp, CURLOPT_WRITEDATA, jsonResp);
        curl_easy_setopt(curlUp, CURLOPT_READFUNCTION, curlFuncs::readDataFile);
        curl_easy_setopt(curlUp, CURLOPT_READDATA, _upload);
        curl_easy_setopt(curlUp, CURLOPT_UPLOAD_BUFFERSIZE, UPLOAD_BUFFER_SIZE);
        curl_easy_setopt(curlUp, CURLOPT_UPLOAD, 1);
        curl_easy_perform(curlUp);
        curl_easy_cleanup(curlUp);

        json_object *parse = json_tokener_parse(jsonResp->c_str()), *id, *name, *mimeType;
        json_object_object_get_ex(parse, "id", &id);
        json_object_object_get_ex(parse, "name", &name);
        json_object_object_get_ex(parse, "mimeType", &mimeType);

        if(name && id && mimeType)
        {
            rfs::RfsItem uploadData;
            uploadData.id = json_object_get_string(id);
            uploadData.name = json_object_get_string(name);
            uploadData.isDir = false;
            uploadData.size = *_upload->o;//should be safe to use
            uploadData.parent = _parent;
            driveList.push_back(uploadData);
        }
        json_object_put(parse);
    }
    else
        writeCurlError("uploadFile", error);

    delete jsonResp;
    delete headers;
    json_object_put(post);
    curl_slist_free_all(postHeaders);
}

void drive::gd::updateFile(const std::string& _fileID, curlFuncs::curlUpArgs *_upload)
{
    if(!tokenIsValid())
        refreshToken();

    //URL
    std::string url = driveUploadURL;
    url.append("/" + _fileID);
    url.append("?uploadType=resumable");

    //Header
    curl_slist *patchHeader = NULL;
    patchHeader = curl_slist_append(patchHeader, std::string(HEADER_AUTHORIZATION + token).c_str());

    //Curl
    std::string *jsonResp = new std::string;
    std::vector<std::string> *headers = new std::vector<std::string>;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, patchHeader);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFuncs::writeDataString);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlFuncs::writeHeaders);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonResp);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, headers);
    
    int error = curl_easy_perform(curl);
    std::string location = curlFuncs::getHeader("Location", headers);
    if(error == CURLE_OK && location != HEADER_ERROR)
    {
        CURL *curlPatch = curl_easy_init();
        curl_easy_setopt(curlPatch, CURLOPT_PUT, 1);
        curl_easy_setopt(curlPatch, CURLOPT_URL, location.c_str());
        curl_easy_setopt(curlPatch, CURLOPT_READFUNCTION, curlFuncs::readDataFile);
        curl_easy_setopt(curlPatch, CURLOPT_READDATA, _upload);
        curl_easy_setopt(curlPatch, CURLOPT_UPLOAD_BUFFERSIZE, UPLOAD_BUFFER_SIZE);
        curl_easy_setopt(curlPatch, CURLOPT_UPLOAD, 1);
        curl_easy_perform(curlPatch);
        curl_easy_cleanup(curlPatch);

        for(unsigned i = 0; i < driveList.size(); i++)
        {
            if(driveList[i].id == _fileID)
            {
                driveList[i].size = *_upload->o;
                break;
            }
        }
    }

    delete jsonResp;
    delete headers;
    curl_slist_free_all(patchHeader);
    curl_easy_cleanup(curl);
}

void drive::gd::downloadFile(const std::string& _fileID, curlFuncs::curlDlArgs *_download)
{
    if(!tokenIsValid())
        refreshToken();

    //URL
    std::string url = driveURL;
    url.append("/" + _fileID);
    url.append("?alt=media");

    //Headers
    curl_slist *getHeaders = NULL;
    getHeaders = curl_slist_append(getHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());

    //Downloading is threaded because it's too slow otherwise
    rfs::dlWriteThreadStruct dlWrite;
    dlWrite.cfa = _download;

    Thread writeThread;
    threadCreate(&writeThread, rfs::writeThread_t, &dlWrite, NULL, 0x8000, 0x2B, 2);

    //Curl
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, getHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rfs::writeDataBufferThreaded);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dlWrite);
    threadStart(&writeThread);
    
    curl_easy_perform(curl);

    threadWaitForExit(&writeThread);
    threadClose(&writeThread);

    curl_slist_free_all(getHeaders);
    curl_easy_cleanup(curl);
}

void drive::gd::deleteFile(const std::string& _fileID)
{
    if(!tokenIsValid())
        refreshToken();

    //URL
    std::string url = driveURL;
    url.append("/" + _fileID);

    //Header
    curl_slist *delHeaders = NULL;
    delHeaders = curl_slist_append(delHeaders, std::string(HEADER_AUTHORIZATION + token).c_str());

    //Curl
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, delHeaders);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_perform(curl);

    for(unsigned i = 0; i < driveList.size(); i++)
    {
        if(driveList[i].id == _fileID)
        {
            driveList.erase(driveList.begin() + i);
            break;
        }
    }

    curl_slist_free_all(delHeaders);
    curl_easy_cleanup(curl);
}

std::string drive::gd::getFileID(const std::string& _name, const std::string& _parent)
{
    for(unsigned i = 0; i < driveList.size(); i++)
    {
        if(!driveList[i].isDir && driveList[i].name == _name && driveList[i].parent == _parent)
            return driveList[i].id;
    }
    return "";
}

std::string drive::gd::getDirID(const std::string& _name)
{
    for(unsigned i = 0; i < driveList.size(); i++)
    {
        if(driveList[i].isDir && driveList[i].name == _name)
            return driveList[i].id;
    }
    return "";
}

std::string drive::gd::getDirID(const std::string& _name, const std::string& _parent)
{
    for(unsigned i = 0; i < driveList.size(); i++)
    {
        if(driveList[i].isDir && driveList[i].name == _name && driveList[i].parent == _parent)
            return driveList[i].id;
    }
    return "";
}