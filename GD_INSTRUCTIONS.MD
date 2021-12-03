# <center> How to use Google Drive with JKSV </center>

**Google only allows unverified apps to have up to 100 test users. Not only would this limit be filled within minutes, but each user has to manually added. People have been asking for some kind of cloud support since I wrote JKSM on 3DS and this is my way to support it while getting around Google's restrictions.**


1. Go to https://console.cloud.google.com/, if this is your first time, accept the terms and you should now have the dashboard in front of you.
2. Click `CREATE PROJECT` on the next screen.<br><center><img src="https://i.imgur.com/42Wruo4.png" /></center>
3. On the next screen name your project JKSV. Organization is not required. Click create.
4. Give it a few seconds and the project should be created. Select JKSV if it hasn't been selected already.<center><img src="https://i.imgur.com/P1lXGea.png" /></center>
5. On the left side, find APIs and Services and click Dashboard.<br><center><img src="https://i.imgur.com/Ve8NRVr.png" /></center>
6. Once the dashboard loads, click `+ENABLE APIS AND SERVICES`.<br><center><img src="https://i.imgur.com/RzB8ChU.png" /></center>
7. Scroll down a little and find Google Drive API under Google Workspace.<br><center><img src="https://i.imgur.com/cAC7h1r.png" /></center>
8. Click on it and Click Enable on the next screen.
9. On the next screen, Google should be informing you that you need to create credentials in order to use Drive. Click Create Credentials.<br><center><img src="https://i.imgur.com/gMyedT4.png" /></center>
10. Under `Which API are you using?`, find `Cloud Storage API`. Under `What data will you be accessing?`, select `User data`. Click done.
11. On the next screen, there should be a reminder to configure your OAuth Consent screen AKA your login screen. Click `CONFIGURE CONSENT SCREEN`.<br><center><img src="https://i.imgur.com/SGmUnPp.png" /></center>
12. Select external and then create.
13. Fill out the next screen. Only App name, user support email, and developer contact information are required. The rest can be left blank. Save and Continue.
14. On the next screen, click `ADD OR REMOVE SCOPES`. Find `../auth/drive` or just type `https://www.googleapis.com/auth/drive` into `Manually add scopes`. Click update at the bottom. Save and Continue.
15. Add yourself as a test user. Save and Continue.
16. `BACK TO DASHBOARD`
17. On the left under APIs and Services, click Credentials.<br><center><img src="https://i.imgur.com/iWD1GTk.png" /></center>
18. On the next screen, click `+ CREATE_CREDENTIALS`. Select `OAuth Client ID`.<br><center><img src="https://i.imgur.com/ri3tuB7.png" /></center>
19. On the next screen, under `Application Type`, select `Desktop App`. Click `Create`.
20. You now have your Client ID and Client Secret. Copy and paste these somewhere for later. Almost done!
21. You're going to need to login and get an authentication code for JKSV to exchange. This is only needed once. Afterwards, JKSV will use a refresh token so you do not have to log in every time you want to upload or download something. Replace `YOUR_CLIENT_ID_HERE` with your client ID obtained in the previous step: `https://accounts.google.com/o/oauth2/v2/auth?client_id=YOUR_CLIENT_ID_HERE&redirect_uri=urn:ietf:wg:oauth:2.0:oob&response_type=code&scope=https://www.googleapis.com/auth/drive`.
22. At the end, you will be presented with a code. You will need this for the final step.
23. On your Switch's SD card, go to JKSV's config file and open it. `SDMC:/config/JKSV/JKSV.cfg`.
24. Add these lines and save. You should be good to go now:
```
driveClientID = YOUR_CLIENT_ID_HERE
driveClientSecret = YOU_CLIENT_SECRET_HERE
driveAuthCode = YOU_CODE_FROM_STEP_22_HERE
```
