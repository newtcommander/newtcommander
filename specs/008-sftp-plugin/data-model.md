# Data Model: SFTP Plugin

**Feature**: `008-sftp-plugin` | **Date**: 2026-07-16 | **Plan**: [plan.md](plan.md)

Entities from the spec mapped to concrete plugin structures. Persistence
layout details: [contracts/registry-schema.md](contracts/registry-schema.md).

## 1. Connection profile — `CSFTPServer`

Modeled on FTP's `CFTPServer` (ftp.h:494), trimmed of FTP-only fields
(passive mode, transfer mode, list command, TLS flags, server types)
and extended for SSH.

| Field | Type | Notes / validation |
|---|---|---|
| `ItemName` | string | bookmark display name; non-empty for saved bookmarks |
| `Address` | string (UTF-8) | host or IP; required |
| `Port` | int | 1–65535; **default 22** (FR-001) |
| `UserName` | string | may be empty → prompt at connect |
| `AuthMethod` | enum `{Password=0, PrivateKey=1}` | v1 set; Agent reserved for stretch |
| `EncryptedPassword` | BYTE* + size | password-manager blob (AES/scrambled); never plaintext at rest |
| `SavePassword` | BOOL | FALSE → never persisted, prompt each connect (US6-3) |
| `KeyFilePath` | string | required when `AuthMethod=PrivateKey`; existence checked at connect (US5-5) |
| `EncryptedPassphrase` | BYTE* + size | password-manager blob for key passphrase (FR-004) |
| `SavePassphrase` | BOOL | analogous to `SavePassword` |
| `InitialPath` | string (UTF-8) | optional; empty → server home (US1-1) |
| `TargetPanelPath` | string | optional local path opened in the other panel (FTP parity) |
| `KeepAliveSendEvery` / `KeepAliveStopAfter` | int (s/min) | defaults from global config (FR-022) |
| `UseCompression` | BOOL | default FALSE (stretch FR-026) |

Collection: `CSFTPServerList` (indirect array; ordered — user can
reorder in Organize Bookmarks). Quick-connect profile stored separately
(FTP `QuickConnectServer` pattern).

**Lifecycle**: created/edited in the connect dialog; persisted in
`SaveConfiguration`; secrets re-encrypted on `PasswordManagerEvent`
(master password created/changed/removed).

## 2. Server trust record — `CSFTPHostKey`

TOFU store (clarification #3: plugin-own store, not OpenSSH known_hosts).

| Field | Type | Notes |
|---|---|---|
| `Host` | string | normalized lowercase |
| `Port` | int | key identity is `(Host, Port)` — one record each |
| `KeyType` | string | e.g. `ssh-ed25519`, `rsa-sha2-256`, `ecdsa-sha2-nistp256` |
| `PublicKey` | base64 string | full public key blob — comparison is exact-blob equality, not fingerprint |
| `FingerprintSHA256` | string | cached display form `SHA256:<base64>` (OpenSSH style) |
| `Added` | FILETIME | informational |

Collection: `CSFTPHostKeyList`, persisted under `Known Hosts`.

**State transitions** (FR-006, US4):

```
unknown ──user accepts+store──▶ trusted
unknown ──user "connect once"─▶ (session-only trust, not persisted)
unknown ──user aborts─────────▶ (no record, no connection)
trusted ──server key matches──▶ trusted (silent connect)
trusted ──server key differs──▶ MISMATCH WARNING ──explicit accept──▶ trusted (replaced)
                                              └──decline──────────▶ trusted (old kept, no connection)
```

No transition happens without user interaction except the silent match.

## 3. Remote file entry — `CFileData` + `CSFTPItemData`

Standard fields go into `CFileData` (SDK, UTF-8 names, interface 104);
SFTP-specific per-row data hangs off `CFileData::PluginData`:

| `CSFTPItemData` field | Source (SFTP v3) | Used by |
|---|---|---|
| `Mode` | `ATTRS.permissions` | Rights column (`drwxr-sr-t` incl. special bits), chmod dialog, Attr synthesis |
| `Uid` / `Gid` | `ATTRS.uid/gid` | Owner/Group columns fallback (numeric) |
| `Owner` / `Group` | parsed `longname` (v3) or v4+ names | Owner/Group columns (FR-019) |
| `LinkTarget` | `readlink` | symlink display (FR-014), Enter navigation |
| `LinkIsDir` / `LinkBroken` | `stat` on target | panel type, edge cases (broken symlink) |

`CFileData` mapping: `Name` = UTF-8 name (invalid UTF-8 sanitized,
never silently corrupted — FR-010); `Size` = `ATTRS.size` (64-bit);
`LastWrite` = mtime→FILETIME (UTC); `Attr` synthesized
(`FILE_ATTRIBUTE_READONLY` ⇐ no owner write bit, `FILE_ATTRIBUTE_HIDDEN`
⇐ dotfile, directory bit from mode); `IsLink` from mode type;
`Hidden` mirrors dotfile convention.

Owned by `CSFTPListingData : CPluginDataInterfaceAbstract`, which frees
`CSFTPItemData` in `ReleasePluginData` and provides the Rights/Owner/
Group `COLUMN_ID_CUSTOM` columns in `SetupView` (toggle per FR-021).

## 4. Transfer operation — `CSFTPOperation` / `CSFTPQueueItem`

Simplified from FTP's operats model (research.md §6 D5): one operation =
one background worker thread + item queue + progress dialog.

| Structure | Fields (key) |
|---|---|
| `CSFTPOperation` | type enum `{Download, Upload, Delete, ChangeAttrs}`; connection params snapshot; `CSFTPQueue`; worker thread handle; state `{Running, Paused, Finishing, Done, Failed}`; totals (bytes/items done/all); speed meter |
| `CSFTPQueueItem` | kind `{File, DirExplore, DirCreate, DirDelete, Symlink…}`; remote path (UTF-8); local path; size; state `{Waiting, Processing, Done, Skipped, Failed}`; problem code; resume offset |

**Resume** (FR-011, per-file granularity): on restart of an interrupted
copy, if target exists and is smaller than source, offer
Resume/Overwrite/Skip; resume seeks source+target to target size
(`libssh2_sftp_seek64` / file pointer) and continues. Recursive
symlink policy per clarification #4 (file links = content, dir links =
skip + report).

## 5. Session — `CSFTPSession` (runtime only, not persisted)

Wraps one SSH+SFTP connection: socket, `LIBSSH2_SESSION*`,
`LIBSSH2_SFTP*`, negotiated server key (for trust check), auth state,
keepalive timestamps, current directory, last-error text (for FR-024
messages), and the per-connection log ID (FR-032). Owns reconnect:
`EnsureConnected()` re-dials + re-auths + restores cwd (FR-023).

## 6. Plugin configuration — `CSFTPConfig` (global)

Persisted top-level values (defaults in parentheses): connect timeout
(20 s), operation timeout (30 s), keepalive send-every (60 s) /
stop-after (30 min), connect retries (3) + delay (10 s), default
column view (`UnixRights` | `AttributeStyle`) (UnixRights, FR-021),
show octal in info line (TRUE, FR-018), resume overlap/min-size (FTP
parity), logging enabled (TRUE) + max log size + max closed-connection
logs (FR-032), last bookmark index, dialog placements, `ConfigVersion`
for upgrades.
