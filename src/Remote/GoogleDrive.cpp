#include "Remote/GoogleDrive.hpp"
#include "Curl/Curl.hpp"
#include "JSON.hpp"
#include "Remote/Remote.hpp"
#include <algorithm>
#include <cstring>
#include <switch.h>
#include <thread>

namespace
{
    // Header strings.
    static const char *HEADER_CONTENT_TYPE_APP_JSON = "Content-Type: application/json; charset=UTF-8";
    static const char *HEADER_AUTHORIZATION_BEARER = "Authorization: Bearer %s";

    // Mime-type for folders.
    static const char *MIME_TYPE_FOLDER = "application/vnd.google-apps.folder";

    // These are the various addresses and strings for Google's API.
    const char *DRIVE_TOKEN_URL = "https://oauth2.googleapis.com/token";
    const char *DRIVE_TOKEN_VERIFY_URL = "https://oauth2.googleapis.com/tokeninfo?%s";
    const char *DRIVE_API_URL = "https://www.googleapis.com/drive/v3/files";
    const char *DRIVE_UPLOAD_URL = "https://www.googleapis.com/upload/drive/v3/files";
    const char *DRIVE_APPROVAL_URL = "https://accounts.google.com/o/oauth2/approval/";
    const char *DRIVE_REDIRECT_URI = "urn:ietf:wg:oauth:2.0:oob:auto";
    const char *DRIVE_GRANT_TYPE_AUTH_CODE = "authorization_code";
    const char *DRIVE_GRANT_TYPE_REFRESH_TOKEN = "refresh_token";
    const char *DRIVE_LOGIN_URL =
        "https://accounts.google.com/o/oauth2/v2/auth?client_id=%s&redirect_uri=urn:ietf:wg:oauth:2.0:oob:auto&response_type=code&scope=https:/"
        "/www.googleapis.com/auth/drive";

    // These are they keys used in creating and reading drive json posts and replies
    const char *DRIVE_JSON_KEY_INSTALLED = "installed";
    const char *DRIVE_JSON_KEY_CLIENT_ID = "client_id";
    const char *DRIVE_JSON_KEY_CLIENT_SECRET = "client_secret";
    const char *DRIVE_JSON_KEY_REFRESH_TOKEN = "refresh_token";
    const char *DRIVE_JSON_KEY_AUTH_CODE = "code";
    const char *DRIVE_JSON_KEY_REDIRECT_URI = "redirect_uri";
    const char *DRIVE_JSON_KEY_GRANT_TYPE = "grant_type";

    // This is the default query sent to get everything listed at once.
    const char *DRIVE_DEFAULT_QUERY =
        "?fields=files(name,id,mimeType,size,parents)&pageSize=1000&q=trashed=false\%20and\%20\%27me\%27\%20in\%20owners";

    // This is the buffer size for sprintf'ing and reply urls so it can be adjusted easier.
    constexpr size_t URL_BUFFER_SIZE = 0x300;
} // namespace

Remote::GoogleDrive::GoogleDrive(const char *DriveJSONPath)
{
    JSON::Object DriveJSON = JSON::NewObject(json_object_from_file, DriveJSONPath);
    if (!DriveJSON)
    {
        return;
    }

    json_object *Installed = json_object_object_get(DriveJSON.get(), DRIVE_JSON_KEY_INSTALLED);
    if (!Installed)
    {
        return;
    }

    json_object *ClientID = json_object_object_get(Installed, DRIVE_JSON_KEY_CLIENT_ID);
    json_object *ClientSecret = json_object_object_get(Installed, DRIVE_JSON_KEY_CLIENT_SECRET);
    if (!ClientID || !ClientSecret)
    {
        return;
    }
    m_ClientID = json_object_get_string(ClientID);
    m_ClientSecret = json_object_get_string(ClientSecret);

    json_object *RefreshToken = json_object_object_get(Installed, DRIVE_JSON_KEY_REFRESH_TOKEN);
    if (RefreshToken)
    {
        m_RefreshToken = json_object_get_string(RefreshToken);
    }

    m_IsInitialized = true;
}

Remote::GoogleDrive::~GoogleDrive()
{
}

