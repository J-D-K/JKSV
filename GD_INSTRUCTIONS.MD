# <center> How to use Google Drive with JKSV </center>

**Google only allows unverified apps to have up to 100 test users. Not only would this limit be filled within minutes, but each user has to manually added. People have been asking for some kind of cloud support since I wrote JKSM on 3DS and this is my way to support it while getting around Google's restrictions.**

**Note: Due to Google's restrictions on unverified apps, you will be required to login once every seven days. There is nothing I can do about this at the moment.**


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
20. You now have your Client ID and Client Secret. Almost done!
23. On your Switch's SD card, go to JKSV's config file and open it. `SDMC:/config/JKSV/JKSV.cfg`.
24. Add these lines and save. The next time you open JKSV, you will need to login to Google on your Switch:
```
driveClientID = YOUR_CLIENT_ID_HERE
driveClientSecret = YOU_CLIENT_SECRET_HERE
```
