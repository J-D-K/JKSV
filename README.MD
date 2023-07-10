# JKSV

JK's Save Manager Switch Edition.

<img src="https://i.imgur.com/yLcTPzt.jpg"/>

## Info
This started as a simple, straight port of my 3DS save manager I publicly released in 2016. Despite not originally wanting to take it too far, I continued working on it for fun when I can. Currently it can:
1. Dump and restore account save data.
2. Dump and restore device saves shared by all users (Games such as Animal Crossing).
3. Dump and restore BCAT Data.
4. Dump and restore cache Saves.
5. Dump system save data.
    * Dumping this data is always enabled, but writing back needs to be enabled from the options menu. Writing to this can be very dangerous.
    * Processes can be terminated from the Extras menu allowing you to open even more of these and explore more.
6. Export save data to folders like the orignal or compressed zip files to save space.
7. Upload and download save backups to [Google Drive](./REMOTE_INSTRUCTIONS.MD#gdrive) if it is configured.
8. Upload and download save backups to [WebDav](./REMOTE_INSTRUCTIONS.MD#webdav) if it is configured.
9. Create save data so the user no longer needs to boot games to import saves.
    * Titles can be rescanned from the Extras menu. For example, if you insert a game card while JKSV is open, rescanning will load and add it to the save creation menu(s).
10. Export and use SVI files to create save data for titles not installed on your system. For games that detect other game saves to unlock content.
     * SVI files are simply the title ID, NACP, and icon packed into a file. Placing them in `JKSV/svi` will load them as if they are any other game on your switch. They will appear in the save creation menus with the rest.
11. Extend save data containers to any size the user wants or automatically if the save cannot fit into the current one.
12. Delete save data from the system.
13. Reset save data as if the game was never launched.
14. Display stats and information about the game/save: Play time, launch count, title ID (TID), save ID(SID)/name of save file on user nand partition, etc.
15. Open and explore bis storage partitions via the Extras menu
    * BIS Storage is opened inside a basic filebrowser. The partition's listing is on the left. Your SD is on the right.
    * Only copying to SD and file properties work on BIS partitions. Writing to and deleting are disabled unless enabled like system save data.
16. Misc Extras:
    * Ability to remove downloaded firmware updates from NAND. This is located under Extras.
    * Terminating processes by [ID](https://switchbrew.org/wiki/Title_list#System_Modules). Allowing you to dump normally unopenable system archives.
    * Mount by System Save [ID](https://switchbrew.org/wiki/Flash_Filesystem#System_Savegames). Normally used when the terminated process makes JKSV unable to rescan titles without the Switch crashing.
    * Mount and open RomFS of process the homebrew menu takes over (if launched as NRO).
        * Hold R while opening a game or applet with Atmosphere so the homebrew menu loads. Open JKSV and press minus and select **Mount Process RomFS**. The romfs of the app should appear in the browser along with your SD on the right.

**NOTE: Some features may require building JKSV from source. I am extremely picky and only release when I am satisfied and sure things work 100% as expected.**

## Quick Guide
1. Main/User Menu
	* A Selects the currently highlighted user and allows you to browse their titles.
	* Y Dumps the save data for all users.
	* X opens the sub menu of options for the currently highlighted user:
		* Dump All for [X] dumps all of the saves for the highlighted user.
		* Create save data opens a list of games found on your switch and will allow to create save data for them without needing to start the games.
			* SVI files located in `JKSV/svi` will also be loaded and added to this list. These are to create save data for games not currently on the system. This is for games that search for other saves to unlock extra content.
			* Cache saves require a cache index number to be created. This information can be found under information for a cache save when being exported.
		* Create All Save Data creates save data for every title on your system for the selected user.
		* Delete All Save Data deletes all save data for the selected user. This is permanant.
	* Settings and Extras below.

2. Title/Game Select
	* A selects and opens the backup menu.
		* Adding `.zip` to the end of your file name will force zip regardless of whether it's enabled or not.
	* Y Favorites a title and pushes it to the top of your games.
	* L and R jump forward down your games.
	* X opens the title options menu:
		* Information displays stats about the game for the current user.
		* Blacklist adds the title to a list that is not displayed.
		* Change Output Folder changes the folder to which the game's saves are written.
		* Open in File Mode opens the save in a basic file browser.
		* Delete All Save Backups deletes all of the backups for the current title.
		* Reset Save Data wipes the save clean as if it was never run.
		* Delete Save Data deletes the save data for the title from the system. This is the same as doing it from the Switch's data management setting.
		* Extend Save Data extends the container for the current title. This is also done automatically if the save being imported is too large for the container.
			* Different games have different limits. Most games do not need this at all. A few will take advantage of a larger container, others extend theirs at times and will need larger containers than created by default.
		* Export SVI exports the data needed to create save data for the current title to `JKSV/ncap/[TID].svi`. This is just the title ID, NACP, and icon of the title. These can be used to create save data for games not installed on your system.

4. File Mode
	* A opens directories. 
	* B goes back up a folder if possible.
	* X opens a small menu of options for files and directories:
		* Copy to [X] - Copies the currently selected item to the location opened on the other panel. Selecting the first `.` will use the directory opened as root to copy.
		* Delete deletes the currently selected item.
		* Rename renames the currently selected item.
		* Make Dir creates a folder.
		* Properties gets file size and directory size.
	* ZL or ZR Change the controlled menu.

5. Settings
	* Empty Trash Bin deletes all backups inside the `_TRASH_` folder. The trash bin feature can be disabled further down.
	* Check For Updates checks github for updates to JKSV. This currently only updates the NRO release. Maybe NSP later.
	* Set JKSV Save Output Folder allows you to set where JKSV's working directory is. Files and folders should be relocated for you.
	* Edit Blacklisted Titles allows you to removed titles blacklisted from being shown.
	* Delete All Save Backups wipes JKSV's folder of all save backups.
	
5. Extras
	* SD To SD Browser opens the filebrowser with your SD open in both panels
	* BIS: [X] opens partition [X] in the filebrowser.
	* Remove Update deletes system updates downloaded from Nintendo and asks to reboot the system to get rid of the update nag.
	* Terminate Process asks for a title ID to terminate.
	* Mount System Save asks for the save ID to mount. This is for when JKSV is unable to rescan with a process terminated.
	* Rescan Titles reloads save data information. This can be used to reload after a process is terminated or when options are changed.
	* Mount Process RomFS opens the title's romfs that is taken over to launch the homebrew menu. This only works as an NRO. The NSP will only open JKSV's own RomFS.
	* Backup JKSV folder writes the entire JKSV folder to a ZIP archive and places it in your JKSV folder.

**NOTE**: Press Plus to Exit JKSV. JKSV saves all config, favorites, and the blacklist when exited. Pressing the home button and closing that way will not allow this to take place.

## Building:
1. Requires [devkitPro](https://devkitpro.org/) and [libnx](https://github.com/switchbrew/libnx)
2. `dkp-pacman -S switch-curl switch-freetype switch-libjpeg-turbo switch-tinyxml2 switch-libjson-c switch-libpng switch-libwebp switch-sdl2 switch-sdl2_gfx switch-sdl2_image switch-zlib`

## Credits and Thanks:
* [shared-font](https://github.com/switchbrew/switch-portlibs-examples) example by yellows8 for loading system font with Freetype. All other font handling code (converting to SDL2, resizing on the fly, checking for glyphs, cache, etc) is my own.
* [Iguniisu](https://github.com/igniscitrinus) for the icon.
	* Uses graphics from [Twemoji](https://github.com/twitter/twemoji) covered by the [Creative Commons License](https://creativecommons.org/licenses/by/4.0/legalcode)
* [Leo](https://github.com/qazrfv1234) For the Traditional Chinese translation
* [JamePeng](https://github.com/JamePeng) For the Simplified Chinese translation.
