#include "Remote/Storage.hpp"

namespace Remote
{
    class GoogleDrive : public Remote::Storage
    {
        public:
            // This attempts to load the client and secret from the passed path.
            GoogleDrive(const char *DriveJSONPath);
            ~GoogleDrive();

            bool CreateDirectory(std::string_view DirectoryName, std::string_view ParentID);
            bool UploadFile(std::string_view FileName, std::string_view ParentID, std::shared_ptr<Curl::UploadStruct> UploadStruct);
            bool UpdateFile(std::string_view FileID, std::shared_ptr<Curl::UploadStruct> UploadStruct);
            bool DownloadFile(std::string_view FileID, std::shared_ptr<Curl::DownloadStruct> DownloadStruct);
            bool DeleteFile(std::string_view FileID);

        private:
            // Client & secret
            std::string m_ClientID;
            std::string m_ClientSecret;
            // Token and refresh token.
            std::string m_Token;
            std::string m_RefreshToken;
            // This holds the authorization header and is updated when either ExchangeAuthentication code or RefreshToken are called.
            char m_AuthorizationHeader[0x300] = {0};
            // These aren't used outside of the class.
            bool SignInAuthenticate(std::string &AuthCodeOut);
            bool ExchangeAuthenticationCode(std::string_view AuthCode);
            bool RefreshToken(void);
            bool TokenIsValid(void);
            bool RequestListing(const char *ListingURL, std::string &JSONOut);
            bool ProcessListing(std::string_view ListingJSON, bool ClearListing);
    };
} // namespace Remote
