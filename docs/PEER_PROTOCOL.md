# Peer Transfer Protocol

This document describes the wire protocol used by the JKSV Peer fork. It is written for maintainers who need to debug a transfer, add a compatible client, or review the implementation.

The protocol is intentionally small. It is a LAN-only, console-to-console protocol with no internet, relay, encryption, or compression negotiation.

## 1. Transport summary

| Purpose | Transport | Port |
| --- | --- | --- |
| Discovery beacon | UDP broadcast | `55556` |
| Save session | TCP | `55555` |

The constants live in `include/peer/Protocol.hpp`.

The host binds the TCP listener on `0.0.0.0:55555`, starts a UDP broadcast thread, and waits for a connection. The receiver binds a UDP socket to `55556`, listens for beacons, then connects to the advertised TCP port.

Both consoles are Nintendo Switch systems, so the protocol uses native byte order for its headers and payload lengths. It is not portable across architectures without a byte-order change.

## 2. Discovery beacon

The host broadcasts a fixed 38-byte UDP packet to `255.255.255.255:55556` every 500 ms while it is waiting for a peer.

| Offset | Size | Field | Notes |
| --- | --- | --- | --- |
| 0 | 4 | magic | ASCII `JKSV` |
| 4 | 2 | tcp_port | TCP listener port in host byte order |
| 6 | 32 | name | Null-terminated UTF-8 display name, zero padded |

The receiver reads for up to 3 seconds with a 500 ms socket receive timeout. A packet is accepted only when it is at least 38 bytes and the magic matches `JKSV`. Peers are de-duplicated by IPv4 address.

If discovery finds nothing, the UI falls back to a manual IPv4 address. The manual path uses the same TCP session and pairing flow.

## 3. Session framing

Every TCP message is framed as:

```
+----------------+----------------+---------------------+
| uint32 type    | uint32 length  | length payload bytes|
+----------------+----------------+---------------------+
```

- `type` is one of the `peer::MessageType` values.
- `length` is the payload size in bytes.
- The payload follows immediately.
- Both fields are native-endian 32-bit unsigned integers.
- A payload may be empty. The maximum accepted payload is 1 MiB (`Session::MAX_PAYLOAD`).
- The receiver rejects an unknown message type or an oversized payload before allocating the payload buffer.

## 4. Message types

| Value | Name | Purpose |
| --- | --- | --- |
| 1 | `Hello` | Pairing nonce. Payload is 16 random bytes. |
| 2 | `List` | Announces the next backup. |
| 3 | `Get` | Reserved for a receiver-initiated request. Not used by the current sender. |
| 4 | `Data` | File chunk. The sender writes at most 4096 bytes per message. |
| 5 | `Done` | Per-file completion, or pairing confirmation with the text `CONFIRMED`. |
| 6 | `Error` | Error code string. |
| 7 | `End` | End of the batch. Payload is the decimal number of files. |
| 8 | `Busy` | Keepalive while the sender compresses a folder. Empty payload. |

## 5. Pairing

The pairing exchange happens before any `List` or `Data` message.

1. The host sends `Hello` with a 16-byte random nonce.
2. The receiver waits for the host nonce, generates its own 16-byte nonce, and sends `Hello` with it.
3. Both sides concatenate the host nonce first and the receiver nonce second.
4. Both sides derive the six-digit code:

   ```
   code = 0
   for each of the 32 bytes:
       code = (code * 31 + byte) % 1000000
   ```

5. Both screens display the code as `%06u` and wait for the user to press **A**.
6. On confirmation, each side sends `Done` with the payload `CONFIRMED`.
7. The host waits for the receiver's `Done`; the receiver waits for the host's `Done`.
8. Only after both confirmations does the sender start the batch.

The code is a confirmation check, not a cryptographic authentication protocol. A mismatch means the consoles did not exchange the same nonces, usually because one side connected to a different host or a stale session was reused. Cancel on both consoles and retry.

The pairing wait is 60 seconds (`600` iterations of 100 ms). If it expires, the transfer reports **Pairing cancelled**.

## 6. Batch sequence

The sender processes the selection sequentially. For each selected backup:

