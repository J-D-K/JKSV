# Peer Save Transfer

This document describes the LAN-only peer save transfer added by the JKSV Peer fork. It covers the user flow, the files it creates, the configuration it stores, cancellation and error behavior, troubleshooting, known limitations, and the hardware test matrix.

The feature is a fork addition. Upstream JKSV does not contain it. The original application and credits belong to J-D-K.

## 1. Scope

The peer transfer moves save backups directly from one Nintendo Switch console to another over the local network.

- It works with the same backup formats JKSV already writes: unpacked folders and ZIP archives.
- It transfers one backup or a batch of marked backups.
- It receives a batch into the normal JKSV backup folder and can apply selected backups to a user.
- It uses a six-digit pairing code that both consoles must confirm before any save data is sent.
- It never uses Nintendo's online services, LDN, or the internet.

It is not a cloud sync feature, a background service, or a replacement for JKSV's Google Drive and WebDAV support.

## 2. Requirements

- Two Nintendo Switch consoles on the same local network.
- The same JKSV Peer build on both consoles.
- Enough free space on the receiving console for the largest backup in the batch.
- The Homebrew Menu and network access on both consoles.
- An access point that allows client-to-client traffic. Guest networks and access points with client isolation often block discovery and connections.

The transfer uses TCP port `55555` for the session and UDP port `55556` for discovery. See [PEER_PROTOCOL.md](PEER_PROTOCOL.md) for the wire format.

## 3. Opening the feature

From the main menu, open **Extras** and select **Peer transfer**. The peer screen has three options:

- **Send** - choose a local backup or a marked batch and host a transfer.
- **Receive** - scan for a host, or enter its IP address, and receive a batch.
- **Back** - return to Extras.

The header shows this console's current IP address so the other console can enter it manually when broadcast discovery is blocked.

## 4. Sending

### 4.1 Single backup

1. Select **Send**.
2. If only one backup exists, JKSV Peer starts hosting immediately.
3. If several backups exist, the selection list opens. Highlight a backup and press **A** without marking anything to send that one backup.
4. The screen shows **Waiting for a peer...** and advertises this console on UDP port `55556`.

### 4.2 Batch send

1. Select **Send** to open the backup list.
2. Press **Y** on each backup to mark or unmark it. The header shows the number of marked backups.
3. Press **A** to send every marked backup. If nothing is marked, **A** sends only the highlighted backup.
4. Press **B** to clear the marks and return to the main peer menu.

The sender processes marked backups in the order they appear in the list. It announces each backup before preparing it, compresses a selected folder to a temporary `peer_send_<tick>.zip`, and sends a keepalive while compression runs. A backup that is already a ZIP is streamed directly. The temporary ZIP is deleted after the file is sent.

### 4.3 Host wait and cancellation

- The host waits up to 30 seconds for a peer and checks for cancellation every 100 ms.
- Press **B** during a transfer to request cancellation.
- Press **+** to cancel and exit the application. The worker is joined before the process exits.
- If the connection is lost, the sender reports **Connection lost** and returns to the menu.
- If a source file cannot be read or a temporary ZIP cannot be created, the sender reports the failure, sends an error message when possible, and stops the batch.

## 5. Receiving

### 5.1 Discovery

1. Select **Receive**.
2. JKSV Peer scans for broadcast beacons for up to 3 seconds.
3. If a host is found, it connects to the first advertised peer.
4. If no host is found, it asks for an IPv4 address. The last manually entered address is pre-filled.

The manual fallback is useful when the access point blocks broadcast traffic but still allows direct connections.

### 5.2 Pairing

Both consoles must confirm the same six-digit code before save data is sent.

1. The host sends a 16-byte nonce, and the receiver replies with its own 16-byte nonce.
2. Both consoles derive the code from both nonces in the same order.
3. Both screens show the code. Compare them.
4. Press **A** on both consoles to confirm.
5. If either console cancels, the transfer stops.
6. If the codes do not match, cancel on both consoles and start again.

The code is a safety check against connecting to the wrong console. It is not a cryptographic authentication protocol. Use the feature only on a trusted local network.

### 5.3 Receiving a batch

The receiver creates the target title folder if needed, then writes each incoming file to a `.part` file. The receiver renames the `.part` file to its final name only after the sender's announced byte count matches the number of bytes received. An interrupted transfer therefore never leaves a partial file with a valid backup name.

A folder backup is compressed by the sender and arrives as `<backup name>.zip`, matching JKSV's normal exported-backup convention. A backup that is already a ZIP keeps its `.zip` name. The receiver rejects title or backup names that could escape the backup directory (a `.` or `..` name, slashes, backslashes, or embedded NUL characters).

If a file with the same name already exists, the receiver appends `_1`, `_2`, and so on before the extension until it finds a free name. Earlier backups are not overwritten.

When the batch ends, the receiver shows a completion pop-up with the number of backups it received, then opens the post-receive selection screen.

## 6. Post-receive apply

The post-receive screen only offers automatic apply for backups that meet all of these conditions:

- The backup has valid `save_meta.bin` metadata.
- The save type is an account save.
- The title is installed on the receiving console.
- The save type matches the selected user.

If no received backup meets the conditions, JKSV Peer reports that the files were saved but cannot be applied automatically. They remain in the backup folder and can be applied later with **Save Import**.

### 6.1 Selecting a user

- If exactly one compatible user exists, the picker resolves immediately.
- If several compatible users exist, a user picker is shown.
- Press **A** to choose a user or **B** to cancel.

### 6.2 Selecting backups

