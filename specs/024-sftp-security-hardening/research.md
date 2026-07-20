# Research & Consolidated Findings: SFTP Security & Functional Review

**Feature**: 024-sftp-security-hardening
**Date**: 2026-07-20
**Method**: Five parallel analysis agents over the whole `src/plugins/sftp/`
plugin (crypto/transport, authentication & secrets, file-operations & path
handling, memory-safety & C++ quality, encoding/UI), plus a broad
session/auth audit, cross-checked and de-duplicated here. Each finding below was
verified against the code by at least one agent and, for the two reported bugs,
independently by ≥2 agents and by a live reproduction.

---

## R0. Live reproduction of the handshake failure (decisive)

A hardened OpenSSH server was stood up in Docker to reproduce and validate:
only an **ECDSA** host key; `KexAlgorithms ecdh-sha2-nistp256` (no legacy DH
groups); `HostKeyAlgorithms ecdsa-sha2-nistp256`; password auth on.

- **Broken-client algorithm set** (legacy `diffie-hellman-group14/16` KEX +
  `rsa-sha2-*`/`ssh-rsa` host keys — exactly what the current plugin offers):
  `Unable to negotiate ... no matching key exchange method found. Their offer:
  ecdh-sha2-nistp256`. → reproduces the user's failure (libssh2 reports this
  identical no-common-algorithm condition as *"Unable to exchange encryption
  keys"*).
- **Fixed-client algorithm set** (`ecdh-sha2-nistp256` KEX + `ecdsa-sha2-nistp256`
  host key — exactly what `LIBSSH2_ECDSA_WINCNG` adds): negotiation **succeeds**,
  reaching the password-auth stage.

This confirms the fix is both **necessary** (current set fails) and
**sufficient** (ECDSA set connects) for this class of modern/hardened server.

---

## Root-cause analyses of the two reported bugs

### RC-1 — "Unable to exchange encryption keys" (Critical)

Confirmed chain (crypto + memory agents, and libssh2 source):

1. `sftp.props:16` defines only `LIBSSH2_WINCNG`, **not** `LIBSSH2_ECDSA_WINCNG`
   → `wincng.h:88-92` sets `LIBSSH2_ECDSA = 0`; `wincng.h:74` hardcodes
   `LIBSSH2_ED25519 = 0`.
2. With both `#if LIBSSH2_ECDSA` and `#if LIBSSH2_ED25519` blocks compiled out,
   the client's whole KEX offer (`kex.c:3248-3268`) is only the
   `diffie-hellman-group*` methods, and its whole host-key offer
   (`hostkey.c:1346-1375`) is only `rsa-sha2-512/256` and `ssh-rsa`.
3. `kex_agree_methods` must agree on **both** a KEX and a host-key algorithm
   (`kex.c:3598,3675`); failure of either → `LIBSSH2_ERROR_KEX_FAILURE`
   (`kex.c:4140-4142`), which `session.c:786-792` renders as *"Unable to exchange
   encryption keys"* (distinct from the DH-computation error "Unrecoverable error
   exchanging keys" at `kex.c:4162`).
4. `session.cpp:443-445` prefixes it with `IDS_ERR_HANDSHAKE`.

A modern/hardened server that restricts KEX to curve25519/ecdh, or that presents
only ed25519/ecdsa host keys (no RSA), shares no common algorithm with the
client → the reported error.

**Fix**: define `LIBSSH2_ECDSA_WINCNG` (adds `ecdh-sha2-nistp256/384/521` KEX and
`ecdsa-sha2-nistp256/384/521` host keys — implementation already present in
`wincng.c`/`hostkey.c`/`kex.c`, requires Windows 10+ at runtime, satisfied by the
Win11 target). Also raise `WINVER`/`_WIN32_WINNT` from `0x0601` to `0x0A00` to
make the platform floor honest. **Ed25519/curve25519 is out of scope**: the
WinCNG backend contains no curve25519/ed25519 code at all (cannot be enabled by a
flag; would require a different crypto backend). ECDSA is the interoperability
floor because every server offering curve25519 also offers ecdh-sha2-nistp256 by
default. This residual limitation is documented for the user.

