# JKSV Rewrite
<p align="center">
  <a href="https://github.com/J-D-K/JKSV">
    <img alt="Language" src="https://img.shields.io/github/languages/top/J-D-K/JKSV?style=for-the-badge">
  </a>
  <a href="https://github.com/J-D-K/JKSV/stargazers">
    <img alt="GitHub stars" src="https://img.shields.io/github/stars/J-D-K/JKSV?style=for-the-badge">
  </a>
  <a href="https://github.com/J-D-K/JKSV/network/members">
    <img alt="GitHub forks" src="https://img.shields.io/github/forks/J-D-K/JKSV?style=for-the-badge">
  </a>
  <a href="https://github.com/J-D-K/JKSV/releases">
    <img alt="GitHub downloads" src="https://img.shields.io/github/downloads/J-D-K/JKSV/total?style=for-the-badge">
  </a>
  <a href="https://github.com/J-D-K/JKSV/releases/latest">
    <img alt="GitHub release" src="https://img.shields.io/github/v/release/J-D-K/JKSV?style=for-the-badge">
  </a>
  <a href="https://github.com/J-D-K/JKSV/blob/master/LICENSE">
    <img alt="License" src="https://img.shields.io/github/license/J-D-K/JKSV?style=for-the-badge">
  </a>
</p>

---

