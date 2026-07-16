# Phase 0 Research & Design Document: SFTP Plugin

**Feature**: `008-sftp-plugin` | **Date**: 2026-07-16 | **Spec**: [spec.md](spec.md)
**Status**: ✅ **CONFIRMED by user on 2026-07-16** (zadání §0 gate cleared — implementation may proceed)

> **This document is the analysis the zadání requires the user to confirm
> before implementation** (spec → Dependencies & Process Constraints). It
> answers: (1) the plugin contract, (2) how the FTP plugin solves the
> connection dialog, saved connections, and password storage, (3) how the
> host renders columns and file attributes, (4) what language/framework/
> dependencies are available, and (5) the SSH/SFTP library choice with
> justification. Sources: repository research across `src/plugins/shared/`
> (SDK), `src/plugins/ftp/` (~86k lines), `src/common/dep/`, build system.

---

## 1. Plugin contract (what a plugin implements, how it registers)

A Salamander plugin is a DLL renamed to `.spl` exporting two C functions
(`sftp.def`): `SalamanderPluginGetReqVer` and `SalamanderPluginEntry`.
The current SDK interface version is **104** (`spl_vers.h`,
`LAST_VERSION_OF_SALAMANDER`) — an ABI where `CFileData::Name` is
**UTF-8** (feature 004). A new plugin builds against 104; this is a
perfect fit for SFTP, whose wire protocol filenames are UTF-8.

Entry sequence (reference: `src/plugins/ftp/ftp.cpp:281-361`):

1. Version check against host, `LoadLanguageModule()` → `.slg`.
2. `GetSalamanderGeneral()`, `GetSalamanderGUI()`.
3. `SetBasicPluginData(name, FUNCTION_FILESYSTEM | FUNCTION_CONFIGURATION |
   FUNCTION_LOADSAVECONFIGURATION, version, copyright, description,
   regKeyName="SFTP", NULL, fsName="sftp")` — registers the `sftp:`
   file-system name users type into the path box.
4. `SetPluginUsesPasswordManager()` — opt into master-password events.
5. `GetPluginFSName()` to learn the actually assigned fs-name.
6. Return a static `CPluginInterface*`.

Interfaces a file-system plugin implements:

| Class (SFTP) | SDK base | Role |
|---|---|---|
| `CPluginInterface` | `CPluginInterfaceAbstract` (spl_base.h) | lifecycle: `About`, `Release`, `LoadConfiguration`/`SaveConfiguration` (registry), `Configuration` dialog, `Connect` (registers menu items + Change Drive item + icons), `Event`, `PasswordManagerEvent` |
| `CPluginInterfaceForFS` | `CPluginInterfaceForFSAbstract` (spl_fs.h) | factory: `OpenFS`/`CloseFS`, `ExecuteOnFS` (Enter key), `ExecuteChangeDriveMenuItem`, `DisconnectFS`, `ConvertPathToInternal/External` |
| `CPluginFSInterface` | `CPluginFSInterfaceAbstract` (spl_fs.h) | one live connection per panel: `ChangePath`, `ListCurrentPath`, `TryCloseOrDetach`, `Event`, `ReleaseObject`, plus optional services |
| `CPluginInterfaceForMenuExt` | `CPluginInterfaceForMenuExtAbstract` (spl_menu.h) | Plugins-menu command dispatch |
| `CSFTPListingData` | `CPluginDataInterfaceAbstract` (spl_com.h) | per-listing custom columns (Rights/Owner/Group) via `SetupView` |

Optional FS capabilities are declared by `GetSupportedServices()` returning
`FS_SERVICE_*` flags; the SFTP set maps 1:1 to spec FRs (see
`contracts/plugin-contract.md`): `COPYFROMFS`, `MOVEFROMFS`,
`COPYFROMDISKTOFS`, `MOVEFROMDISKTOFS`, `DELETE`, `QUICKRENAME`,
`CREATEDIR`, `CHANGEATTRS`, `VIEWFILE`, `SHOWINFO`, `CONTEXTMENU`,
`GETCHANGEDRIVEORDISCONNECTITEM`, `GETFSICON`, `GETNEXTDIRLINEHOTPATH`,
`COMMANDLINE`.

---

## 2. How the FTP plugin solves connection UI, saved connections, passwords

