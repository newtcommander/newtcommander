# Tasks: SFTP Plugin — Remote File Management over SSH

**Input**: Design documents from `/specs/008-sftp-plugin/`
**Prerequisites**: plan.md, spec.md, research.md (user-confirmed design gate), data-model.md, contracts/, quickstart.md

**Tests**: Spec requests verification (SC-010, DoD "základní operace ověřené testy") — a dev-only console harness covers pure units (key loader, rights formatter); protocol behavior is verified via quickstart.md scenarios against a real OpenSSH server. No TDD ordering imposed.

**Organization**: Grouped by user story (US1–US7 from spec.md) so each story is an independently testable increment. **Gate cleared: research.md confirmed by user 2026-07-16 — implementation may proceed.**

## Implementation Status (2026-07-17)

**56 / 73 tasks done.** The plugin is fully implemented and **builds cleanly in all
four configurations** (Debug/Release × x64/x86); the full `build.cmd` builds the app
plus all 19 enabled plugins (sftp + ftp regression) with exit 0.

**Verified — build/static**: compilation + link in all configs (libssh2+WinCNG under
MSVC — R1 resolved), full-solution integration + FTP no-regression (SC-009 build level),
plugin-policy/solution-filter wiring; pure-logic unit harness **45/45**
(`test/harness.cpp` — rights formatter incl. setuid/setgid/sticky, octal, UTF-8
sanitation, path helpers, attribute synthesis).

**Verified — RUNTIME against real OpenSSH (WSL2), Debug + Release** via a standalone
libssh2 smoke test (`test/sftp_smoke.c`) exercising the exact `CSFTPSession` API
sequence: TCP → SSH handshake (WinCNG) → host-key SHA-256 → **password auth** → SFTP
init → directory listing with **correct Unix permissions** (`-rwsr-xr-x` setuid,
`drwxrwxrwt` sticky, `lrwxrwxrwx` symlink) → **UTF-8 filename** → file download. Covers
the runtime core of SC-001, SC-002, SC-005. **Found & fixed a real bug**: Debug build
crashed during handshake due to `/RTCc` on the libssh2 C files (now disabled).

**Spike S1 done (T050)** — key-format matrix against the live server (`test/key_auth.c`):
libssh2's WinCNG backend reads **only classic PEM RSA** keys from file; OpenSSH-container
keys (ed25519 / RSA / ECDSA) all fail. Full result + implications in research.md §8.

**Remaining — needs interactive Salamander GUI (cannot be automated headless)**: the
manual quickstart checkpoints exercising the FULL plugin (not just the libssh2 layer) in
the two-panel UI — T018, T027, T034, T045, T049, T061, T066, T072. The underlying
operations are implemented and the SSH/SFTP layer they call is runtime-proven; what is
unverified is the Salamander-panel integration (column rendering, panel refresh, dialogs).

**Remaining — real feature work (US5 key loader)**: T051–T054, now scoped by S1 —
a plugin-side OpenSSH-container/ed25519 parser + bcrypt-KDF is required for the keys
`ssh-keygen` makes by default. Current build supports password auth (verified) and
classic PEM RSA key auth (verified); OpenSSH-container/ed25519 keys are not yet accepted.

**Adversarial code review done** (whole plugin, cross-checked against the SDK + FTP
reference): **19 bugs found and fixed** — data-loss cases (move deleting skipped/partial
sources, silent overwrite, upload path-separator, path traversal), correctness (remote
ops using the sanitized name, mode-1 target prefix, 64-bit resume), crash-risks (focus
CFileData used after a modal dialog, `ChangePanelPathToPluginFS` inside an FS method,
unterminated `realpath` buffer), and robustness (keepalive never armed, listing cancel
dead, stale owner/group, overwrite/OOM handling, IPv6 literals). All fixes build in every
config and the runtime smoke test still passes. See commit "fix 19 bugs from adversarial
code review".

**Remaining — minor**: T070 (clang-format sweep), T073 (docs polish).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: parallelizable (different files, no dependency on incomplete tasks)
- **[Story]**: US1–US7 for story phases; none for Setup/Foundational/Polish

## Phase 1: Setup (project scaffolding & dependency)