> **Unofficial fork:** This branch is a peer-transfer fork of [J-D-K/JKSV](https://github.com/J-D-K/JKSV). The original application, code, and credits belong to J-D-K. This fork adds LAN console-to-console save transfer and publishes as **JKSV Peer**. It is not affiliated with or endorsed by the upstream author.

<p align="center">
	If you appreciate my work, or if JKSM or JKSV has gotten you out of a jam, please consider supporting development through a donation. Any contribution is appreciated, but never required! Creating and maintaining these tools takes a lot of time and effort.
</p>
<p align="center">
	<a href='https://ko-fi.com/W7W51JF9W6' target='_blank'><img height='42' style='border:px;height:42px;' src='https://storage.ko-fi.com/cdn/kofi3.png?v=6' border='0' alt='Support me for the countless hours I poured into JKSV' /></a>
</p>

## Rewritten from the ground up:

JKSV's rewrite makes improvements on the following:
- **Clean up and modernize the codebase:** The original master branch code became a mess over time and is extremely difficult to navigate and maintain. 	
- **Fully encapsulated design:** Everything is encapsulated via interfaces. This alone makes the rewrite easier to work with and **much**  less fragile in comparison to the original.
- **Improved error logging:** The master branch did an extremely poor job at logging errors. The rewrite corrects this greatly. Each error is logged with the file, line, and column making errors and bugs easier to track down.
- **Designed with translations in mind:** Unlike the original, translations weren't a feature that were tacked on later. This also made the original difficult to maintain.
- **Use standard formats:** JKSV originally used a custom file parser for data files. This has been removed in favor of JSON using libjson-c.
- **Uses [FsLib](https://github.com/J-D-K/FsLib):**
	- All file operations are handled using FsLib. FsLib is a C++ wrapper I wrote around libnx's FS API.
	- FsLib also handles SD card redirection so libraries that depend on C standard I/O can read and write files to and from the SD card.
- **Streamlined Google Drive login.**
- **Improve WebDav Support and code.**
- **Title Cache:** Titles found on the system are cached the same way [JKSM](https://github.com/J-D-K/JKSM) was. This improves boot time greatly on higher Switch firmware versions.
	- The title cache is also **auto-invalidating**. No need to manually update it like JKSM on 3DS.
- **Modern C++ design patterns and resource management:**
	- **Polymorphism and state patterns** instead of scattered global variables and logic.
	- **Smart pointers** to enure no memory leaks can occur.
	- **C APIs wrapped in smart pointers and classes** to ensure proper cleanup.
	- **Thread-pooled task system** Reduces overhead by reusing threads for tasks rather than creating and destroying them repeatedly.
	- **SDL textures** are all wrapped and managed by a central texture manager.

JKSV's rewrite is a ground-up rewrite. It shares none of the original's code **at all**. **Everything** has been reworked and rewritten.

## Features:
- **Supports all save data types used on Nintendo Switch:**
	- **Account** (User)
	- **System**
	- **BCAT**
	- **Device / Shared**
	- **Temporary**
	- **Cache**
	- **System BCAT**
- **Export Options:**
	- Standard, unpacked folders for editors.
	- ZIP for better compatibility.
		* **NOTE: ZIP is required and enforced for cloud backup!**
- **All At Once:**
	- JKSV can backup all the save data on the system or for individual users at once.
- **Cloud Support:**
	- If configured, JKSV currently supports:
		- **Google Drive**
		- **WebDav**
	- JKSV can be configured to upload these automatically.
- **Create Save Data:**
	- JKSV can create save data for any title on your system  by **itself**. No need to start a game to import data for it.
	- JKSV can create save data for every title found on your system **at once** for a single user.
- **SVI File Support:**
	- Import and create save data for titles not installed on your system. 
		- **NOTE: These are not properly installed to the system. Use at your own discretion!**
- **Resize save data:**
	- JKSV can extend save data to any size you would like. This is useful for games such as Minecraft.
- **Cleanup Tools**
	- JKSV features functions and tools to make cleanup less of a chore compared to Nintendo's Data Management and confirmation prompts:
		- Just hold A for three seconds to:
			- Delete save data for individual games.
			- Delete all save data for a single user at once.
- **Statistics:**
	- View basic data and statistics about the save and title.
- **Customizable:**
	- JKSV features many options to customize it to your liking.
- **Safety:**
	- JKSV can require holding to complete destructive operations or be set to make quick automatic backups upon restoring backups.

## Peer Save Transfer (fork addition)

This fork adds a LAN-only console-to-console save transfer mode on top of upstream JKSV:

- **Send:** choose one backup or mark several backups and send them as a batch.
- **Receive:** scan for a host, or enter its IP address manually, then receive the batch into the normal backup folder.
- **Pairing:** both consoles display the same six-digit code and require **A** before any save data is sent.
- **Post-receive:** choose a user, apply one or more received backups, and keep one backup per game marking.
- **Cancellation:** **B** cancels a transfer; **+** exits while the host is waiting.

The transfer runs only on the local network. It does not use Nintendo's online services, LDN, or the internet.

See [docs/PEER_TRANSFER.md](docs/PEER_TRANSFER.md) for the user guide, [docs/PEER_PROTOCOL.md](docs/PEER_PROTOCOL.md) for the wire protocol, and [BUILDING.md](BUILDING.md) for build and validation instructions. The fork builds as `JKSV-Peer.nro` with the display name `JKSV Peer` and keeps `JK` as the application author.

## Remote Storage / Upload instructions.
Detailed instructions for how to use JKSV's Google Drive and WebDav features can be found **[here](https://switch.hacks.guide/homebrew/jksv.html)**.

## Building:

1. Requires [devkitPro](https://devkitpro.org/), [libnx](https://github.com/switchbrew/libnx), and Python.

2. Select Switch Development when installing or follow the instructions [here](https://devkitpro.org/wiki/Getting_Started) if you're not running Windows.

3. Install the following libraries using pacman and/or devkitpro's MSYS2 installation:
	- **switch-bzip2**
	- **switch-curl**
	- **switch-freetype**
	- **switch-harfbuzz**
	- **switch-libjpeg-turbo**
	- **switch-libjson-c**
	- **switch-libpng**
	- **switch-libwebp**
	- **switch-sdl2**
	- **switch-sdl2_image**
	- **switch-tinyxml2**
	- **switch-zlib**
4. Clone this fork (or your own fork of it) with `git clone --recurse-submodules <repository-url>` so the bundled libraries are included. Upstream JKSV is at `https://github.com/J-D-K/JKSV.git`, but it does not contain the peer transfer changes.

5. Press Y while in the Switch Homebrew menu, navigate to the directory JKSV was cloned into on your PC, and type `make -j send`. JKSV Peer should build and nxlink will attempt to send it to your Switch.

## Credits and Thanks:
* [Iguniisu](https://github.com/igniscitrinus) for the icon.
	* Uses graphics from [Twemoji](https://github.com/twitter/twemoji) covered by the [Creative Commons License](https://creativecommons.org/licenses/by/4.0/legalcode)

* [shared-font](https://github.com/switchbrew/switch-portlibs-examples) example by yellows8 for loading system font with Freetype.

* Everyone that has contributed to translating and fixing the translations for JKSV. 