### RC-2 — Mojibake of the file name in the change-owner/group dialog (High)

Confirmed (encoding agent + direct read): `dialogs.cpp:507`, `OwnerGroupProc`
`WM_INITDIALOG`, sets the target label with `SetDlgItemTextA` — the ANSI setter —
while `d->Label` is a UTF-8 server name (seeded from `SFTPRealName` via
`CollectPanelItems`). The sibling chmod dialog correctly uses `SetDlgItemTextU8`
(`dialogs.cpp:379`). **Fix**: use `SetDlgItemTextU8` (the only *display* surface
still routing a server name through ANSI; all message boxes / panel columns /
wait windows already render UTF-8 via the core).

---

## Consolidated finding register

Legend — **Decision**: FIX (this feature) / DEFER (documented follow-up).

### Critical / directly reported

| ID | Sev | Location | Issue | Decision & fix |
|----|-----|----------|-------|----------------|
| CF-1 | Critical | `sftp.props:16` | No ECDSA/ecdh → handshake fails (RC-1) | **FIX**: add `LIBSSH2_ECDSA_WINCNG`; raise `WINVER`/`_WIN32_WINNT` to `0x0A00` |
| CF-2 | High | `dialogs.cpp:507` | Owner/group dialog name mojibake (RC-2) | **FIX**: `SetDlgItemTextA` → `SetDlgItemTextU8` |
| CF-3 | Low* | `lang.rc2:42,48` + `session.cpp:146-155` | `IDS_ERR_HANDSHAKE`="SSH handshake failed: **%s**" but `SetLastErrorFromSsh` already appends the libssh2 message → literal `%s` shown (exactly what the user saw); `IDS_ERR_SFTPINIT` has a doubled separator | **FIX**: drop `%s` from `IDS_ERR_HANDSHAKE`, tidy `IDS_ERR_SFTPINIT` (*user-visible, so prioritized*) |

### Security hardening (Medium)

| ID | Sev | Location | Issue | Decision & fix |
|----|-----|----------|-------|----------------|
| CF-4 | Med | `session.cpp` after `libssh2_session_init_ex` (:432) | Client offers weak algorithms (RC4/arcfour, 3des-cbc, dh-group1-sha1, group14-sha1, hmac-md5, hmac-sha1-96, ssh-rsa/SHA-1). Only negotiate against weak-only servers, but should not be offered. | **FIX**: `libssh2_session_method_pref` with modern-only lists for KEX / CIPHER_CS/SC / MAC_CS/SC / HOSTKEY, keeping interop (group14-sha256, ecdh, chacha20, aes-ctr, rsa-sha2, ecdsa) |
| CF-5 | Med | `operats.cpp` DeleteRecursive/DownloadDir/ChmodRecursive/ChownRecursive; UploadDirRecursive/DeleteLocalTree | Unbounded recursion driven by server/local directory depth with large stack frames → stack-overflow crash/DoS (~115 levels download, ~46 upload) | **FIX**: enforce a recursion-depth cap (abort with error past N) |
| CF-6 | Med | `operats.cpp:467-496` DeleteLocalTree / UploadDirRecursive | Local dir junction/symlink followed on move disk→FS → deletes the link target's contents (data loss outside moved tree) | **FIX**: detect `FILE_ATTRIBUTE_REPARSE_POINT`; don't recurse; remove the link itself |
| CF-7 | Med | `fs.cpp:354-357` GetCurrentPath; callers `fs.cpp:154-188` | Internal navigation truncated to `MAX_PATH` for remote paths >~250 chars → up-dir/enter navigates to wrong path | **FIX**: use the FS object's full `Path` (large buffer) for internal navigation instead of the MAX_PATH `GetCurrentPath` |
| CF-8 | Med | `dialogs.cpp:98,748,786` | Passwords/passphrases read via `GetDlgItemTextA` → non-ACP secrets mangled before send/store (auth *fails* for such passwords) | **FIX**: read via `GetDlgItemTextU8` (placeholder logic is ASCII, unaffected) |
| CF-9 | Med | `session.cpp:335,356-358`; `keyload.cpp:10` | Non-ASCII private-key *path* fails: `GetFileAttributesA`/`CreateFileA`/libssh2 `fromfile` (CRT `fopen`, ACP) | **FIX**: `SplU8ToWAlloc` + `GetFileAttributesW`; load the key via `CreateFileW` + `libssh2_userauth_publickey_frommemory` |
| CF-10 | Med | `keyload.cpp:35-53` | `KeyFormatSupported` returns TRUE for `kfPuTTY` though no `.ppk` parser exists → feature-017 "clear error" defeated | **FIX**: return FALSE for `kfPuTTY` with a "convert to OpenSSH format" message |
| CF-11 | Med | `dialogs.cpp:752,787` (`SFTP_SECRET_PLACEHOLDER` :658) | The `"********"` stored-secret placeholder can be treated as a typed secret (field made dirty, or `dd==NULL`) → corrupts the saved blob / sent as password | **FIX**: exclude the placeholder value from the "typed" test |
| CF-12 | Med | `fs.cpp:989-1000` CopyOrMoveFromDiskToFS | Upload target validated on host+port but **not user** → runs on the wrong-identity session | **FIX**: also compare `user` (as `IsOurPath` does) |

