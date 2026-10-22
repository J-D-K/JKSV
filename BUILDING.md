# Building JKSV Peer

This document describes how to go from a fresh clone of this repository to a runnable **JKSV-Peer.nro** test build, how to deploy it to a console, and how to validate a release.

This repository is an unofficial peer-transfer fork of [J-D-K/JKSV](https://github.com/J-D-K/JKSV). The original application, code, and credits belong to J-D-K. The fork adds the LAN-only peer save transfer described in [docs/PEER_TRANSFER.md](docs/PEER_TRANSFER.md). The application is built with **devkitPro (devkitA64 + libnx)** and produces `JKSV-Peer.nro` plus `JKSV-Peer.nacp`. It also builds two bundled submodules, `Libraries/FsLib` and `Libraries/SDLLib`, which are defined in `.gitmodules`.

## 1. Prerequisites

- **devkitPro** with **devkitA64** and **libnx**. See the note about `403 Forbidden` in step 2 if the installer download fails.
- A **Python 3** interpreter (used by `Assets/compress_assets.py`, which `make` runs before compiling).
- **git**.
- A Nintendo Switch with the Homebrew Menu and, for network deploys, `nxlink`.

The `Makefile` aborts early if `DEVKITPRO` is not set, so make sure the toolchain is fully configured before building.

## 2. Install the toolchain (native)

1. Install devkitPro following the official guide: <https://devkitpro.org/wiki/Getting_Started>.
   - On Windows use the graphical installer; on Linux/macOS use the `dkp-pacman` bootstrap installer.
   - If `wget https://apt.devkitpro.org/install-devkitpro-pacman` returns **403 Forbidden**, Cloudflare is rejecting the default `wget` user agent. Fetch the script with a browser user agent instead, then run it:

     ```sh
     curl -fsSL -A "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120 Safari/537.36" \
       -o install-devkitpro-pacman https://apt.devkitpro.org/install-devkitpro-pacman
     chmod +x ./install-devkitpro-pacman
     sudo ./install-devkitpro-pacman
     ```

     (The installer itself and `dkp-pacman` use devkitPro's own user agent, which the servers accept; only the initial plain `wget` fetch is blocked.)
2. Export the environment variables (add them to your shell profile so they persist):

   ```sh
   export DEVKITPRO=/opt/devkitpro
   export DEVKITA64=$DEVKITPRO/devkitA64
   export PATH=$DEVKITPRO/tools/bin:$PATH
   ```

3. Install the Switch packages the project links against:

   ```sh
   dkp-pacman -S --needed \
     switch-bzip2 \
     switch-curl \
     switch-freetype \
     switch-harfbuzz \
     switch-libjpeg-turbo \
     switch-libjson-c \
     switch-libpng \
     switch-libwebp \
     switch-sdl2 \
     switch-sdl2_image \
     switch-tinyxml2 \
     switch-zlib
   ```

## 3. Get the submodules

The `Makefile` builds `Libraries/FsLib` and `Libraries/SDLLib` before the main project. From the repository root run:

```sh
git submodule update --init --recursive
```

Verify both are present:

```sh
git submodule status
```

Each line should start with a commit hash (not `-`), for example:

```
 382c7d4... Libraries/FsLib (heads/master)
 9fa1be9... Libraries/SDLLib (9fa1be9)
```

## 4. Build

From the repository root:

```sh
make -j"$(nproc)"
```

On success the build produces, among other artifacts:

- `JKSV-Peer.nro` - the homebrew binary to run on the console.
- `JKSV-Peer.nacp` - the metadata/icon file used when packaging the `.nro`.

The NACP display name is `JKSV Peer` and the author is `JK`. A clean build can be forced with:

```sh
make clean && make -j"$(nproc)"
```

`make clean` also removes the generated `romfs/Text` bundle. The next build recreates it from `Assets/Text/*.json`.

The peer code lives in `source/peer/` and `source/appstates/PeerState.cpp`, with headers in `include/peer/` and `include/appstates/`. The LAN ports are defined in `include/peer/Protocol.hpp`.

## 5. Deploy a test build

Pick whichever matches your setup:

- **nxlink over the network** (devkitPro provides `nxlink`). Put the console in the Homebrew Menu, press **Y** to start the netloader, then run:

  ```sh
  make send
  ```

  For a debug build with logging (`nxlink -s`):

  ```sh
  make debug
  ```

- **SD card**: copy `JKSV-Peer.nro` to your SD card (for example `sdmc:/switch/JKSV-Peer.nro`) and launch it from the Homebrew Menu (hbmenu).

After launching, confirm the JKSV Peer UI loads. This is the acceptance check for a test build.

## 6. Release validation

Run these checks before publishing a build or opening a pull request.

1. Validate every translation file:

   ```sh
   for file in Assets/Text/*.json; do
     python3 -m json.tool "$file" >/dev/null || exit 1
   done
   ```

2. Build from a clean tree:

   ```sh
   make clean && make -j"$(nproc)"
   ```

3. Check the artifact and metadata:

   ```sh
   ls -l JKSV-Peer.nro JKSV-Peer.nacp
   strings JKSV-Peer.nacp | grep 'JKSV Peer'
   strings -n 2 JKSV-Peer.nacp | grep -x 'JK'
   ```

4. If you keep the OpenSpec planning tree alongside this repository, run from the OpenSpec root:

   ```sh
   openspec validate --specs
   openspec validate peer-branch-publication --strict
   ```

5. Check for whitespace errors:

   ```sh
   git diff --check
   ```

6. Run the peer transfer smoke test below.

### Peer transfer smoke test

Test with two consoles on the same LAN, both running the same build. Record the result of each case.

| Case | Steps | Expected |
| --- | --- | --- |
| Send one | Send one backup from A to B. | B receives it; the file opens as a valid backup. |
| Send batch | Mark several backups on A and send. | B receives every file in order; progress updates. |
| Receive batch | Receive a batch on B. | B shows a completion pop-up and the post-receive selection screen lists the backups. |
| Apply subset | Mark some received backups and apply. | Only the marked backups are applied. |
| One per game | Send two backups for the same game. | The receive screen marks one backup per game. |
| Cancel send | Press B during a send. | Both sides stop without a partial file left behind. |
| Cancel receive | Press B during a receive. | The partial `.part` file is removed and the screen returns. |
| Disconnect | Turn off or disconnect one console during a transfer. | The other console reports the connection loss and returns. |
| Exit while waiting | Press + while the host is waiting. | The host exits cleanly and the worker joins. |
| No debug file | Complete a transfer. | No `peer_debug.txt` is created; `JKSV.log` remains the only runtime log. |

The transfer uses TCP port `55555` and UDP discovery port `55556`. Both consoles must be on the same subnet, and the access point must allow client-to-client traffic.

## 7. Troubleshooting

- **`Please set DEVKITPRO in your environment`** - `DEVKITPRO` is not exported; see step 2.
- **Missing headers/libs for a `switch-*` dependency** - re-run the `dkp-pacman -S --needed ...` command from step 2.
- **`Libraries/FsLib` or `Libraries/SDLLib` is empty** - the submodules were not initialized; see step 3.
- **`Assets/compress_assets.py` fails** - ensure `python3` (or `python`) is on `PATH`. The `Makefile` auto-detects which one to use via the `PYTHON` variable.
- **`make clean` reports a missing `romfs/Text`** - the clean target uses `rm -fr` for this directory; update to the current `Makefile` if an older tree still fails.
- **The peer is not discovered** - confirm both consoles are on the same LAN, the access point allows client-to-client traffic, and UDP port `55556` is not blocked. Use the manual IP path as a fallback.
- **The manual IP connection fails** - confirm the host is on the Send screen, the IP address is correct, and TCP port `55555` is reachable.
- **The pairing code does not match** - cancel on both consoles and start again. The code is derived from both nonces and must match on both screens.
- **A received backup cannot be applied** - the game may not be installed, the save type may not match the selected user, or the backup may be corrupt. The receive flow saves the file and reports the reason; use Save Import to apply it manually.