**Connection dialog**: `CConnectDlg` (`dialogs.h:331`, `dialogs1.cpp`) —
one dialog serving both "Connect to FTP Server" (Ctrl+Shift+F, Plugins
menu, and Alt+F1/F2 Change Drive item) and "Organize Bookmarks" mode. It
embeds a `CBookmarksListbox` (drag-reorder, context menu); per-server
"Advanced" options live in `CConnectAdvancedDlg`. Connecting calls
`ChangePanelPathToPluginFS(panel, fsName, "")`, which drives
`OpenFS → ChangePath → ListCurrentPath`.

**Saved connections**: `CFTPServer` struct (`ftp.h:494`) held in
`CFTPServerList` inside a global `CConfiguration Config`. Persistence is
the plugin's registry key (host passes `HKEY` + `CSalamanderRegistryAbstract*`
to `LoadConfiguration`/`SaveConfiguration`), bookmarks under subkey
`Bookmarks`, one numbered subkey per bookmark with named values
(`Name`, `Address`, `User`, `Port`, `Initial Path`, …). Survives
restarts because the host persists its registry tree on exit.

**Password storage (three tiers, host-provided)**: The FTP plugin never
stores plaintext. It uses `CSalamanderPasswordManagerAbstract`
(spl_gen.h:767): `EncryptPassword(plain, &blob, &size, encrypt)` where
`encrypt=TRUE` produces **AES-256** (key derived from the user's master
password via PBKDF2; Gladman AES vendored in `src/common/dep/crypt/`,
wired through the core `CPasswordManager`, `src/pwdmngr.cpp`) and
`encrypt=FALSE` produces a scrambled blob. Registry values: `PasswordE`
(AES) or `PasswordS` (scrambled). On master-password create/change/remove
the host calls `PasswordManagerEvent` and the plugin re-encrypts all
stored blobs (`CFTPServerList::EncryptPasswords`). **The SFTP plugin
reuses this API unchanged** for passwords *and key passphrases* → FR-005
parity is automatic. (The FTP legacy import path `Password` +
`ScramblePassword` is not needed in a new plugin.)

**Logs**: `CLogData`/`CLogs` global + `CLogsDlg` window (own thread) —
per-connection append-only text log. Pattern copies directly (FR-032).

**Operations**: `CFTPOperation` + `CFTPQueue`/`CFTPQueueItem*` +
`CFTPWorker` pool + `CFTPDiskThread` + `COperationDlg` progress window
(own thread) + "Solve error" dialog family. Resume states and speed
meter included. The *shape* is protocol-agnostic and is the model for
SFTP's transfer engine; the implementation is entangled with FTP sockets
and will be re-implemented smaller (see §7).

**chmod**: FTP already has `ChangeAttributes` → `CChangeAttrsDlg`
(`dialogs.h:949`) plus helpers `GetAttrsFromUNIXRights`/`GetUNIXRightsStr`
(`ftputils.cpp:1432/1507`) converting `rwxr-xr-x` text ↔ bits. SFTP
copies the dialog pattern and helpers; unlike FTP (`SITE CHMOD` quirks),
SFTP has a first-class `setstat(mode)` operation.

**Reuse conclusion**: reuse means **copying patterns into a sibling
plugin** — FTP compiles everything into `FTP.SPL` with FTP-specific
globals; there is no linkable shared library between plugins. Shared
compiled code is limited to the SDK support files every plugin builds
(`shared/auxtools.cpp`, `dbg.cpp`, `mhandles.cpp`, `winliblt.cpp`).
What SFTP copies: connect/bookmark dialogs, bookmark+password model,
logs subsystem, chmod dialog+helpers, operations/progress architecture,
column mechanism. What SFTP replaces: the entire FTP socket layer
(`sockets/ctrlcon/datacon`), the OpenSSL/FTPS layer (`ssl.cpp`), and the
LIST parser engine (`parser*`, server types) — SFTP receives structured
attributes, so a fixed column set replaces runtime parser configuration.

---

## 3. How the host renders columns and file attributes

Each panel row is a `CFileData` (spl_com.h:203): UTF-8 `Name`, `Ext`,
`CQuadWord Size`, `FILETIME LastWrite`, and `DWORD Attr` — **the "windows
attributes" are just `FILE_ATTRIBUTE_*` bits the plugin chooses to set**;
the built-in Attributes column (`COLUMN_ID_ATTRIBUTES`) renders them as
R/H/S/A letters. `DWORD_PTR PluginData` is plugin-owned per-row storage.