- Press **Y** to mark or unmark a backup.
- Only one backup per game can stay marked. Marking a second backup for the same title unmarks the previous one.
- Press **A** to apply the marked backups. If nothing is marked, **A** applies the highlighted backup.
- Press **B** to return without applying.

### 6.3 Overwrite confirmation

If any selected backup would overwrite an existing save for the chosen user, JKSV Peer asks for confirmation and reports how many selected backups would overwrite. The batch task applies the files in order and reports how many succeeded and how many failed.

The received files are kept after apply, so the user can restore or re-apply them later.

## 7. Folder layout and naming

Backups are stored under the configured JKSV working directory, which is normally the JKSV backup folder on the SD card. The layout is the same as a normal JKSV backup:

```
<working directory>/
  <title name>/
    <backup name>/
      save_meta.bin
      ...
    <backup name>.zip
```

A received backup keeps the sender's title folder and backup name. Folder backups are received as ZIP archives. If the name already exists, the receiver adds a numeric suffix:

```
<backup name>.zip
<backup name>_1.zip
<backup name>_2.zip
```

The receiver uses a sibling `.part` file while writing and removes it on cancellation, disconnect, size mismatch, or rename failure.

## 8. Configuration

The peer feature stores one key:

- `PeerLastIP` - the last manually entered host IP address. It is saved after a manual attempt and pre-filled the next time.

The fork does not use `PeerTransport`, `PeerRequirePairing`, `PeerPort`, or `PeerDiscoveryPort`. The transport is always LAN, pairing is always required, and the ports are compile-time constants in `include/peer/Protocol.hpp`.

The existing `JKSV.log` remains the only runtime log. The old peer-specific debug log (`peer_debug.txt`) was removed.

## 9. Cancellation and errors

| Situation | Behavior |
| --- | --- |
| **B** during send | The worker checks the cancel flag between chunks, sends a best-effort `CANCELLED` message, closes the connection, and reports **Transfer cancelled**. |
| **B** during receive | The receive loop stops, deletes the active `.part` file, closes the connection, and reports **Transfer cancelled**. |
| **+** while waiting or transferring | The cancel flag is set, the worker is joined, and the application exits. |
| Peer disconnects | The other side reports **Connection lost** or **Receive incomplete** and returns to the menu. |
| Pairing timeout | The pairing confirmation waits up to 60 seconds. If it expires, the transfer reports **Pairing cancelled**. |
| No peers found | The receiver offers the manual IP prompt. |
| Invalid backup metadata | The file is kept, but it is not offered for automatic apply. |
| Title not installed | The file is kept, but it is not offered for automatic apply. |
| Wrong save type for the user | The file is kept, but it is not offered for automatic apply. |

## 10. Troubleshooting

- **The consoles cannot find each other.** Confirm they are on the same subnet, the access point allows client-to-client traffic, and UDP port `55556` is not blocked. Use the manual IP path as a fallback.
- **The manual connection fails.** Confirm the host is on the Send screen, the IP address is correct, and TCP port `55555` is reachable.
- **The pairing code differs.** Cancel on both consoles and start again. A stale session or a wrong host produces different nonces.
- **The transfer stalls.** Cancel with **B**, confirm the network is stable, and retry. Very large batches take longer because each folder is compressed before it is streamed.
- **A received backup cannot be applied.** Check that the game is installed, that the save type matches the selected user, and that the backup metadata is valid. Apply it manually with Save Import if necessary.
- **The SD card is full.** Free space and retry. The receiver writes a `.part` file and then renames it, so it needs room for the incoming file plus any temporary data.

## 11. Known limitations

- LAN only. There is no internet, relay, LDN, or cloud path.
- No encryption. Save data is sent in the clear on the local network.
- The pairing code is a confirmation check, not a cryptographic authentication protocol.
- Only account saves for installed titles can be applied automatically. Other received files must be applied with Save Import.
- The sender processes backups sequentially; there is no parallel transfer.
- The receiver keeps the first advertised peer and does not offer a peer picker when several hosts are present.
- The manual IP path accepts IPv4 addresses only.
- The protocol has no version negotiation. Both consoles must run the same build or a compatible protocol version.

## 12. Hardware test matrix

Run the full matrix on two consoles before publishing a build. Record the firmware versions and the build identifier.

| Case | Steps | Expected |
| --- | --- | --- |
| Send one | Send one backup from A to B. | B receives a valid backup. |
| Send batch | Mark several backups on A and send. | B receives every file in order. |
| Receive batch | Receive a batch on B. | B shows a completion pop-up and the post-receive screen lists the backups. |
| Apply subset | Mark some received backups and apply. | Only the marked backups are applied. |
| One per game | Send two backups for the same game. | The receive screen keeps one marked per game. |
| Cancel send | Press B during a send. | Both sides stop; no valid `.part` file remains. |
| Cancel receive | Press B during a receive. | The `.part` file is removed and the screen returns. |
| Disconnect | Disconnect one console during a transfer. | The other console reports the loss and returns. |
| Exit while waiting | Press + while the host waits. | The host exits cleanly and the worker joins. |
| No debug file | Complete a transfer. | No `peer_debug.txt` is created. |
| Manual IP | Block broadcast or use a different discovery path, then enter the host IP. | The connection succeeds with the same pairing flow. |
| Single user | Use a console with one compatible user. | The post-receive screen skips the picker and uses that user. |

## 13. Build and protocol references

- Build instructions and release checks: [../BUILDING.md](../BUILDING.md)
- Wire protocol: [PEER_PROTOCOL.md](PEER_PROTOCOL.md)
- String indices: [PEER_STRINGS.md](PEER_STRINGS.md)
