# JKSV

Multipurpose tool for Switch.

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
    * NAND Dumping
    * Ability to remove downloaded firmware updates from NAND.
    * Terminating processes by [ID](https://switchbrew.org/wiki/Title_list#System_Modules). Allowing you to dump normally unopenable system archives.
    * Mount by System Save [ID](https://switchbrew.org/wiki/Flash_Filesystem#System_Savegames). Normally used when the terminated process makes JKSV unable to rescan titles without the Switch crashing.

# Building:
1. Requires [devkitPro](https://devkitpro.org/) and [libnx](https://github.com/switchbrew/libnx)
2. Requires switch-freetype, libpng, and libjpeg-turbo

# Credits and Thanks:
* [shared-font](https://github.com/switchbrew/switch-portlibs-examples) example by yellows8
* Authors of switch-examples for account and save mounting code.
* [Iguniisu](https://github.com/igniscitrinus) for the icon.

---
r4dius fork stuff

Changed stuff to match the original switch menus :
- misc positions / sizes
- bottom buttons and font size
- button selection "block"
- "left" menu list background

Not working :
- button selection color is not correct, nor is the effect speed
- would like to resize icons to match switch "all software" size
- adapt missing stuff for dark them
- left menu list / buttons should split to 2 lines when too large
- left menu list / buttons separator should be customizable
- forgot to adapt hidden buttons overlay at bottom

Stuff I'd like to add
- would like some scrolling effect and a scollbar when list is more than 1 screen in height
- detect dark theme when app already open (?)
- popup confirmation when writing to a name that already exists
- propose save name for "new" save rather than write the all thing
 master