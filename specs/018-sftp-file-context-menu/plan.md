# Implementation Plan: SFTP File Context Menu + Owner/Group

**Branch**: `018-sftp-file-context-menu` | **Date**: 2026-07-18 | **Spec**: [spec.md](spec.md)

## Summary

Two additions to the SFTP plugin, following the FTP plugin's proven pattern
(analyzed by audits A/B → research.md):

1. **File context menu**: implement `CPluginFSInterface::ContextMenu` (today an
   empty stub) to build a right-click popup for panel items, exposing **Change
   Attributes** (the existing chmod path incl. recursion) and the standard file
   operations that apply to SFTP items — mirroring how FTP builds/dispatches its
   menu.
2. **Owner/Group change**: new capability. `CSFTPSession::Chown` via
   `libssh2_sftp_stat_ex(..., LIBSSH2_SFTP_SETSTAT, attrs{UIDGID})`; a new
   owner/group dialog (UID/GID + recursive checkbox); a recursive apply
   `ChownRecursive` mirroring the existing `ChmodRecursive`; a panel entry point
   `SFTPChangeOwnerFromPanel` mirroring `SFTPChangeAttrsFromPanel`; and a menu
   item + command wiring.

The chmod machinery (dialog, mode<->rwx, `ChmodRecursive`, `CollectPanelItems`,
`COperationCtx`, `CreateSafeWaitWindow`, cancel handling) already exists and is
reused verbatim as the template for chown.

## Technical Context

**Language/Version**: C++20, MSVC v143, plugin (.spl) | **Deps**: libssh2 (already vendored); no new deps
**Files (expected)**:
- `src/plugins/sftp/fs.cpp` (+`fs.h`) — implement `ContextMenu` (build popup +
  dispatch); route the new owner/group command; maybe `ShowProperties`.
- `src/plugins/sftp/session.cpp` (+`session.h`) — `Chown(path, uid, gid, setUid, setGid)`.
- `src/plugins/sftp/operats.cpp` (+`operats.h`) — `ChownRecursive` +
  `SFTPChangeOwnerFromPanel`.
- `src/plugins/sftp/dialogs.cpp` (+`dialogs.h`) — `ShowOwnerGroupDialog`.
- `src/plugins/sftp/lang/lang.rc2` + `lang.rh` — new dialog + strings + menu
  command IDs.
- possibly `sftp.cpp` / menu-ext if the FTP pattern routes via CPluginInterfaceForMenuExt.
**Testing**: Debug + Release plugin build; interactive test vs the local test
SFTP server (feature 017: localhost:2222 / sftptest); owner/group tested where
the account may chown, else the clear-error path.
**Constraints**: no plugin-ABI change; reuse the existing operation/wait/cancel
framework; recursion must not follow symlinks (matches `ChmodRecursive`); SFTP
owner/group is numeric UID/GID on the wire.

## Constitution Check

| # | Principle | Verdict |
|---|-----------|---------|
| I | Build Reproducibility | PASS — code + plugin resource only |
| II | Backward Compatibility | PASS — adds a menu + a new op; existing paths unchanged |
| III | Incremental Modernization | PASS — mirrors existing chmod machinery + FTP menu pattern |
| IV | Windows Platform Commitment | PASS |
| V | Plugin Architecture Preservation | PASS — uses the plugin-FS ContextMenu API; no core/ABI change |
| VI | UI Consistency | PASS — menu + dialog consistent with FTP/house style |

## Phasing

1. **Audit** (agents A/B) → research.md: exact FTP context-menu build/dispatch +
   attribute/chown pattern + SFTP mapping.
2. **Owner/Group core**: session `Chown`, `ChownRecursive`,
   `SFTPChangeOwnerFromPanel`, `ShowOwnerGroupDialog` + resources.
3. **Context menu**: implement `ContextMenu` exposing Change Attributes +
   Change Owner/Group (+ standard ops per FTP), with command dispatch.
4. **Verify**: builds; interactive test vs the test server; no-regression.

## Complexity Tracking

> Low–moderate. chown is a direct analogue of the existing chmod. The main
> unknown is the exact context-menu build/dispatch API, resolved by the audits;
> mirroring FTP keeps it consistent and low-risk. Recursion reuses the proven
> cancellable walk.