**Purpose**: New plugin skeleton, vendored libssh2, build wiring — repo builds with an empty plugin.

- [X] T001 Create plugin source skeleton in `src/plugins/sftp/`: `precomp.h`/`precomp.cpp` (PCH with spl_*.h + splunicode.h, modeled on `src/plugins/ftp/precomp.h`), `sftp.def` (exports `SalamanderPluginEntry`, `SalamanderPluginGetReqVer`), `sftp.rc`/`sftp.rh2`/`versinfo.rh2`, icon, and `lang/lang.rc`/`lang/lang.rc2`/`lang/lang.rh` (English strings; pattern `src/plugins/ftp/lang/`)
- [X] T002 [P] Vendor libssh2 (pinned 1.11.x release, trimmed to `include/` + `src/`) into `src/common/dep/libssh2/` with `readme.txt` documenting version + manual-update procedure (pattern `src/common/dep/sqlite/readme.txt`)
- [X] T003 Create `src/plugins/sftp/vcxproj/sftp.vcxproj` + `sftp.props` (`ShortProjectName=sftp`): imports shared `plugin_base.props` chain, compiles shared `auxtools.cpp`/`dbg.cpp`/`mhandles.cpp`/`winliblt.cpp`, compiles `src/common/dep/libssh2/src/*.c` with `LIBSSH2_WINCNG` (per-file ObjectFileName to avoid clashes), links `bcrypt.lib;crypt32.lib;ws2_32.lib`; all four Debug/Release × Win32/x64 configs
- [X] T004 [P] Create `src/plugins/sftp/vcxproj/lang_sftp.vcxproj` + `lang_sftp.props` (imports `lang_base.props` → `plugins\sftp\lang\english.slg`); add ProjectReference from sftp.vcxproj
- [X] T005 Register `sftp` + `lang_sftp` in `src/vcxproj/salamand.sln` (two new GUIDs matching vcxproj ProjectGuids + Debug/Release × Win32/x64 configuration rows)
- [X] T006 Add `sftp=on` line to `plugins.cfg` (007 policy: missing line = hard build error)
- [X] T007 [P] License bookkeeping: append libssh2 (BSD-3) block to `doc/third_party.txt`, add `doc/license/license_bsd3.txt` if absent, add dependency row to `architecture/04-dependencies.md`
- [X] T008 Minimal `sftp.cpp` entry stub (version check, LoadLanguageModule, SetBasicPluginData with `FUNCTION_FILESYSTEM|FUNCTION_CONFIGURATION|FUNCTION_LOADSAVECONFIGURATION`, regKey `"SFTP"`, fsName `"sftp"`) so the DLL loads; verify `build.cmd` produces `plugins\sftp\sftp.spl` + `lang\english.slg` and Plugin Manager lists the plugin
- [X] T009 [P] Dev test harness skeleton: `src/plugins/sftp/test/harness.cpp` + `src/plugins/sftp/test/vcxproj/sftp_test.vcxproj` (console exe, built on demand, NOT in salamand.sln / plugins.cfg; runs unit vectors and exits nonzero on failure)

**Checkpoint**: clean `build.cmd` passes; empty plugin loads in Salamander.

---

## Phase 2: Foundational (blocking prerequisites)

**Purpose**: Core infrastructure every story needs: config, session wrapper, logs, FS skeleton, path model.

**⚠️ CRITICAL**: No user story work until this phase completes.