### Robustness / correctness (Low) — FIX (localized, low-risk)

| ID | Sev | Location | Issue | Decision & fix |
|----|-----|----------|-------|----------------|
| CF-13 | Low | `session.cpp:561,595-611` ListDir | `longEntry` never NUL-terminated after `readdir_ex`; owner/group tokenizer scans it as a C string → potential over-read | **FIX**: NUL-terminate `longEntry` after the call |
| CF-14 | Low | `fs.cpp:96-102,113`; globals `dialogs.cpp:10-11` | Decrypted secret left in `g_PendingParams`/`ConnectPlainPassword*` on the connect-cancel path | **FIX**: `WipeSecrets`/zero on every exit path |
| CF-15 | Low | `fs.cpp:39-46` DecryptInto | Secret >511 bytes silently truncated → wrong secret sent, no diagnostic | **FIX**: detect `strlen(plain) >= outSize` and fail with an error |
| CF-16 | Low | `operats.cpp:39-55` SanitizeLocalName | Windows reserved device names (`CON/NUL/COM1…`) and trailing dot/space not neutralized | **FIX**: reject/rename reserved basenames; strip trailing `.`/space |
| CF-17 | Low | `operats.cpp:187-217` DownloadOneFile | `CREATE_ALWAYS` truncates then, on cancel/error, leaves a partial file replacing the original | **FIX**: delete the freshly-created target on failure |
| CF-18 | Low | `fs.cpp:868-870`; `operats.cpp:19-33` | `MultiByteToWideChar` return ignored → `CreateFileW` on uninitialized `wpath` if conversion fails | **FIX**: check the return; abort on 0 |
| CF-19 | Low | `operats.cpp:293,306`; `listing.cpp:146-150`; `session.cpp:572,614-615`; `hostkeys.cpp:79,90-92` | `_strdup`/alloc results unchecked → `strlen(NULL)` crash under OOM; NULL host stored to registry | **FIX**: NULL-check and skip/fail the item |
| CF-20 | Low | `fs.cpp` (no passphrase prompt); `IDS_ENTERPASSPHRASE` dead | Encrypted key with no saved passphrase fails instead of prompting | **FIX**: add a passphrase prompt (reuse `ShowPasswordPrompt` with `IDS_ENTERPASSPHRASE`) |
| CF-21 | Low | `session.cpp:275-279` | If `libssh2_hostkey_hash` returns NULL the trust dialog shows `SHA256:?` yet still lets the user trust a key they cannot verify | **FIX**: fail closed when no fingerprint can be produced |
| CF-23 | Low | `fs.cpp:876` ViewFile | Function-`static char buf[64K]` transfer buffer — re-entrancy/data-race hazard | **FIX**: use a call-local buffer |
| CF-26 | Low | `fs.cpp:180,375` | Remote path built from the sanitized panel `Name` (with `?` substitution) instead of `SFTPRealName` → wrong target for invalid-UTF-8 names | **FIX**: build the remote path from `SFTPRealName` |
| CF-27 | Info | `session.cpp`/`fs.cpp:59` | `UseCompression` persisted & shown but `libssh2_session_flag(LIBSSH2_FLAG_COMPRESS)` never called (dead setting) | **FIX**: apply the flag before handshake when set |
| CF-28 | Info | `fs.cpp:225,642`; `fs.h:65` | `CurrentListing` dangles after the core releases the listing (latent UAF; write-only today) | **FIX**: null it on release |

