#include "fs.h"
#include "rfs.h"
#include "gd.h"
#include "webdav.h"
#include "cfg.h"
#include "ui.h"

rfs::IRemoteFS *fs::rfs = NULL;
std::string fs::rfsRootID;

void fs::remoteInit()
{
    // Google Drive has priority
    driveInit();
    webDavInit();
}

void fs::remoteExit()
{
    if(rfs) {
        delete rfs;
        rfs = NULL;
    }
}

void fs::driveInit()
{
    // Already initialized?
    if (rfs)
        return;

    if(cfg::driveClientID.empty() || cfg::driveClientSecret.empty())
        return;

    bool refreshed = false, exchanged = false;
    drive::gd *gDrive = new drive::gd;
    gDrive->setClientID(cfg::driveClientID);
    gDrive->setClientSecret(cfg::driveClientSecret);
    if(!cfg::driveRefreshToken.empty())
    {
        gDrive->setRefreshToken(cfg::driveRefreshToken);
        refreshed = gDrive->refreshToken();
    }

    if(!refreshed)
    {
        std::string authCode = driveSignInGetAuthCode();
        exchanged = gDrive->exhangeAuthCode(authCode);
    }

    if(gDrive->hasToken())
    {
        if(exchanged)
        {
            cfg::driveRefreshToken = gDrive->getRefreshToken();
            cfg::saveConfig();
        }

        gDrive->driveListInit("");

        if(!gDrive->dirExists(JKSV_DRIVE_FOLDER))
            gDrive->createDir(JKSV_DRIVE_FOLDER, "");

        rfsRootID = gDrive->getDirID(JKSV_DRIVE_FOLDER);
        rfs = gDrive;
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popDriveStarted", 0));
    }
    else
    {
        delete gDrive;
        ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popDriveFailed", 0));
    }
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
    //Prevent crash if empty.
    if(strlen(replyURLCstr) == 0)
        return "";

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

void fs::webDavInit() {
    // Already initialized?
    if (rfs)
        return;

    if (cfg::webdavOrigin.empty())
        return;

    rfs::WebDav *webdav = new rfs::WebDav(cfg::webdavOrigin,
                                          cfg::webdavUser,
                                          cfg::webdavPassword);

    std::string baseId = "/" + cfg::webdavBasePath + (cfg::webdavBasePath.empty() ? "" : "/");
    rfsRootID = webdav->getDirID(JKSV_DRIVE_FOLDER, baseId);

    // check access
    if (!webdav->dirExists(JKSV_DRIVE_FOLDER, baseId)) // this could return false on auth/config related errors
    {
        if (!webdav->createDir(JKSV_DRIVE_FOLDER, baseId))
        {
            delete webdav;
            ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popWebdavFailed", 0));
            return;
        }
    }

    rfs = webdav;
    ui::showPopMessage(POP_FRAME_DEFAULT, ui::getUICString("popWebdavStarted", 0));
}