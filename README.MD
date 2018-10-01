# JKSV

WIP tool/whatever I want for Switch.

## Info
JKSV started as a small project to test some things and get familiar with libnx. A list of what it currently can do:
1. Dump and restore save data
2. Dump system save data (Pressing all four shoulder buttons will activate this)
3. Open and explore bis storage partitions via the dev menu and Adv Mode (Press ZL + ZR + Y to activate)
    * ^This requires you to build JKSV yourself for the time being. I'm not ready to update the release yet.
4. JKSV also has a basic built in filebrowser that allows you to copy data to and from any location on your SD card to your saves. This is available in the folder selection screen.

I have a few things I would like to add that never made it to 3DS, but I don't like making promises I might not be able to keep.

# Building:
1. Requires [devkitPro](https://devkitpro.org/) and [libnx](https://github.com/switchbrew/libnx)
2. Requires switch-freetype, libpng, and libjpeg-turbo

# Credits and Thanks:
* [shared-font](https://github.com/switchbrew/switch-portlibs-examples) example by yellows8
* Authors of switch-examples for account and save mounting code.
* [Iguniisu](https://github.com/igniscitrinus) for the icon.
