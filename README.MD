# JKSV

Data Dump/Restore tool for Switch.

<img src="https://i.imgur.com/3hAVzkQ.jpg"/>

## Info
JKSV on Switch started as a small project/port to test some things and get familiar with libnx. A list of what it currently can do:
1. Dump and restore save data.
    * This includes the ability to dump and restore to/from any location on SD by pressing minus and using the Advanced Mode.
	* Save Data can be dumped and restored directly to and from ZIP archives by enabling `Export To ZIP` in the options menus. Folders will still be detected and usable even with this enabled.
2. Dump system save data
    * Dumping this data is always enabled, but writing back needs to be enabled from the options menu. Writing to this can be very dangerous.
3. Open and explore bis storage partitions via the Extras menu
    * BIS Storage is opened inside a basic filebrowser. The partition's listing is on the left. Your SD is on the right.
    * Only copying to SD and file properties work on BIS partitions. Writing to and deleting are disabled unless enabled like system save data.
4. Misc Extras:
    * Ability to remove downloaded firmware updates from NAND. This is located in the extras menu (ZR on User selection)
    * Terminating processes by [ID](https://switchbrew.org/wiki/Title_list#System_Modules). Allowing you to dump normally unopenable system archives.
    * Mount by System Save [ID](https://switchbrew.org/wiki/Flash_Filesystem#System_Savegames). Normally used when the terminated process makes JKSV unable to rescan titles without the Switch crashing.
	* Mount and open RomFS of process the homebrew menu takes over (if launched as NRO).
		* Hold R while opening a game or applet with Atmosphere so the homebrew menu loads. Open JKSV and press minus and select **Mount Process RomFS**. The romfs of the app should appear in the browser along with your SD on the right.

## Quick Guide
**A custom path can be defined by creating "sdmc:/switch/jksv_dir.txt" and typing the path you want to use. For example, `sdmc:/switch/JKSV/` will force JKSV to create its folder in the switch homebrew folder.**

**Custom "safe titles" can be set by creating a file named `titleDefs.txt` in your JKSV directory. These are the folders on SD in which game saves are stored. For example, creating this file and adding the line `0100BFE00E9CA000 = "The Witcher 3 CE"` will set The Witcher 3 to use `[pathto]/JKSV/The Witcher 3 CE/` directory instead of its title id. These need to be ASCII or they will fail to work.**

1. User Select
	* A opens the selected user's save files. 
	* Y Dumps __all__ save data from __all users__, device save data, and BCAT save data. System save data is not included in this.
	* X Changes the UI mode to a text menu based one for people that prefer 3DS JKSM style text menus instead.
	* Minus Opens the Options menu.
	* ZR opens a small menu of extras.
	
2. Title Select
	* A Opens the title for backup and restore.
	* L and R change the current user.
	* Y Dumps all saves for the currently selected user.
	* X adds the selected title to a list of favorites that are pushed to the top of the title list/icons.
	* Minus adds the selected title to a list of ignored titles.
	* ZR __ERASES__ The selected title's save from the system. This is the same as going into settings and deleting via data management. __THIS DOES NOT DELETE JKSV's SAVE FOLDERS__.
	
3. Backup/Folder Menu
	* Minus opens file mode. File mode is a basic file browser for moving individual files and folders. This also adds the ability to restore saves from any location on your SD card.
	* Holding L or R while selecting new with A will automatically name the backup for you without opening the keyboard.
	* A creates a new backup
	    * JKSV adds some suggestions to the keyboard's dictionary such as: date strings, the current user's name **if it's safe**, and the current title or a generated abbreviation.
	* Y Restores save data from the selected folder.
	* X Deletes the selected folder.
	* ZR __ERASES__ the current save data for the title from __your system__, but leaves the archive on NAND.

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
2. Requires switch-freetype, libpng, zlib, and libjpeg-turbo

## Credits and Thanks:
* [shared-font](https://github.com/switchbrew/switch-portlibs-examples) example by yellows8
* Authors of switch-examples for account and save mounting code.
* [Iguniisu](https://github.com/igniscitrinus) for the icon.
* [Leo](https://github.com/qazrfv1234) For the Traditional Chinese translation
* [JamePeng](https://github.com/JamePeng) For the Simplified Chinese translation.