1. The sender sends `List` with this payload:

   ```
   <selected path>\n<index>/<total>
   ```

   `<selected path>` is either `<title name>` or `<title name>/<backup name>`. `<index>` is 1-based. If the sender compressed a folder backup, the announced `<backup name>` ends in `.zip` so the receiver stores the archive with the normal JKSV extension.

2. If the backup is a directory, the sender compresses its contents into a temporary `peer_send_<tick>.zip`, with entry names relative to the backup root, and sends `Busy` every five seconds until compression finishes. If it is already a ZIP, it is streamed directly.
3. The sender reads the file in 4096-byte chunks and sends each chunk as `Data`.
4. The sender sends `Done` with the decimal file size as the payload.
5. The receiver writes each `Data` payload to a `.part` file.
6. On `Done`, the receiver closes the file, parses the expected size, and compares it with the bytes received.
7. If the size matches, the receiver renames the `.part` file to a unique final name and records the backup metadata. If it does not match, the receiver deletes the `.part` file and reports **Receive incomplete**.

After the last backup:

1. The sender sends `End` with the decimal total number of files.
2. The receiver marks the batch complete and opens the post-receive screen.
3. Both sides close the connection and tear down the transport.

The sender deletes its temporary `peer_send_<tick>.zip` after each file, whether the send succeeds or fails.

## 7. Errors and cancellation

Error payloads are short ASCII strings:

| Code | Meaning |
| --- | --- |
| `ZIP_FAILED` | The sender could not create the temporary ZIP for a folder backup. |
| `READ_FAILED` | The sender could not read the source file. |
| `CANCELLED` | The sender observed the user's cancellation request. |

Cancellation is cooperative:

- The sender checks the cancel flag between 4096-byte chunks.
- The receiver checks the cancel flag in its message loop and deletes the active `.part` file.
- The UI sets the flag and joins the worker before releasing the state.
- Pressing **+** also exits the application after the worker is joined.

If a connection closes unexpectedly, `recv` returns zero or an error, the session read fails, and the UI reports **Connection lost** or **Receive incomplete**.

## 8. Timeouts

| Operation | Budget |
| --- | --- |
| Host waiting for a connection | 30 seconds, polled every 100 ms |
| Discovery scan | 3 seconds |
| Discovery socket receive | 500 ms per read |
| TCP connect | 10 seconds, cancellable every 100 ms |
| Established socket receive | 10 seconds per read; `Busy` frames reset it during compression |
| Pairing confirmation | 60 seconds |

The host wait is implemented as 300 polls of `accept_one(100)`. The old code polled with `accept_one(0)` and slept separately; the current implementation honors the timeout inside `select`.

## 9. Endianness and portability

- Session headers use native byte order.
- The discovery beacon's `tcp_port` uses host byte order.
- Discovery addresses are IPv4 only.
- There is no version field and no negotiation. Both consoles must run the same build or a compatible protocol revision.
- Adding a field to the beacon or changing a message payload is a breaking change unless both sides are updated together.

If a future port needs to run on a different architecture, convert the session header and beacon port to a fixed byte order and add a version handshake.

## 10. Security notes

- Save data is sent in the clear over TCP.
- The pairing code is a human confirmation step, not encryption or authentication.
- The protocol assumes a trusted local network.
- Do not expose the TCP port to the internet or an untrusted network.
- The receiver validates message types and payload sizes and rejects title or backup names that are `.` or `..`, contain a slash or backslash, or contain an embedded NUL, so a peer cannot escape the backup directory. It does not authenticate the peer beyond the pairing confirmation.

## 11. Source map

| Component | Files |
| --- | --- |
| Ports and timeouts | `include/peer/Protocol.hpp` |
| Framing and payload bounds | `include/peer/Session.hpp`, `source/peer/Session.cpp` |
| Discovery, TCP transport, timeouts | `include/peer/LanTransport.hpp`, `source/peer/LanTransport.cpp` |
| Pairing and batch state machine | `source/appstates/PeerState.cpp` |
| Post-receive apply | `source/appstates/ReceivedBackupsState.cpp` |
| Batch import task | `source/tasks/saveimport.cpp` |
