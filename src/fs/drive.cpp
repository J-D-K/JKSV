#include "fs.h"
#include "drive.h"
#include "cfg.h"
#include "ui.h"

drive::gd *fs::gDrive = NULL;
std::string fs::jksvDriveID;

void fs::driveInit()
{
    if(cfg::driveClientID.empty() || cfg::driveClientSecret.empty())
        return;

    bool refreshed = false, exchanged = false;
    fs::gDrive = new drive::gd;
    fs::gDrive->setClientID(cfg::driveClientID);
    fs::gDrive->setClientSecret(cfg::driveClientSecret);
    if(!cfg::driveRefreshToken.empty())
    {
        fs::gDrive->setRefreshToken(cfg::driveRefreshToken);
        refreshed = fs::gDrive->refreshToken();
    }

    if(!refreshed)
    {
        std::string authCode = driveSignInGetAuthCode();
        exchanged = fs::gDrive->exhangeAuthCode(authCode);
    }

    if(fs::gDrive->hasToken())
    {
        if(exchanged)
        {
            cfg::driveRefreshToken = fs::gDrive->getRefreshToken();
            cfg::saveConfig();
        }

        fs::gDrive->driveListInit("");

        if(!fs::gDrive->dirExists(JKSV_DRIVE_FOLDER))
            fs::gDrive->createDir(JKSV_DRIVE_FOLDER, "");

        jksvDriveID = fs::gDrive->getDirID(JKSV_DRIVE_FOLDER);

        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popDriveStarted", 0));
    }
    else
    {
        delete fs::gDrive;
        fs::gDrive = NULL;
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popDriveFailed", 0));
    }
}

void fs::driveExit()
{
    if(fs::gDrive)
        delete gDrive;
}

std::string fs::driveSignInGetAuthCode()
{
    std::string url = "https://accounts.google.com/o/oauth2/v2/auth?client_id=" + cfg::driveClientID + "&redirect_uri=urn:ietf:wg:oauth:2.0:oob:auto&response_type=code&scope=https://www.googleapis.com/auth/drive";
    std::string replyURL;
    WebCommonConfig webCfg;
    WebCommonReply  webReply;
    webPageCreate(&webCfg, url.c_str());
    webConfigSetCallbackUrl(&webCfg, "https://accounts.google.com/o/oauth2/approval/");
    webConfigShow(&webCfg, &webReply);
    
    size_t rLength = 0;
    char replyURLCstr[0x1000];
    webReplyGetLastUrl(&webReply, replyURLCstr, 0x1000, &rLength);
    replyURL.assign(replyURLCstr);

    int unescLength = 0;
    size_t codeBegin = replyURL.find("approvalCode") + 13, codeEnd = replyURL.find_last_of('#');
    size_t codeLength = codeEnd - codeBegin;
    replyURL = replyURL.substr(codeBegin, codeLength);

    char *urlUnesc = curl_easy_unescape(NULL, replyURL.c_str(), replyURL.length(), &unescLength);
    replyURL = urlUnesc;
    curl_free(urlUnesc);

    //Finally
    return replyURL;
}