### Deferred (documented follow-ups — out of scope or disproportionate risk)

| ID | Sev | Location | Issue | Why deferred |
|----|-----|----------|-------|-------------|
| CF-D1 | — | libssh2 WinCNG backend | No Ed25519/curve25519 | Backend has no such code; needs a crypto-backend change, not a flag. ECDSA covers the reported failure. |
| CF-D2 | Low | `operats.cpp:30-33` MakeLocalWidePath | Local paths not `\\?\`-prefixed (fail >MAX_PATH, graceful) | Cross-cutting long-path change tied to features 012–014; graceful failure, not a crash/security issue |
| CF-D3 | Low | `session.cpp:170` getaddrinfo (ANSI) | IDN/non-ASCII hostnames don't resolve | Needs punycode/`GetAddrInfoW` handling; rare; separate change |
| CF-D4 | Low | `operats.cpp`, `listing.cpp` | `lstrcpynA` can truncate mid-UTF-8 → label mojibake for pathologically long non-ASCII names | Display-only, rare; U8 setters already fall back; low value vs. churn |
| CF-D5 | Info | `session.cpp:489-512` | `Disconnect` can block ~30s on a dead transport (`Reconnect`) | `Reconnect` has no callers (dead path) |
| CF-D6 | Info | `session.cpp:725-735` SetMTime | 64-bit mtime cast to `unsigned long` (2038/2106) | Inherent SFTP-v3 protocol limit |
| CF-D7 | Info | localized `.slg` format strings | Translator-controlled strings used as printf format | Build-time validation is tooling; English `.slg` is ASCII & correct today |
| CF-D8 | Info | `sftp.cpp` no-master-password | Secrets only scrambled (not AES) without a master password | Documented platform behavior (same as FTP); correct API use |

---

## Explicitly verified SAFE (no change) — reassurance

- **Host-key verification is fail-closed** at every branch: full base64 key blob
  compared with exact `strcmp` (no fingerprint-only shortcut); unknown/changed
  keys always force the modal dialog; only explicit Trust/Once accept;
  Cancel/dialog-failure reject; "accept once" not persisted; verification
  precedes any credential send. No MITM hole.
- **No secret is ever logged**; no double-free of password-manager buffers;
  master-password mode handled symmetrically on save/load/re-encrypt for both
  bookmarks and quick-connect; `SecureZeroMemory` hygiene throughout.
- **Download traversal (zip-slip) is mitigated**: every component through
  `SanitizeLocalName`; `.`/`..` filtered; remote real-name vs. sanitized
  local-name kept separate; remote symlinks not followed on recursive
  download/delete/chmod/chown.
- **No classic stack/heap overflow** and **no server-data-as-printf-format**
  found; `chacha20-poly1305` AEAD already offered first.
- **UI consistency** (constitution VI): all dialog templates are DIALOGEX +
  DS_SHELLFONT; no owner-draw/subclassing of standard edits; no
  `ICC_STANDARD_CLASSES` misuse.

## Scope decision

Fix all Critical/High + all Medium + the localized, low-risk Low/Info items
(CF-1…CF-21, CF-23, CF-26, CF-27, CF-28). Defer CF-D1…CF-D8 with rationale
above. This keeps the change comprehensive on security/correctness while
respecting the constitution's incremental-modernization principle (touch only
the code being fixed; no adjacent refactors).