bool Remote::GoogleDrive::CreateDirectory(std::string_view DirectoryName, std::string_view ParentID)
{
    if (!GoogleDrive::TokenIsValid() && !GoogleDrive::RefreshToken())
    {
        // The token isn't valid and there was an error refreshing it.
        return false;
    }

    // To do: Figure a better solution for headers.
    curl_slist *PostHeaders = curl_slist_append(PostHeaders, m_AuthorizationHeaderyu);
    PostHeaders = curl_slist_append(PostHeaders, HEADER_CONTENT_TYPE_APP_JSON);

    // Post json
    JSON::Object PostJSON = JSON::NewObject(json_object_new_object);
    json_object *DirectoryNameString = json_object_new_string(DirectoryName.data());
    json_object *MimeTypeString = json_object_new_string(MIME_TYPE_FOLDER);
    json_object_object_add(PostJSON.get(), "name", DirectoryNameString);
    json_object_object_add(PostJSON.get(), "mimeType", MimeTypeString);
    if (!ParentID.empty())
    {
        // This is so stupid and a waste of time.
        json_object *ParentArray = json_object_new_array();
        json_object *ParentIDString = json_object_new_string(ParentID.data());
        json_object_array_add(ParentArray, ParentIDString);
        json_object_object_add(PostJSON.get(), ParentArray);
    }

    std::string Response;
    Storage::CurlPreparePost();
    curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, PostHeaders);
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, DRIVE_UPLOAD_URL);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, Curl::WriteDataToString);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &Response);
    curl_easy_setopt(m_CurlHandle, CURLOPT_POSTFIELDS, json_object_get_string(PostJSON.get()));

    if (!Storage::CurlPerform())
    {
        return false;
    }

    JSON::Object ResponseParser = JSON::NewObject(json_tokener_parse, Response.c_str());
    if (!ResponseParser)
    {
        return false;
    }

    // This is used to check if an error occured. Rewrite will actually log these.
    json_object *Error = json_object_object_get(ResponseParser.get(), "error");
    if (Error)
    {
        return false;
    }

    // Append new directory to list.
    json_object *DirectoryID = json_object_object_get(ResponseParser.get(), "id");
    if (!DirectoryID)
    {
        // Something really messed up happened.
        return false;
    }

    m_RemoteList.emplace_back(DirectoryName, json_object_get_string(DirectoryID), ParentID, 0, true);

    curl_slist_free_all(PostHeaders);

    return true;
}

bool Remote::GoogleDrive::UploadFile(std::string_view FileName, std::string_view ParentID, std::shared_ptr<Curl::UploadStruct> UploadStruct)
{
    if (!GoogleDrive::TokenIsValid() && !GoogleDrive::RefreshToken())
    {
        return false;
    }

    curl_slist *PostHeaders = curl_slist_append(PostHeaders, m_AuthorizationHeader);
    PostHeaders = curl_slist_append(PostHeaders, HEADER_CONTENT_TYPE_APP_JSON);

    char UploadURL[URL_BUFFER_SIZE] = {0};
    std::snprintf(UploadURL, URL_BUFFER_SIZE, "%s?uploadType=resumable", DRIVE_UPLOAD_URL);

    JSON::Object PostJSON = JSON::NewObject(json_object_new_object);
    if (!PostJSON)
    {
        return false;
    }

    json_object *FileNameString = json_object_new_string(FileName.data());
    json_object_object_add(PostJSON.get(), "name", FileNameString);
    if (!ParentID.empty())
    {
        json_object *ParentArray = json_object_new_array();
        json_object *ParentString = json_object_new_string(ParentID.data());
        json_object_array_add(ParentArray, ParentString);
        json_object_object_add(PostJSON.get(), ParentArray);
    }

    std::vector<std::string> Headers;
    Storage::CurlPreparePost();
    curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, PostHeaders);
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, UploadURL);
    curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERFUNCTION, Curl::WriteHeadersToString);
    curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERDATA, &Headers);
    curl_easy_setopt(m_CurlHandle, CURLOPT_POSTFIELDS, json_object_get_string(PostJSON.get()));

    if (!Storage::CurlPerform())
    {
        return false;
    }

    std::string Location;
    if (!Curl::ExtractHeaderValue(Headers, "Location", Location))
    {
        return false;
    }

    std::string Response;
    Storage::CurlPrepareUpload();
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, Location.c_str());
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, Curl::WriteDataToString);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &Response);
    curl_easy_setopt(m_CurlHandle, CURLOPT_READFUNCTION, Curl::ReadDataFromFile);
    curl_easy_setopt(m_CurlHandle, CURLOPT_READDATA, UploadStruct.get()); // There's no other way than to pass the raw pointer...

    if (!Storage::CurlPerform())
    {
        return false;
    }

    JSON::Object ResponseParser = JSON::NewObject(json_tokener_parse, Response.c_str());
    if (!ResponseParser)
    {
        // This is weird, because technically the upload could have gone off without a hitch, but if the response is funky...
        return false;
    }

    json_object *FileID = json_object_object_get(ResponseParser.get(), "id");
    json_object *FileName = json_object_object_get(ResponseParser.get(), "name");
    json_object *MimeType = json_object_object_get(ResponseParser.get(), "mimeType");
    if (!FileID || !FileName || !MimeType)
    {
        return false;
    }

    m_RemoteList.emplace_back(json_object_get_string(FileName), json_object_get_string(FileID), ParentID, UploadStruct->Offset, false);

    curl_slist_free_all(PostHeaders);

    return true;
}

