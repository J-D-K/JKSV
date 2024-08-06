# Remote Storage

⚠️ **WARNING** ⚠️: Breaking change with Version 07.27.2024. See [here](#remote-changelog)

## <a name="gdrive"></a><center> How to use Google Drive with JKSV </center>
**USING GOOGLE DRIVE WITH JKSV CURRENTLY REQUIRES BUILDING IT YOURSELF. I AM VERY BUSY LATELY AND THINGS WILL ONLY GET FINISHED WHEN I HAVE TIME. Thanks, sorry for yelling.**

**NOTE: As of Feb 2023, JKSV now uses the JSON downloaded from Google directly instead of editing JKSV's configuration file.**

**Google only allows unverified apps to have up to 100 test users. Not only would this limit be filled within minutes, but each user has to manually added. People have been asking for some kind of cloud support since I wrote JKSM on 3DS and this is my way to support it while getting around Google's restrictions.**

**Note: Due to Google's restrictions on unverified apps, you will be required to login once every seven days. There is nothing I can do about this at the moment.**


1. Go to https://console.cloud.google.com/cloud-resource-manager, if this is your first time, accept the terms and you should now have the dashboard in front of you.
2. Click `CREATE PROJECT` on the next screen.<br><center><img src="https://i.imgur.com/9SDS2e0.png" /></center>
3. On the next screen name your project JKSV. Organization is not required. Click create.
4. Give it a few seconds and the project should be created.
5. In the top left corner of your screen, click the navigation menu to the left of where it says **Google Cloud**. Find **Enabled APIs and services**. You may need to refresh the page if it doesn't update automatically to continue. <br><center><img src="https://i.imgur.com/JhqOpgc.png" /></center>
6. **Double check at this point to make 100% sure JKSV is the active project just in case it is not.** <br><center><img src="https://i.imgur.com/U49aIcb.png" /></center>
7. Once the dashboard loads, click **+ENABLE APIS AND SERVICES**. <br><center><img src="https://i.imgur.com/qaIhjID.png" /></center>
8. Scroll down a little and find Google Drive API under Google Workspace.<br><center><img src="https://i.imgur.com/cAC7h1r.png" /></center>
9. Click on it and click Enable on the next screen.
10. On the next screen, Google should be informing you that you need to create credentials in order to use Drive. Click Create Credentials.<br><center><img src="https://i.imgur.com/CRhFXQ4.png" /></center>
11. Under **Which API are you using?**, find **Cloud Storage API**. Under **What data will you be accessing?**, select **User data**. Click **Next**. <br><center><img src="https://i.imgur.com/fiulRpn.png" /></center>
12. Fill out the following screen. **Save and continue**.
13. Click **Add or Remove Scopes**.
14. Find **.../auth/drive** in the API list, select it, and click update. **Save and Continue**.
15. At this point you should be at a section named **OAuth Client ID**. Select **Desktop app**, name it **JKSV** and click **Create**.
16. Download the credentials saved in JSON format for later. Click **Done**.
17. Next, open the navigation menu in the top left again. Go down to **APIs and Services** and click on **OAuth consent screen**.<br><center><img src="https://i.imgur.com/OrMtG1x.png" /></center>
18. Scroll down to the section named **Test users**. Add yourself as a test user. This concludes the Google side of things.<br><center><img src="https://i.imgur.com/RTV2LMZ.png" /></center>
19. Next, find the JSON file you downloaded earlier. Copy it or send it via FTP to the following folder on your SD Card: `SD:/config/JKSV/`
20. The next time you start JKSV on your Switch, you should be prompted to login to Google via the Switch's web browser. Ta-da!

## <a name="webdav"></a><center> How to use WebDav with JKSV </center>
**NOTE: If you have [GDrive](#gdrive) configured (via JSON), it takes preference over WebDav**

1. Create a file `webdav.json` with the following content:
    ```json
    {
      "origin": "https://your-webdav-server",
      "basepath": "optional-base-path",
      "username": "testuser",
      "password": "testpassword"
    }
    ```
   - `origin` (mandatory): protocol + serveraddress + (optional port), e.g. `https://your-webdav-server` or `http://localhost:8080` - **No trailing slash**
   - `basepath` (optional): e.g. `dir`, `dir/subdir` must exist beforehand - **No leading AND trailing slash**
   - `username` (optional): username, if server uses credentials
   - `password` (optional): username, if server uses credentials
2. Copy file to following folder on your card `SD:/config/JKSV/`
3. The next time you start JKSV on your Switch, you should get a popup about the Webdav status
4. If problems arise, check the log at `SD:/JKSV/log.txt`

## Remote Changelog
- **07.27.2024**: **Breaking Change**. "Unsafe" characters were removed from titlename on the remote directory. That means,
  that if you had existing safes under a title with unsafe characters, you will need to move them manually.
