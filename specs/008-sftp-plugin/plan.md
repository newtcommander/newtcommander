# Implementation Plan: SFTP Plugin — Remote File Management over SSH

**Branch**: `008-sftp-plugin` | **Date**: 2026-07-16 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/008-sftp-plugin/spec.md`

## Summary

Add a new file-system plugin `sftp.spl` that connects to Linux servers
over SFTP/SSH with password and private-key authentication, TOFU
host-key verification, full file operations (list/transfer/resume/
rename/delete/mkdir/symlinks/chmod/set-times), F3 viewing via the disk
cache, and — the key differentiator — Unix permissions/owner/group
columns with a chmod dialog. UI and configuration mirror the FTP plugin
(same connect/bookmark dialogs pattern, same password-manager storage,
same Logs window pattern). Transport: **libssh2 (vendored source,
BSD-3) on the Windows CNG crypto backend** — zero new runtime
dependencies; a plugin-side key-loader module covers OpenSSH-format and
ed25519 keys that CNG alone cannot. Full analysis and decision record:
[research.md](research.md) — **user confirmation of that design is
required before implementation starts** (zadání §0).

## Technical Context

**Language/Version**: C++20 (`/std:c++latest`), MSVC v143 (VS2022), pure WinAPI
**Primary Dependencies**: Salamander plugin SDK (interface version 104, UTF-8 names); **libssh2 1.11.x** vendored at `src/common/dep/libssh2/` with `LIBSSH2_WINCNG` (bcrypt/crypt32/ws2_32); existing `dep/zlib` 1.2.11 (optional transport compression); compact vendored ed25519 + bcrypt-KDF reference code (key loader); host services: `CSalamanderPasswordManagerAbstract`, `CSalamanderForViewFileOnFSAbstract` (disk cache), `CSalamanderGUIAbstract`
**Storage**: Windows Registry via host-passed `CSalamanderRegistryAbstract` (plugin key `SFTP`: bookmarks, known hosts, settings); secrets as password-manager blobs (AES-256 with master password, scrambled otherwise)
**Testing**: manual scenario verification per [quickstart.md](quickstart.md) against a real OpenSSH server (WSL2 / Windows OpenSSH Server); dev-only console harness for pure units (key loader, rights formatter, .ppk/OpenSSH parsers) with test vectors; regression smoke of the FTP plugin (SC-009)
**Target Platform**: Windows 11+ x64 (x86 build kept working like other plugins)
**Project Type**: native desktop application plugin (DLL → `.spl` + `english.slg` language module)
**Performance Goals**: 10,000-entry directory listing without UI freeze (cancellable); ≥ typical LAN throughput on 4 GB single-file transfers with resume; keepalive holds idle sessions ≥ 30 min
**Constraints**: GPLv2-or-later-compatible dependencies only; no cross-platform frameworks; no package manager (vendored source only); FTP plugin must remain untouched (SC-009); plugin buildable via `build.cmd` + `plugins.cfg` policy (feature 007)
**Scale/Scope**: 1 new plugin (~15–20 kLOC expected incl. vendored libssh2 ≈ 40 kLOC C); 2 new vcxproj; 1 new vendored dependency; no core-app changes

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Assessment | Status |
|---|---|---|
| I. Build Reproducibility | libssh2 vendored as pinned source, compiled in-tree; no manual steps, no downloaded DLLs (contrast: FTP's OpenSSL); wired into `build.cmd` via `plugins.cfg` entry | PASS |
| II. Backward Compatibility | Purely additive: new plugin, new registry key, new fs-name `sftp`; FTP plugin source untouched; no SDK/ABI changes | PASS |
| III. Incremental Modernization | Self-contained new code following existing plugin idioms; copies FTP patterns without refactoring FTP | PASS |
| IV. Windows Platform Commitment | Pure WinAPI + CNG crypto; libssh2 is plain C compiled by MSVC; BSD-3 + public-domain/zlib vendored code — all GPLv2-compatible (recorded in `doc/third_party.txt`) | PASS |
| V. Plugin Architecture Preservation | Implemented as a plugin; resolves the SFTP capability lost with the removed `winscp` plugin using an open-source dependency; plugin interfaces used as documented, not modified | PASS |

**Post-Phase-1 re-check** (after data-model/contracts): no new violations —
design keeps all changes inside `src/plugins/sftp/`, `src/common/dep/libssh2/`,
solution/build registration, and docs. PASS.

## Project Structure

### Documentation (this feature)

```text
specs/008-sftp-plugin/
├── plan.md              # This file
├── research.md          # Phase 0 — design document (user-confirmed gate)
├── data-model.md        # Phase 1 — entities & registry schema mapping
├── quickstart.md        # Phase 1 — build & verification walkthrough
├── contracts/
│   ├── plugin-contract.md   # SDK interfaces, FS_SERVICE flags ↔ FR map, path syntax
│   └── registry-schema.md   # Config/bookmarks/known-hosts persistence layout
└── tasks.md             # Phase 2 (/speckit.tasks — not created by /speckit.plan)
```

### Source Code (repository root)

```text
src/common/dep/libssh2/          # NEW — vendored libssh2 (include/, src/, readme.txt with pin)
src/plugins/sftp/                # NEW plugin (pattern: src/plugins/ftp, trimmed)
├── precomp.h / precomp.cpp      # PCH: spl_*.h + shared headers
├── sftp.cpp / sftp.h            # entry, CPluginInterface, config load/save
├── sftp.def                     # exports SalamanderPluginEntry/GetReqVer
├── fs.cpp / fs.h                # CPluginInterfaceForFS + CPluginFSInterface (ChangePath/ListCurrentPath/...)
├── session.cpp / session.h      # SSH/SFTP session wrapper (connect, auth, keepalive, reconnect)
├── hostkeys.cpp / hostkeys.h    # trust store + fingerprint dialogs (TOFU)
├── keyload.cpp / keyload.h      # key loader: PEM/PKCS#8, OpenSSH container, .ppk v2; sign callback
├── listing.cpp / listing.h      # ATTRS→CFileData mapping, CPluginDataInterface columns (Rights/Owner/Group)
├── operats.cpp / operats.h      # operation queue + worker thread + resume (simplified FTP model)
├── dialogs.cpp / dialogs.h      # connect/bookmarks, advanced, chmod, fingerprint, solve-error dialogs
├── logs.cpp / logs.h            # session log (CLogs/CLogsDlg pattern)
├── sftp.rc / *.rh2 / lang/      # resources + language module sources
└── vcxproj/
    ├── sftp.vcxproj / sftp.props            # → $(OutDir)plugins\sftp\sftp.spl
    └── lang_sftp.vcxproj / lang_sftp.props  # → plugins\sftp\lang\english.slg
src/vcxproj/salamand.sln         # + sftp, lang_sftp project entries (new GUIDs)
plugins.cfg                      # + "sftp=on" (mandatory per 007 policy)
doc/third_party.txt              # + libssh2, ed25519/bcrypt-KDF notices
architecture/04-dependencies.md  # + dependency table row
```

**Structure Decision**: single new plugin directory modeled on
`src/plugins/ftp` with the FTP-specific thirds (socket layer, TLS,
LIST parsers) replaced by `session/hostkeys/keyload/listing`; vendored
dependency follows the `src/common/dep` in-tree compilation pattern
(no separate DLL, unlike sqlite, to avoid another `utils\` runtime file).

## Complexity Tracking

No constitution violations to justify. The one notable scope choice —
re-implementing a simplified operations engine instead of linking FTP's
(~11 files, FTP-socket-entangled) — is required because no shared
library boundary exists between plugins (research.md §2), and copying
the full FTP engine would violate "reuse over duplication" worse than a
purpose-built smaller one.