In `ListCurrentPath` the plugin fills a `CSalamanderDirectoryAbstract`
(`SetValidData(VALID_DATA_*)`, then `AddFile`/`AddDir`) and returns a
`CPluginDataInterfaceAbstract*`. Before each redraw the host calls its
`SetupView(leftPanel, CSalamanderViewAbstract* view, …)`, where the
plugin edits the column template: `InsertStandardColumn(...)` or
`InsertColumn(...)` with a `CColumn` whose `ID = COLUMN_ID_CUSTOM` and
whose `GetText` callback fills a shared transfer buffer from
`CFileData`/`PluginData` (`GetTransferVariables`). This is exactly how
the FTP plugin shows a **Rights** column (`drwxr-xr-x`) plus **Owner**
and **Group** as parser-defined text columns.

**SFTP column design** (FR-017…FR-021): fixed set — standard Name, Ext,
Size, Date, Time + custom **Rights** (symbolic, from SFTP `permissions`,
incl. type char and setuid/setgid/sticky), **Owner**, **Group** (names
from `longname`/v4 fields when available, else numeric uid/gid). Octal
mode is shown in the chmod dialog and the Information line (FR-018).
`Attr` is additionally synthesized (read-only ⇐ no write permission,
hidden ⇐ dotfile) so the classic Attributes column remains meaningful;
a plugin configuration toggle switches the default column set between
"Unix rights" and "attribute-style" views (FR-021). Symlinks set
`CFileData::IsLink`; target text is appended per convention in the
Rights/Info line and used by `GetFullName`.

---

## 4. Language, framework, and dependencies available

- **C++20 (`/std:c++latest`), MSVC v143, pure WinAPI**; MSBuild;
  UTF-8-BOM sources; clang-format enforced. License: `GPL-2.0-or-later`
  (SPDX headers repo-wide).
- **No package manager** — all third-party code is vendored source in
  `src/common/dep/`: zlib 1.2.11, bzip2, Gladman AES (`crypt/`),
  fmt 10.1.1 (vendored, currently unused), wil (header-only, patched),
  sqlite 3.28.0, pnglite, nanosvg. Two integration patterns exist:
  **(a)** compile the dep's `.c` files directly into the consumer
  (zlib/bzip2/crypt into `salamand.exe`), **(b)** a standalone DLL
  project (`sqlite.vcxproj` → `utils\sqlite.dll`).