bool Remote::GoogleDrive::UpdateFile(std::string_view FileID, std::shared_ptr<Curl::UploadStruct> UploadStruct)
{
    if (!GoogleDrive::TokenIsValid() && !GoogleDrive::RefreshToken())
    {
        return false;
    }

    // Create patch URL
    char PatchURL[URL_BUFFER_SIZE] = {0};
    std::snprintf(PatchURL, URL_BUFFER_SIZE, "%s/%s?uploadType=resumable", DRIVE_UPLOAD_URL, FileID.data());
    // Authorization header.
    curl_slist *PatchHeader = curl_slist_append(PostHeader, m_AuthorizationHeader);

    // Setup curl patch request.
    std::string Response;
    std::vector<std::string> Headers;
    Storage::CurlPreparePatch();
    curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, PatchHeader);
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, PatchURL);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, Curl::WriteDataToString);
    curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERFUNCTION, Curl::WriteHeadersToString);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &Response);
    curl_easy_setopt(m_CurlHandle, CURLOPT_HEADERDATA, &Headers);
    if (!Storage::CurlPerform())
    {
        return false;
    }

    std::string Location;
    if (!Curl::ExtractHeaderValue(Headers, "Location", Location))
    {
        return false;
    }

    Storage::CurlPrepareUpload();
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, Location.c_str());
    curl_easy_setopt(m_CurlHandle, CURLOPT_READFUNCTION, Curl::ReadDataFromFile);
    curl_easy_setopt(m_CurlHandle, CURLOPT_READDATA, UploadStruct.get());
    if (!Storage::CurlPerform())
    {
        return false;
    }

    // Update the file size in list according to final offset in upload struct
    auto FindFile =
        std::find(m_RemoteList.begin(), m_RemoteList.end(), [FileID](const Remote::StorageItem &Item) { return Item.GetItemID() == FileID; });
    if (FindFile == m_RemoteList.end())
    {
        // Well, isn't this awkward...
        return false;
    }

    FindFile->SetItemSize(UploadStruct->Offset);

    curl_slist_free_all(PatchHeader);

    return true;
}