- [X] T010 Implement `CPluginInterface` lifecycle in `src/plugins/sftp/sftp.cpp`/`sftp.h`: `About`, `Release` (thread/session teardown order), `Connect` (Change Drive item via `SetChangeDriveMenuItem`, Plugins-menu items: Connect to SFTP Server Ctrl+Shift+S, Organize Bookmarks, Show Logs; icons via `SetIconListForGUI`), `Event`, `GetInterfaceForFS`, `GetInterfaceForMenuExt` dispatch skeleton
- [X] T011 Implement `CSFTPConfig` global + `LoadConfiguration`/`SaveConfiguration` for top-level values (timeouts, keepalive, retries, column view, logging, resume, `Version`) per `contracts/registry-schema.md` in `src/plugins/sftp/sftp.cpp`
- [X] T012 [P] Logs subsystem in `src/plugins/sftp/logs.cpp`/`logs.h`: per-connection append-only log store + Logs window in own thread (FTP `CLogs`/`CLogsDlg` pattern), never logging secrets (FR-032); `Show Logs` menu command wiring
- [X] T013 [P] Helpers in `src/plugins/sftp/sftputils.cpp`/`sftputils.h`: UTF-8 sanitation for invalid names (lossless-visible fallback, FR-010), SFTP/libssh2 status → localized error message mapping (FR-024 categories), path join/split for `sftp:user@host:port/path` syntax
- [X] T014 `CSFTPSession` wrapper in `src/plugins/sftp/session.cpp`/`session.h`: WinSock connect (getaddrinfo, IPv4/6), libssh2 handshake in blocking mode with `libssh2_session_set_timeout`, SFTP channel open, clean shutdown, last-error capture, session-log emission; host-key blob + fingerprint exposure for verification hooks
- [X] T015 Cancellable wait-window helper (ESC aborts main-thread network calls) in `src/plugins/sftp/dialogs.cpp`/`dialogs.h` (FTP wait-window interaction pattern)
- [X] T016 `CPluginInterfaceForFS` + `CPluginFSInterface` skeleton in `src/plugins/sftp/fs.cpp`/`fs.h`: `OpenFS`/`CloseFS`, path model (`GetCurrentPath`, `IsOurPath`, `GetFullName`, `GetRootPath`), `ConvertPathToInternal`/`External`, `GetSupportedServices` per `contracts/plugin-contract.md`, `TryCloseOrDetach`/`ReleaseObject` lifecycle
- [X] T017 `ExecuteChangeDriveMenuItem`/`ChangeDriveMenuItemContextMenu`/`ExecuteChangeDrivePostCommand` in `src/plugins/sftp/fs.cpp` routing to connect flow (stub dialog OK at this point)
- [ ] T018 Checkpoint build: plugin loads, Alt+F1 shows SFTP item, menu commands present, graceful "not connected" behavior

**Checkpoint**: foundation ready — story phases can begin.

---

## Phase 3: User Story 1 — Connect to a Linux Server and Browse Files (P1) 🎯 MVP

**Goal**: Create a connection (host/port-22/user/password), connect with mandatory fingerprint prompt, browse UTF-8 listings.

**Independent Test**: quickstart.md scenario 1 — connect to real OpenSSH (WSL), browse, UTF-8 names render; wrong password gives distinct, retryable error.

