# Research: SFTP File Context Menu + Owner/Group (Feature 018)

Two parallel FTP audits (A context-menu mechanism, B attributes/chmod/chown)
plus orchestrator verification. Detail in `audit/A.md`, `audit/B.md`.

## R1 — Why right-click did nothing (Agent A, verified)

Salamander core (not the plugin) decides when to call each FS method, gated by
`GetSupportedServices()` bits. Right-click → `shellsup.cpp` calls
`CPluginFSInterface::ContextMenu(..., fscmItemsInPanel, ...)` **only if
`FS_SERVICE_CONTEXTMENU` (0x00080000) is set** (`spl_fs.h:208`). The SFTP plugin
did NOT return it (`fs.cpp` GetSupportedServices) AND `ContextMenu` was an empty
stub (`fs.h:128`) → nothing happened. **Fix**: add the service bit + implement
`ContextMenu`.

## R2 — The FTP context-menu pattern to mirror (Agent A)

`CPluginFSInterface::ContextMenu` (`ftp/fs3.cpp:888`): for `fscmItemsInPanel` it
`CreatePopupMenu()` and fills it **purely** from
`SalamanderGeneral->EnumSalamanderCommands` with `wID = salCmd + 5000`
(`fs3.cpp:928,953`), separators between command-type groups, filtered by
`enabled` + type (`sctyForFocusedFile` / `…FocusedFileOrDirectory` /
`…SelectedFilesAndDirectories`), then `TrackPopupMenuEx(TPM_RETURNCMD)`
(`fs3.cpp:961`); dispatch = `PostSalamanderCommand(cmd-5000)` for `cmd>=5000`
(`fs3.cpp:963-964`) or a `switch` of private `CM_*` ids.

**Change Attributes appears for free**: it is the standard command
`SALCMD_CHANGEATTRS 46` (`spl_gen.h:622`), enumerated because the FS sets
`FS_SERVICE_CHANGEATTRS`; picking it re-enters the FS `ChangeAttributes` method.
FTP adds **no** custom per-item commands; its `menu.cpp` items are
connection-level only.

## R3 — Attributes (chmod) already complete in SFTP (Agent B, verified)

SFTP already implements the full chmod path and it **already recurses**:
`ChangeAttributes` (`fs.cpp:1011`) → `SFTPChangeAttrsFromPanel` (`operats.cpp:654`,
snapshots the selection before the modal dialog) → `ShowChmodDialog`
(`dialogs.cpp`, 9 rwx + setuid/setgid/sticky + octal + `IDC_RECURSE` + set-mtime)
→ `ChmodRecursive` (`operats.cpp:625`, depth-first, **skips symlinks**) →
`CSFTPSession::Chmod` (`session.cpp:693`) = `libssh2_sftp_stat_ex(...,
SETSTAT, {ATTR_PERMISSIONS})`. So chmod/drwxrwxrwx only needed to be **exposed
in the menu** — which R2's enumeration does automatically. (FTP itself does chmod
via `SITE CHMOD %03o` and has NO owner/group support at all.)

## R4 — Owner/group is the only genuinely new capability (Agents A+B)

FTP does not support chown. SFTP reads uid/gid + owner/group names in the
listing (`CSFTPItemData.Uid/Gid/Owner/Group`, `listing.h:9`) for display only —
no write path existed. libssh2 supports it directly: SETSTAT with
`LIBSSH2_SFTP_ATTR_UIDGID` + `attrs.uid/gid`. SFTP v3 is numeric on the wire
(no name resolution) → the dialog takes absolute numeric UID/GID.

## R5 — Implementation (this feature)

1. **`FS_SERVICE_CONTEXTMENU`** added to `GetSupportedServices` (fs.cpp).
2. **`ContextMenu`** implemented (fs.cpp) mirroring FTP: EnumSalamanderCommands
   +5000 (yields View/Copy/Move/Delete/Rename/**Change Attributes**), a
   separator, then our **Change Owner/Group…** item (private id `CTXCMD_CHOWN`
   = 4001, below the +5000 range). Dispatch: `PostSalamanderCommand(cmd-5000)`
   for standard commands, or `SFTPChangeOwnerFromPanel` directly for our item
   (same context as `ChangeAttributes`; it snapshots the selection first).
3. **`CSFTPSession::Chown`** (session.cpp): reads current attrs first, then
   SETSTAT `UIDGID`, overriding only the requested field(s) → an unchanged
   owner/group is preserved (FR-005).
4. **`ChownRecursive` + `SFTPChangeOwnerFromPanel`** (operats.cpp): direct
   analogues of `ChmodRecursive`/`SFTPChangeAttrsFromPanel` (cancellable wait
   window; skip symlinks; recurse for directories).
5. **`ShowOwnerGroupDialog`** (dialogs.cpp) + `IDD_OWNERGROUP` (lang.rc2):
   per-field "change owner/group" checkboxes (so an unchanged field stays
   intact), numeric UID/GID edits seeded from the item, and an "apply
   recursively" checkbox enabled only when a directory is in the selection.
6. `CollectPanelItems` extended to copy `Uid/Gid` (to seed the dialog;
   harmless for chmod).

Design choice: a **separate** owner/group dialog + menu item (not owner/group
fields bolted onto the chmod dialog) keeps the two operations distinct and
matches the "Change Attributes" vs "Change Owner/Group" menu split.

## R6 — Verification

- Debug + Release x64 build clean.
- Interactive test is the user's (needs the live test SFTP server, feature 017:
  localhost:2222 / sftptest). Owner/group change requires a server/user
  permitted to chown; where not permitted, the clear IDS_ERR_CHOWN path (logged)
  is what is exercised — SFTP servers commonly reject chown for non-root, which
  is expected and handled without crashing.