bool Remote::GoogleDrive::DownloadFile(std::string_view FileID, std::shared_ptr<Curl::DownloadStruct> DownloadStruct)
{
    if (!GoogleDrive::TokenIsValid() && !GoogleDrive::RefreshToken())
    {
        return false;
    }

    char DownloadURL[URL_BUFFER_SIZE] = {0};
    std::snprintf(DownloadURL, URL_BUFFER_SIZE, "%s/%s?alt=media", DRIVE_API_URL, FileID);

    curl_slist *GetHeaders = curl_slist_append(GetHeaders, m_AuthorizationHeader);

    Storage::CurlPrepareGet();
    curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, GetHeaders);
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, DownloadURL);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, Curl::WriteDataToBuffer);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, DownloadStruct.get());
    // We need to start the writing thread before calling Storage::CurlPerform so it's running before this thread starts downloading data.
    std::thread DownloadThread(Curl::WriteDataBufferToFile, DownloadStruct);

    // This thread will now start downloading the file and reading it into the buffer part by part instead of directly to SD cause that is SLOW.
    // When it has enough, it passes it to Download thread to write while this one gets back to downloading again.
    if (!Storage::CurlPerform())
    {
        return false;
    }
    DownloadThread.join();

    curl_slist_free_all(GetHeaders);

    // I assume everything went fine.
    return true;
}

bool Remote::GoogleDrive::DeleteFile(std::string_view FileID)
{
    if (!GoogleDrive::TokenIsValid() && !GoogleDrive::RefreshToken())
    {
        return false;
    }

    char DeleteURL[URL_BUFFER_SIZE] = {0};
    std::snprintf(DeleteURL, URL_BUFFER_SIZE, "%s/%s", DRIVE_API_URL, FileID.data());

    curl_slist *DeleteHeader = curl_slist_append(DeleteHeader, m_AuthorizationHeader);

    Storage::CurlPrepareDelete();
    curl_easy_setopt(m_CurlHandle, CURLOPT_HEADER, DeleteHeader);
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, DeleteURL);

    if (!Storage::CurlPerform())
    {
        return false;
    }

    auto FindFile =
        std::find(m_RemoteList.begin(), m_RemoteList.end(), [FileID](const Remote::StorageItem &Item) { return Item.GetItemID() == FileID; });
    if (FindFile == m_RemoteList.end())
    {
        return false;
    }
    m_RemoteList.erase(FindFile);
    return true;
}

bool Remote::GoogleDrive::SignInAuthenticate(std::string &AuthCodeOut)
{
    // If this is being executed, the constructor should have been able to load everything except a token and refresh token.
    // This really shouldn't be able to happen?
    if (m_IsInitialized)
    {
        return;
    }

    // This is the full URL to use for the login page.
    char LoginURL[URL_BUFFER_SIZE] = {0};
    std::snprintf(LoginURL, URL_BUFFER_SIZE, DRIVE_LOGIN_URL, m_ClientID.c_str());

    // This is what's needed to launch the browser applet. To do: Error check this.
    WebCommonConfig WebConfig;
    WebCommonReply WebReply;
    Result WebError = webPageCreate(&WebConfig, LoginURL);
    if (R_FAILED(WebError))
    {
        return false;
    }

    WebError = webConfigSetCallbackUrl(&WebConfig, DRIVE_APPROVAL_URL);
    if (R_FAILED(WebError))
    {
        return false;
    }

    WebError = webConfigShow(&WebConfig, &WebReply);
    if (R_FAILED(WebError))
    {
        return false;
    }

    // Now we cut the authentication code out of the reply URL.
    char ReplyURL[URL_BUFFER_SIZE] = {0};
    size_t ReplyLength = 0;
    WebError = webReplyGetLastUrl(&WebReply, ReplyURL, URL_BUFFER_SIZE, &ReplyLength);
    if (R_FAILED(WebError))
    {
        return false;
    }

    // Gonna just do this using C standard stuff this time, even if C++ strings are nice.
    char *ApprovalCode = std::strstr(ReplyURL, "approvalCode") + 13;
    if (!ApprovalCode)
    {
        return false;
    }

    // This will point to the end of the code
    char *CodeEnd = std::strchr(ApprovalCode, '#');
    if (!CodeEnd)
    {
        return false;
    }

    AuthCodeOut.assign(ApprovalCode, CodeEnd - ApprovalCode);

    return true;
}