- **There is no SSH, TLS, or general crypto library in the repo.**
  OpenSSL exists only as vendored *headers* in the FTP plugin, which
  `LoadLibrary`s OpenSSL **1.0.x** DLLs (`libeay32/ssleay32`) from
  `utils\` at runtime and cleanly degrades to plain FTP when absent.
  Windows CNG (`bcrypt.h`) is used **nowhere** yet — greenfield.
- Host services plugins can call: zlib/bzip2 (`CSalamanderZLIB/BZIP2Abstract`),
  AES+SHA1 (`CSalamanderCryptAbstract`, spl_crypt.h), password manager,
  registry, GUI toolkit (`spl_gui.h`), disk cache for F3 viewing
  (`CSalamanderForViewFileOnFSAbstract`, spl_fs.h:34; usage model in
  `ftp/fs5.cpp:345-471`).

---

## 5. Decision: SSH/SFTP library

### D1 — Library: **libssh2** (vendored source), BSD-3-Clause

| Candidate | License | Verdict |
|---|---|---|
| **libssh2 1.11.x** | BSD-3 (GPLv2-compatible) | **Chosen.** Pure C, small (~120 files trimmed), builds cleanly under MSVC, SFTP v3 (= spec baseline), keepalive API, agent API (Pageant + Windows OpenSSH named pipe), keyboard-interactive, zlib compression, 64-bit offsets. Battle-tested (curl, libgit2). Fits the repo's vendored-source pattern exactly. |
| libssh 0.11 | LGPL-2.1 | Capable (ed25519, known_hosts built-in) but **requires OpenSSL/mbedTLS/gcrypt** — no Windows-CNG backend, so it drags in a full crypto library the repo deliberately does not vendor. Rejected. |
| wolfSSH | GPLv3-only (or commercial) | GPLv3 would effectively relicense shipped binaries; also needs wolfSSL. Rejected. |
| PuTTY code (MIT) | MIT | Full-featured (native `.ppk`, ed25519, own crypto) but not a library — embedding is a WinSCP-scale fork effort; helper-process architecture (FileZilla's fzsftp) is fragile. Rejected as base; **its documented `.ppk` format informs the SHOULD-level .ppk loader**. |
| Old `winscp` plugin | proprietary RTL | Removed in 007 precisely for license/buildability. Not a source. |

### D2 — Crypto backend: **Windows CNG (`LIBSSH2_WINCNG`)**

Zero new crypto dependency, pure WinAPI (constitution IV), no runtime
DLL to go missing (avoids FTP's OpenSSL situation). CNG provides RSA,
DH/ECDH, AES, SHA-1/2 used by the transport. Alternatives: OpenSSL
(huge vendoring or fragile runtime DLLs — rejected), mbedTLS (dual
Apache-2.0/GPL-2.0-or-later, acceptable license and a viable **fallback**
if WinCNG gaps prove larger than expected — kept as Plan B, not vendored
now).

### D3 — Private-key formats (FR-003/FR-004): plugin-side key loader

CNG has **no Ed25519** and libssh2's WinCNG backend has historically
weaker key-file parsing than its OpenSSL backend. Since FR-003 makes
OpenSSH-format keys a MUST (modern `ssh-keygen` default is ed25519 in
OpenSSH container format), the plugin ships its own **key-loader
module** feeding libssh2's callback-based auth
(`libssh2_userauth_publickey` with a custom sign callback):

- **PEM RSA / PKCS#8**: parsed via libssh2/WinCNG native path (verify
  coverage in spike task; else handled by the loader via CNG imports).
- **OpenSSH container** (`-----BEGIN OPENSSH PRIVATE KEY-----`): own
  parser; passphrase KDF is bcrypt (vendored compact public-domain
  blowfish/bcrypt-KDF, ~2 files); ed25519 signing via a vendored
  compact public-domain/zlib-licensed ed25519 implementation (ref10
  family, ~4 files). RSA keys from the container are imported into CNG.
- **PuTTY `.ppk`** (SHOULD): v2 supported (SHA-1 KDF, documented
  format); v3 uses Argon2 — deferred with a clear error naming
  supported formats, matching FR-003's rejection contract.

Risk is contained: the crypto we hand-roll is *parsing + one signature
primitive from a vetted reference implementation*, not transport crypto.

### D4 — Vendoring & build integration

- `src/common/dep/libssh2/` — trimmed upstream (include/ + src/), pinned
  release noted in a `readme.txt` (manual-update pattern like sqlite).
- Compiled **in-tree into `sftp.spl`** (pattern (a), like zlib into the
  core exe) with `LIBSSH2_WINCNG`, linking `bcrypt.lib`, `crypt32.lib`,
  `ws2_32.lib`. zlib for transport compression compiles from the
  existing `dep/zlib` (stretch FR-026; off by default).
- License bookkeeping: append libssh2 + ed25519-impl blocks to
  `doc/third_party.txt`; add `doc/license/license_bsd3.txt` if absent;
  new row in `architecture/04-dependencies.md`.
- New projects: `src/plugins/sftp/vcxproj/sftp.vcxproj` +
  `lang_sftp.vcxproj` (imports shared `plugin_base.props`/`lang_base.props`;
  outputs `sftp.spl` + `english.slg`), two entries + GUIDs in
  `salamand.sln`, and the mandatory `sftp=on` line in `plugins.cfg`
  (007 policy: missing line = hard build error).

---

## 6. Design decisions (architecture)

### D5 — Session & threading model

- One `CPluginFSInterface` per panel connection owning one **SSH session**
  (libssh2 in blocking mode with `libssh2_session_set_timeout`).
  Interactive operations (`ChangePath`/`ListCurrentPath`, rename, mkdir,
  chmod) run the SFTP call with a cancellable wait window (ESC), the
  FTP plugin's interaction pattern.
- Bulk transfers (copy/move/delete trees) run in an **operation worker
  thread** with its own SSH session(s), a queue of items, and a progress
  dialog in its own thread — a simplified re-implementation of the FTP
  operations architecture (single worker in v1; the queue design leaves
  room for parallel workers, stretch FR-026).
- Keepalive via `libssh2_keepalive_config` + periodic tick; reconnect
  policy copied from FTP (`ReconnectIfNeeded` semantics): on broken
  session, re-establish + re-auth + restore working directory (FR-022/23).
- Threads start via `CSalamanderDebugAbstract::CallWithCallStack`;
  password-manager and viewer calls stay on the main thread (SDK rule).

### D6 — Host-key trust store (TOFU, clarification #3)

Registry subkey `Known Hosts` under the plugin config key: one entry per
`host:port` storing key type, full public key (base64), and cached
SHA-256 fingerprint (OpenSSH base64 format, MD5 hex shown secondarily).
Verification order: exact key match → silent; unknown → fingerprint
dialog (trust & store / connect once / abort); mismatch → prominent
warning dialog, connection refused unless explicitly accepted (FR-006).
No code path auto-accepts.

### D7 — Data mapping (SFTP → panel)

SFTP v3 `ATTRS` (size, uid/gid, permissions, atime/mtime) + `longname`
owner/group text (v3) map to `CFileData` + per-row `PluginData` struct
(mode, uid, gid, owner, group, link target). UTF-8 names pass through
unchanged (interface 104); invalid UTF-8 is sanitized lossless-visibly
(spec edge case). Directory symlinks resolved via `stat` vs `lstat` to
set `IsLink` + directory flag; recursive downloads follow file links,
skip+report directory links (clarification #4).

### D8 — Out of scope for v1 (recorded)

ssh-agent/Pageant auth, multi-prompt keyboard-interactive (2FA),
chown/chgrp, parallel transfers, compression default-on, `.ppk` v3,
proxies/jump hosts, OpenSSH `known_hosts` import, F4 edit.

---

## 7. Risks & verification spikes

| # | Risk | Mitigation / spike |
|---|---|---|
| R1 | libssh2-WinCNG key-format coverage differs from docs | **Spike S1 (first implementation task)**: build libssh2+WinCNG, test auth with RSA-PEM, RSA-OpenSSH, ed25519-OpenSSH, encrypted variants against real OpenSSH; route gaps to the key-loader (D3) |
| R2 | SFTP throughput of libssh2 (request pipelining) | Use large read/write buffers (≥256 KB) enabling libssh2 internal read-ahead; SC-007 (4 GB file) is the acceptance test |
| R3 | Huge directories (10k+ entries) block UI | Listing runs with cancellable wait window; `SetApproximateCount` preallocation; SC-007 test |
| R4 | Ed25519/bcrypt vendored code correctness | Use unmodified vetted reference implementations + test vectors in the dev test harness |
| R5 | Reconnect edge cases (drop mid-transfer) | Copy FTP's reconnect UX; resume via `libssh2_sftp_seek64` + size check (per-file granularity per spec assumption) |
| R6 | `.ppk` v3 expectation | Clear rejection message naming supported formats (FR-003) + help note (convert via PuTTYgen) |

All spec NEEDS-CLARIFICATION items were resolved in `/speckit.clarify`
(5 questions, recorded in spec.md); no open unknowns remain for planning.

## 8. Spike S1 result (2026-07-17) — libssh2 WinCNG key-format coverage

Ran `libssh2_userauth_publickey_fromfile_ex` (the exact call
`CSFTPSession::Authenticate` uses) via a standalone test
(`test/key_auth.c`) against real OpenSSH in WSL2, one key per format:

| Key file format | Result |
|---|---|
| **Classic PEM RSA** (`-----BEGIN RSA PRIVATE KEY-----`) | **works** |
| OpenSSH container, ed25519 (`-----BEGIN OPENSSH PRIVATE KEY-----`) | fails (rc −1) |
| OpenSSH container, RSA | fails (rc −1) |
| OpenSSH container, ECDSA | fails (rc −1) |

**Conclusion**: the WinCNG backend of libssh2 reads *only* classic
PKCS#1 PEM RSA private keys from file. It cannot parse the modern
OpenSSH private-key container at all — independent of the key type
inside it. This confirms research §D3's prediction and firmly scopes the
remaining User Story 5 work (tasks T051–T054): to support the keys
`ssh-keygen` produces **by default today** (OpenSSH-container, ed25519),
the plugin **must** ship its own key loader:

- OpenSSH-container parser + (for encrypted keys) bcrypt-KDF,
- ed25519 signing via a vendored reference implementation,
- RSA keys from the container imported into CNG,
- feeding libssh2 through the callback-based `libssh2_userauth_publickey`
  sign path rather than `..._fromfile`.

**Interim behavior of the current build**: key auth works for classic
PEM RSA keys only. A user with an OpenSSH-format RSA key can convert it
with `ssh-keygen -p -m PEM -f <key>`; an ed25519 key cannot be converted
to PEM and therefore requires the loader (T051–T054). Password auth is
unaffected and fully working (verified end-to-end).