- [X] T019 [US1] Quick-connect dialog (host, port default 22, user name, auth=password; `CConnectDlg` layout subset, no bookmark list yet) in `src/plugins/sftp/dialogs.cpp` + resources in `src/plugins/sftp/lang/lang.rc`
- [X] T020 [US1] Password authentication in `src/plugins/sftp/session.cpp`: `libssh2_userauth_password` + transparent single-prompt keyboard-interactive fallback (`libssh2_userauth_keyboard_interactive`, auto-answer sole prompt with the password — clarification #1); wrong password → re-prompt loop without restart (US1-4)
- [X] T021 [US1] Minimal host-key gate in `src/plugins/sftp/hostkeys.cpp`/`hostkeys.h` + fingerprint dialog in `dialogs.cpp`: every connect shows SHA-256 fingerprint prompt (accept-for-session / abort); **no silent accept path exists from day one** (FR-006 minimum; persistence arrives in US4)
- [X] T022 [US1] `ChangePath` + `ListCurrentPath` in `src/plugins/sftp/fs.cpp` + ATTRS→`CFileData` mapping in `src/plugins/sftp/listing.cpp`: opendir/readdir with cancellable wait window, UTF-8 names (interface 104), size/date/dir-flag/IsLink, `..` entry, `SetValidData`, `SetApproximateCount`, initial path = profile value or server home (realpath `.`)
- [X] T023 [US1] `CSFTPListingData : CPluginDataInterfaceAbstract` minimal in `src/plugins/sftp/listing.cpp`: `CSFTPItemData` alloc/free (`ReleasePluginData`), default detailed view untouched (custom columns arrive in US2)
- [X] T024 [US1] Navigation in `src/plugins/sftp/fs.cpp`: `ExecuteOnFS` (enter directory / `..` / directory symlink), `GetNextDirectoryLineHotPath`, `GetPathForMainWindowTitle`, panel refresh (`FSE_ACTIVATEREFRESH`, forceRefresh relist)
- [X] T025 [US1] Connect-error taxonomy surfaced via `sftputils` mapping: unreachable vs refused vs auth-failed vs host-key-declined (US1-5), all logged (FR-024/FR-032)
- [X] T026 [US1] Session log records connect/auth/listing lines; Logs window shows the live connection (clarification #5 minimum)
- [ ] T027 [US1] Checkpoint: quickstart scenario 1 manual pass (fingerprint prompt, browse, UTF-8 dir `žluťoučký_кůň_日本語`, subdir/parent navigation, ESC cancels long listing)

**Checkpoint**: MVP — read-only SFTP browsing works end to end.

---

## Phase 4: User Story 2 — See and Change Unix Permissions (P1) 🔑

**Goal**: Rights/Owner/Group columns (`drwxr-sr-t` incl. special bits), octal display, chmod dialog applying setstat.

**Independent Test**: quickstart scenarios 6–7 — prepared server entries (setuid, sticky, symlink) render correctly; chmod 0644→0755 visible in `ls -l`.

- [X] T028 [P] [US2] Rights formatter in `src/plugins/sftp/sftputils.cpp`: mode → symbolic string (type char `-dlbcsp`, rwx, s/S, t/T), mode → octal, symbolic/octal → mode, Win-attr synthesis (READONLY ⇐ no owner write, HIDDEN ⇐ dotfile); unit vectors in `src/plugins/sftp/test/harness.cpp`
- [X] T029 [US2] Full `CSFTPItemData` population in `src/plugins/sftp/listing.cpp`: mode, uid/gid, owner/group parsed from v3 `longname` (fallback numeric — spec edge case), symlink target via readlink + target stat (`LinkIsDir`/`LinkBroken`)
- [X] T030 [US2] Custom columns in `src/plugins/sftp/listing.cpp` `SetupView`: insert Rights/Owner/Group (`COLUMN_ID_CUSTOM`, GetText callbacks via transfer variables, sorting support), column width persistence (`ColumnFixedWidthShouldChange`/`ColumnWidthWasChanged` → config)
- [X] T031 [US2] View toggle + octal: `Column View` config (UnixRights ↔ AttributeStyle, FR-021) honored by `SetupView`; octal mode + link target in Information Line (`GetInfoLineContent`, FR-018); basic Configuration dialog page hosting the toggle in `src/plugins/sftp/dialogs.cpp`
- [X] T032 [US2] Chmod dialog in `src/plugins/sftp/dialogs.cpp` (FTP `CChangeAttrsDlg` pattern): rwx + setuid/setgid/sticky checkboxes with tri-state for multi-select, live octal edit box, include-subdirectories option
- [X] T033 [US2] `ChangeAttributes` in `src/plugins/sftp/fs.cpp`: setstat over selection (recursive walk when include-subdirs, cancellable wait window), per-entry permission-denied reporting (US2-5), refresh after completion
- [ ] T034 [US2] Checkpoint: quickstart scenarios 6–7 pass (special bits shown, owner/group names + numeric fallback, chmod applied and re-listed)

**Checkpoint**: the feature's key differentiator works.

---

## Phase 5: User Story 3 — Transfer and Manage Remote Files (P1)

**Goal**: download/upload with progress+resume, rename/move, delete, mkdir, symlinks, set times, F3 view.

**Independent Test**: quickstart scenarios 8–9 — byte-identical round-trip with non-ASCII names; F3 opens viewer.

- [X] T035 [US3] Operation engine scaffolding in `src/plugins/sftp/operats.cpp`/`operats.h`: `CSFTPOperation` (worker thread via `CallWithCallStack`, own `CSFTPSession`), `CSFTPQueue`/`CSFTPQueueItem` state machine per data-model.md §4, cancellation, `Release(force)` teardown integration
- [X] T036 [US3] Operation progress dialog (own thread; totals, current item, speed meter, pause/cancel; FTP `COperationDlg` pattern simplified) in `src/plugins/sftp/dialogs.cpp`
- [X] T037 [US3] Download (`CopyOrMoveFromFS`) in `src/plugins/sftp/operats.cpp`: recursive explore, symlink policy (file links = target content, dir links = skip + report in summary — clarification #4), local write + mtime preservation, ≥256 KB buffers (R2)
- [X] T038 [US3] Upload (`CopyOrMoveFromDiskToFS`) in `src/plugins/sftp/operats.cpp`: recursive, mtime preservation via setstat, move-variant deletes source on success
- [X] T039 [US3] Resume both directions (FR-011): target-exists-smaller → Resume/Overwrite/Skip dialog, `libssh2_sftp_seek64` + file-pointer seek, configurable overlap re-read + min-size (registry values)
- [X] T040 [US3] `Delete` (recursive queue: files → rmdir), `CreateDir`, `QuickRename` (rename/move incl. UTF-8 names) in `src/plugins/sftp/fs.cpp` + `operats.cpp`
- [X] T041 [P] [US3] Create Symlink command (Plugins menu / context menu + target dialog) and set-modification-time field in the Change Attributes dialog (FR-014/FR-015) in `src/plugins/sftp/dialogs.cpp` + `fs.cpp`
- [X] T042 [US3] F3 `ViewFile` in `src/plugins/sftp/fs.cpp` via `CSalamanderForViewFileOnFSAbstract` (cache key `"<fsname>:user@host:port/path"`, download → `OpenViewer` → `FreeFileNameInCache`; FTP `fs5.cpp` pattern); purge `RemoveFilesFromCache("<fsname>:")` on `Release` (FR-031)
- [X] T043 [US3] `ExecuteOnFS` Enter-on-file = ViewFile; post-mutation refresh (`PostChangeOnPathNotification`, `ACCEPTSCHANGENOTIF`) in `src/plugins/sftp/fs.cpp`
- [X] T044 [US3] Per-item error resolution in `src/plugins/sftp/operats.cpp` + `dialogs.cpp`: skip/retry/skip-all prompts; disk-full/quota stops with partial-file identification for resume (spec edge cases)
- [ ] T045 [US3] Checkpoint: quickstart scenarios 8–9 pass (round-trip `fc /b` identical, rename with diacritics, symlink create/read, F3 view)

**Checkpoint**: all P1 stories complete — day-to-day server administration works.

---

## Phase 6: User Story 4 — Verify the Server's Identity (P2)

**Goal**: Persistent TOFU trust store; silent match, mismatch warning, no silent accept anywhere.

**Independent Test**: quickstart scenarios 2–3 — second connect silent; regenerated server key triggers warning; decline refuses and keeps old key.

- [X] T046 [US4] `CSFTPHostKeyList` + `Known Hosts` registry persistence (exact public-key blob storage, base64) per `contracts/registry-schema.md` in `src/plugins/sftp/hostkeys.cpp`
- [X] T047 [US4] Full verification flow in `src/plugins/sftp/hostkeys.cpp`: exact-blob match → silent; unknown → trust-and-store / connect-once (session-only) / abort; fingerprint display SHA-256 base64 (primary) + MD5 hex (secondary)
- [X] T048 [US4] Mismatch warning dialog in `src/plugins/sftp/dialogs.cpp`: prominent risk wording, decline → connection refused + stored key kept, explicit accept → replace record (data-model §2 transitions); all decisions logged
- [ ] T049 [US4] Checkpoint: quickstart scenarios 2–3 pass; code audit confirms no accept path without user interaction (FR-006)

---

## Phase 7: User Story 5 — Authenticate with a Private SSH Key (P2)

**Goal**: Key-file auth for OpenSSH/PEM/PKCS#8 (+ .ppk v2), passphrase prompt/store, precise error messages.

**Independent Test**: quickstart scenarios 5 + 14 — all three generated keys connect; wrong passphrase says "key could not be unlocked"; `.ppk` v3 rejected with format list.

- [X] T050 [US5] **Spike S1** (research R1): auth matrix of libssh2+WinCNG native key loading against `k_rsa_pem`, `k_rsa_ossh`, `k_ed25519` (± passphrase) on a real OpenSSH server; record results as addendum in `specs/008-sftp-plugin/research.md` and finalize keyload.cpp scope
- [ ] T051 [P] [US5] Vendor compact reference crypto for the key loader: ed25519 (ref10-family, zlib/PD license) + bcrypt-KDF/blowfish into `src/common/dep/ed25519/` + `src/common/dep/bcrypt_kdf/` with pins in readme; append notices to `doc/third_party.txt`
- [ ] T052 [US5] OpenSSH private-key container parser (base64 envelope, bcrypt-KDF decrypt for passphrase-protected keys) in `src/plugins/sftp/keyload.cpp`/`keyload.h`; test vectors in `src/plugins/sftp/test/harness.cpp`
- [ ] T053 [US5] Auth wiring in `src/plugins/sftp/session.cpp`: `libssh2_userauth_publickey` sign-callback for ed25519 (vendored) and RSA (CNG import); PEM/PKCS#8 through native libssh2 path where S1 proved it; distinct errors — key unreadable vs passphrase wrong vs server rejected key (US5-3/US5-5)
- [ ] T054 [US5] PuTTY `.ppk` v2 loader (SHA-1 KDF per documented format) + explicit rejection message for v3/unknown formats naming supported ones (FR-003) in `src/plugins/sftp/keyload.cpp`
- [X] T055 [US5] Connect dialog auth UI in `src/plugins/sftp/dialogs.cpp`: auth-method selector, key-file picker (existence check at connect, US5-5), passphrase prompt with "save passphrase" (password-manager blob, FR-004)
- [ ] T056 [US5] Checkpoint: quickstart scenarios 5 + 14 pass against OpenSSH with `PasswordAuthentication no`

---

## Phase 8: User Story 6 — Save Connections and Reuse Them After Restart (P2)

**Goal**: Full bookmark management with FTP-parity UI; secrets survive restart under password-manager protection.

**Independent Test**: quickstart scenario 4 — saved bookmark with stored password connects after restart with zero typing; verify with and without Master Password.

- [X] T057 [US6] `CSFTPServer`/`CSFTPServerList` model (data-model §1) + `Bookmarks`/`Quick Connect` registry persistence per `contracts/registry-schema.md` in `src/plugins/sftp/sftp.cpp` (config section) — replaces the quick-connect-only storage from US1
- [X] T058 [US6] Connect dialog full mode in `src/plugins/sftp/dialogs.cpp`: bookmark listbox (drag reorder, context menu, New/Rename/Delete/Copy), Advanced per-server dialog (initial remote path, target panel path, keepalive overrides, compression flag)
- [X] T059 [US6] Organize Bookmarks mode + Plugins-menu command (`CConnectDlg` mode-1 pattern) in `src/plugins/sftp/dialogs.cpp`
- [X] T060 [US6] Secret lifecycle in `src/plugins/sftp/sftp.cpp`: Save Password/Passphrase honored (no blob written when off — US6-3), `PasswordManagerEvent` re-encrypts all bookmark + quick-connect blobs, cannot-decrypt flow prompts (FTP `EnsurePasswordCanBeDecrypted` pattern)
- [ ] T061 [US6] Checkpoint: quickstart scenario 4 pass (restart persistence, master password created/changed/removed transitions)

---

## Phase 9: User Story 7 — Survive Network Problems Gracefully (P3)

**Goal**: Keepalive, reconnect with cwd restore, timeout configuration, large-scale robustness.

**Independent Test**: quickstart scenarios 10–11 — mid-transfer network kill → resume completes 4 GB file; 10k-entry dir lists cancellably.

- [X] T062 [US7] Keepalive in `src/plugins/sftp/session.cpp`: `libssh2_keepalive_config` + periodic tick (FS timer event / worker heartbeat), global + per-profile intervals (FR-022)
- [ ] T063 [US7] Reconnect in `src/plugins/sftp/session.cpp` + `fs.cpp` + `operats.cpp`: `EnsureConnected()` re-dial/re-auth (stored secrets silently, else prompt), host-key re-verification against trust store, cwd restore (FR-023); interactive ops offer reconnect on lost session; workers reconnect + resume current item (US7-2/3)
- [X] T064 [US7] Configuration dialog completion in `src/plugins/sftp/dialogs.cpp`: connect/operation timeouts, retry count + delay, keepalive, resume overlap/min-size, logging limits (FR-022, registry-schema values)
- [ ] T065 [US7] Scale hardening: 10k-entry listing responsiveness audit (approximate-count preallocation, ESC cancel, no O(n²)), 64-bit offset audit for >4 GB transfers in `src/plugins/sftp/operats.cpp`/`listing.cpp` (R2/R3)
- [ ] T066 [US7] Checkpoint: quickstart scenarios 10–11 pass (kill network mid-transfer → reconnect + resume; 10k listing; idle session survives 30 min)

---

## Phase 10: Polish & Cross-Cutting Concerns

- [X] T067 [P] Dev harness complete run (`src/plugins/sftp/test/harness.cpp`): rights formatter, OpenSSH parser, bcrypt-KDF, ed25519 vectors, .ppk v2 — all green; document invocation in `specs/008-sftp-plugin/quickstart.md` (SC-010)
- [X] T068 [P] FTP regression: `git diff --stat main -- src/plugins/ftp` shows zero changes; quickstart scenario 13 smoke (connect/list/download) passes (SC-009, FR-028)
- [X] T069 Localization audit: every user-visible string via `LoadStr` from `src/plugins/sftp/lang/lang.rc2`, no hardcoded UI text; plugin description/copyright resources complete (FR-030)
- [ ] T070 [P] `clang-format` normalize + UTF-8-BOM check on all new sources in `src/plugins/sftp/` and vendored-glue files (constitution: formatting gate)
- [X] T071 Win32 (x86) build verification of `sftp.vcxproj` + `lang_sftp.vcxproj` (both platforms stay green like other plugins)
- [ ] T072 Full quickstart validation: execute scenarios 1–14 against WSL OpenSSH, record pass/fail table in `specs/008-sftp-plugin/quickstart.md` (SC-001…SC-010 evidence)
- [ ] T073 [P] Docs: update `CLAUDE.md` key facts (project/plugin counts, libssh2 dependency), add sftp entry to `architecture/09-plugin-catalog.md`

---

## Dependencies & Execution Order

### Phase dependencies

- **Setup (1)** → **Foundational (2)** → story phases; **Polish (10)** last.
- Story order by priority: **US1 (3) → US2 (4) → US3 (5) → US4 (6) → US5 (7) → US6 (8) → US7 (9)**.

### Story dependencies (beyond Foundational)

- **US1**: none — MVP. Includes the *minimal* host-key prompt (T021) so no insecure interim build ever exists.
- **US2**: builds on US1 listing (T022/T023).
- **US3**: builds on US1 session/listing; independent of US2.
- **US4**: extends T021 with persistence; independent of US2/US3.
- **US5**: extends T020 auth path; needs T050 spike first (T051 may run in parallel with it).
- **US6**: extends T019 dialog + T011 config; uses secrets flows shared with US5's passphrase storage (T055 ↔ T060 coordinate on the blob helpers).
- **US7**: touches session/fs/operats — schedule after US3 to avoid same-file churn.

### Parallel opportunities

- Setup: T002, T004, T007, T009 alongside T001/T003.
- Foundational: T012, T013 parallel to T010/T011.
- After Phase 2, US2 and US3 can proceed in parallel with US4 (different files: `listing/dialogs` vs `hostkeys`), e.g.:
  - Dev A: T028–T034 (US2), Dev B: T035–T045 (US3), Dev C: T046–T049 (US4)
- Within US5: T051 [P] parallel to T050.

## Implementation Strategy

**MVP first**: Phases 1–3 (T001–T027) deliver a demonstrable read-only SFTP browser with password auth and mandatory fingerprint gate. **Stop, validate quickstart scenario 1, demo.**

**Incremental delivery**: each subsequent phase ends in a quickstart-mapped checkpoint (T034, T045, T049, T056, T061, T066); ship-ready after Phase 10's full validation (T072).

**Task counts**: Setup 9 · Foundational 9 · US1 9 · US2 7 · US3 11 · US4 4 · US5 7 · US6 5 · US7 5 · Polish 7 — **73 total**.