bool Remote::GoogleDrive::ExchangeAuthenticationCode(std::string_view AuthCode)
{
    // Headers needed. Still trying to figure out how to wrap this in unique_ptr...
    curl_slist *PostHeader = curl_slist_append(PostHeader, HEADER_CONTENT_TYPE_APP_JSON);
    // JSON to post. To do: Maybe just use a std::string, because json-c makes this more of a pain than it needs to be.
    JSON::Object PostJSON = JSON::NewObject(json_object_new_object);
    json_object *ClientIDString = json_object_new_string(m_ClientID.c_str());
    json_object *ClientSecretString = json_object_new_string(m_ClientSecret.c_str());
    json_object *AuthenticationCodeString = json_object_new_string(AuthCode.data());
    json_object *RedirectURIString = json_object_new_string(DRIVE_REDIRECT_URI);
    json_object *GrantTypeString = json_object_new_string(DRIVE_GRANT_TYPE);
    // Add them to Post json
    json_object_object_add(PostJSON.get(), DRIVE_JSON_KEY_CLIENT_ID, ClientIDString);
    json_object_object_add(PostJSON.get(), DRIVE_JSON_KEY_CLIENT_SECRET, ClientSecretString);
    json_object_object_add(PostJSON.get(), DRIVE_JSON_KEY_AUTH_CODE, AuthenticationCodeString);
    json_object_object_add(PostJSON.get(), DRIVE_JSON_KEY_REDIRECT_URI, RedirectURIString);
    json_object_object_add(PostJSON.get(), DRIVE_JSON_KEY_GRANT_TYPE, GrantTypeString);

    // Curl post request.
    std::string Response;   // This is the string we're reading into.
    Storage::PreparePost(); // This will setup the base of a post.
    curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, PostHeader);
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, DRIVE_TOKEN_URL);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, Curl::WriteDataToString);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &Response);
    curl_easy_setopt(m_CurlHandle, CURLOPT_POSTFIELDS, json_object_get_string(PostJSON.get()));

    if (!Storage::CurlPerform())
    {
        return false;
    }

    JSON::Object ResponseParser = JSON::NewObject(json_tokener_parse, Response.c_str());
    if (!ResponseJSON)
    {
        return false;
    }

    json_object *AccessToken = json_object_object_get(ResponseParser.get(), "access_token");
    json_object *RefreshToken = json_object_object_get(ResponseParser.get(), "refresh_token");
    if (!AccessToken || !RefreshToken)
    {
        return false;
    }
    // Set these.
    m_Token = json_object_get_string(AccessToken);
    m_RefreshToken = json_object_get_string(RefreshToken);

    // Create authorization header cause typing it every time sucks.
    std::snprintf(m_AuthorizationHeader, 0x300, HEADER_AUTHORIZATION_BEARER, m_Token.c_str());

    curl_slist_free_all(PostHeader);

    return true;
}

bool Remote::GoogleDrive::RefreshToken(void)
{
    curl_slist *PostHeader = curl_slist_append(&PostHeader, HEADER_CONTENT_TYPE_APP_JSON);

    // Assemble post json.
    JSON::Object PostJSON = JSON::NewObject(json_object_new_object);
    json_object *ClientIDString = json_object_new_string(m_ClientID.c_str());
    json_object *ClientSecretString = json_object_new_string(m_ClientSecret.c_str());
    json_object *RefreshTokenString = json_object_new_string(m_RefreshToken.c_str());
    json_object *GrantTypeString = json_object_new_string(DRIVE_GRANT_TYPE_REFRESH_TOKEN);
    json_object_object_add(PostJSON.get(), DRIVE_JSON_KEY_CLIENT_ID, ClientIDString);
    json_object_object_add(PostJSON.get(), DRIVE_JSON_KEY_CLIENT_SECRET, ClientSecretString);
    json_object_object_add(PostJSON.get(), DRIVE_JSON_KEY_REFRESH_TOKEN, RefreshTokenString);
    json_object_object_add(PostJSON.get(), DRIVE_JSON_KEY_GRANT_TYPE, GrantTypeString);

    // Curl Post
    std::string Response;
    Storage::CurlPreparePost();
    curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, PostHeader);
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, DRIVE_TOKEN_URL);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, Curl::WriteDataToString);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &Response);
    curl_easy_setopt(m_CurlHandle, CURLOPT_POSTFIELDS, json_object_get_string(PostJSON.get()));

    if (!Storage::CurlPerform())
    {
        return false;
    }

    JSON::Object ResponseParser = JSON::NewObject(json_tokener_parse, Response.c_str());
    if (!ResponseParser)
    {
        return false;
    }

    json_object *AccessToken = json_object_object_get(ResponseParser.get(), "access_token");
    if (!AccessToken)
    {
        return false;
    }

    m_Token = json_object_get_string(AccessToken);

    std::snprintf(m_AuthorizationHeader, 0x300, HEADER_AUTHORIZATION_BEARER, m_Token.c_str());

    curl_slist_free_all(PostHeader);

    return true;
}

