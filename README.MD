# JKSV

Data Dump/Restore tool for Switch.

## Info
JKSV on Switch started as a small project/port to test some things and get familiar with libnx. A list of what it currently can do:
1. Dump and restore save data.
    * This includes the ability to dump and restore to/from any location on SD by pressing minus and using the Advanced Mode.
2. Dump system save data
    * Pressing all four shoulder buttons at once will rescan and include the previously unlisted system saves.
    * Dumping this data is allowed, but writing back is not.
3. Open and explore bis storage partitions via the Extras menu
    * BIS Storage is opened inside a basic filebrowser. The partition's listing is on the left. Your SD is on the right.
    * Only copying to SD and file properties work on BIS partitions. Writing to and deleting are disabled for now.
4. Misc Extras:
    * Ability to remove downloaded firmware updates from NAND.
    * Terminating processes by [ID](https://switchbrew.org/wiki/Title_list#System_Modules). Allowing you to dump normally unopenable system archives.
    * Mount by System Save [ID](https://switchbrew.org/wiki/Flash_Filesystem#System_Savegames). Normally used when the terminated process makes JKSV unable to rescan titles without the Switch crashing.
	* Mount and open RomFS of process the homebrew menu takes over (if launched as NRO) (To be re-added later)
		* Hold R while opening a game or applet with Atmosphere so the homebrew menu loads. Open JKSV and press minus and select **Mount Process RomFS**. The romfs of the app should appear in the browser along with your SD on the right.

# Building:
1. Requires [devkitPro](https://devkitpro.org/) and [libnx](https://github.com/switchbrew/libnx)
2. Requires switch-freetype, libpng, and libjpeg-turbo

# Credits and Thanks:
* [shared-font](https://github.com/switchbrew/switch-portlibs-examples) example by yellows8
* Authors of switch-examples for account and save mounting code.
* [Iguniisu](https://github.com/igniscitrinus) for the icon.