bool Remote::GoogleDrive::TokenIsValid(void)
{
    char TokenURL[URL_BUFFER_SIZE];
    std::snprintf(TokenURL, URL_BUFFER_SIZE, DRIVE_TOKEN_VERIFY_URL, m_Token.c_str());

    std::string Response;
    Storage::PrepareGet();
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, TokenURL);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, Curl::WriteDataToString);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &Response);

    if (!Storage::CurlPerform())
    {
        return false;
    }

    JSON::Object ResponseParser = JSON::NewObject(json_tokener_parse, Response.c_str());
    if (!ResponseParser)
    {
        return false;
    }

    // I'm assuming checking for this is enough to know if the Token is still valid? It worked before.
    json_object *Error = json_object_object_get(ResponseParser.get(), "error");
    if (Error)
    {
        return false;
    }
    return true;
}

bool Remote::GoogleDrive::RequestListing(const char *ListingURL, std::string &JSONOut)
{
    curl_slist *PostHeader = curl_slist_append(PostHeader, m_AuthorizationHeader);

    Storage::CurlPrepareGet();
    curl_easy_setopt(m_CurlHandle, CURLOPT_HTTPHEADER, PostHeader);
    curl_easy_setopt(m_CurlHandle, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(m_CurlHandle, CURLOPT_URL, ListingURL);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEFUNCTION, Curl::WriteDataToString);
    curl_easy_setopt(m_CurlHandle, CURLOPT_WRITEDATA, &JSONOut);

    bool Success = Storage::CurlPerform();

    curl_slist_free_all(PostHeader);

    return Success;
}

bool Remote::GoogleDrive::ProcessListing(std::string_view ListingJSON, bool ClearListing)
{
    if (ClearListing)
    {
        m_RemoteList.clear();
    }

    JSON::Object ListingParser = JSON::NewObject(json_tokener_parse, ListingJSON.data());
    json_object *FileList = json_object_object_get(ListingParser.get(), "files");
    if (!FileList)
    {
        return false;
    }

    size_t ArrayLength = json_object_array_length(FileList);
    // I don't really know if this makes that much of a difference.
    m_RemoteList.reserve(m_RemoteList.size() + ArrayLength);
    for (size_t i = 0; i < ArrayLength; i++)
    {
        json_object *CurrentFile = json_object_array_get_idx(FileList, i);
        if (!CurrentFile)
        {
            // Gonna return here because something is really screwed up and continuing probably won't help.
            return false;
        }

        // Gonna grab these all at one shot instead of dragging out the error checking process.
        json_object *FileID = json_object_object_get(CurrentFile, "id");
        json_object *FileName = json_object_object_get(CurrentFile, "name");
        json_object *MimeType = json_object_object_get(CurrentFile, "mimeType");
        json_object *FileSize = json_object_object_get(CurrentFile, "size");
        // This still doesn't make sense to me since they can only have one parent but it's an array.
        json_object *ParentArray = json_object_object_get(CurrentFile, "parents");

        // If any are NULL, failure.
        if (!FileID || !FileName || !MimeType || !FileSize || !ParentArray)
        {
            return false;
        }

        // I still think this is stupid, but...
        json_object *ParentID = json_object_array_get_idx(ParentArray, 0);

        // I don't see the point in assigning these to string_views and then doing this even if it's more readable.
        m_RemoteList.emplace_back(json_object_get_string(FileName),
                                  json_object_get_string(FileID),
                                  json_object_get_string(ParentID),
                                  json_object_get_uint64(FileSize),
                                  std::strcmp(json_object_get_string(MimeType), MIME_TYPE_FOLDER) == 0);
    }
    return true;
}
