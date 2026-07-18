# Audit A — FTP context-menu / command wiring → SFTP mapping (Feature 018)

Scope: how the FTP plugin surfaces a right-click file/dir menu and routes panel
commands (ContextMenu, ChangeAttributes, ShowProperties, QuickRename) to the
plugin FS, and exactly what the SFTP plugin already has, so we can replicate it.

All paths relative to `E:\Projects\salamander`. Every claim cites `file:line`.

---

## 1. The plugin-FS command/context-menu model (Salamander core → FS)

The FS virtual methods live on `CPluginFSInterfaceAbstract` and are documented in
`src\plugins\shared\spl_fs.h`. Each is gated by a bit returned from
`GetSupportedServices()`:

- `FS_SERVICE_QUICKRENAME   0x00000020`  (`spl_fs.h:182`)
- `FS_SERVICE_CHANGEATTRS   0x00000200`  (`spl_fs.h:190`)
- `FS_SERVICE_SHOWPROPERTIES 0x00001000` (`spl_fs.h:196`)
- `FS_SERVICE_CONTEXTMENU   0x00080000`  (`spl_fs.h:208`)

Context-menu type constants (arg `type` to `ContextMenu`) — `spl_fs.h:226-229`:
- `fscmItemsInPanel 0` — right-click on selected/focused files+dirs (THE per-file menu)
- `fscmPathInPanel 1` — right-click on the change-drive button (current path)
- `fscmPanel 2` — right-click in empty panel area

Method contracts: `ChangeAttributes` (`spl_fs.h:674`), `ShowProperties`
(`spl_fs.h:689`), `ContextMenu` (`spl_fs.h:710`), `HandleMenuMsg` (`spl_fs.h:718`,
for owner-draw/IContextMenu2/3 in the menu).

### Who calls what, in the core

- **Change Attributes command** (Ctrl+F2 / Files menu): core calls the FS method
  directly, gated on `FS_SERVICE_CHANGEATTRS`:
  `src\fileswn5.cpp:625-626` (guard) → `fileswn5.cpp:648-649`
  `GetPluginFS()->ChangeAttributes(name, HWindow, panel, count-selectedDirs, selectedDirs)`.
  On success (TRUE) core clears the selection (`fileswn5.cpp:654-659`).
  => There is NO context menu involved for Ctrl+F2; it is a first-class routed command.

- **Properties** (Alt+Enter): `src\shellsup.cpp:1480-1487`, gated on
  `FS_SERVICE_SHOWPROPERTIES` → `ShowProperties(...)`. If the service bit is not
  set, nothing happens (falls through).

- **Right-click context menu**: `src\shellsup.cpp:1490-1537`, gated on
  `FS_SERVICE_CONTEXTMENU`:
  - selection/focus present → `ContextMenu(name, listboxHWND, x, y, fscmItemsInPanel, panel, count-selectedDirs, selectedDirs)` (`shellsup.cpp:1519-1521`)
  - empty-area click → `fscmPanel` (`shellsup.cpp:1527-1529`)
  - change-drive button → `fscmPathInPanel` (`shellsup.cpp:1533-1535`)
  If `FS_SERVICE_CONTEXTMENU` is NOT set, the whole `if` is skipped and the code
  falls to the drag/shell-ext branch (`shellsup.cpp:1539+`) — i.e. **right-click
  does effectively nothing plugin-specific.** This is exactly the current SFTP
  situation.

The **plugin itself builds and shows the popup** inside its `ContextMenu` method
(LoadMenu / CreatePopupMenu + TrackPopupMenuEx). Salamander core only decides
*when* to call it and with which `type`/counts.

---

## 2. FTP `GetSupportedServices` — which services FTP advertises

`src\plugins\ftp\fs3.cpp:245-266`:
```
FS_SERVICE_CONTEXTMENU |
//FS_SERVICE_SHOWPROPERTIES |   <-- deliberately OFF
FS_SERVICE_CHANGEATTRS |
... COPYFROMDISKTOFS/MOVE.../MOVEFROMFS/COPYFROMFS/DELETE/VIEWFILE/CREATEDIR/
ACCEPTSCHANGENOTIF/QUICKRENAME/GETFSICON/GETNEXTDIRLINEHOTPATH/
GETCHANGEDRIVEORDISCONNECTITEM/GETPATHFORMAINWNDTITLE/SHOWSECURITYINFO
```
So FTP enables **CONTEXTMENU + CHANGEATTRS + QUICKRENAME**, and leaves
SHOWPROPERTIES OFF. FTP's `ShowProperties` is an empty inline stub
(`src\plugins\ftp\ftp.h:1731-1732`).

---

## 3. FTP `ContextMenu` — how the right-click menu is built & dispatched

`src\plugins\ftp\fs3.cpp:888-1039`.

Structure:
- Loads menu resource `IDM_ACTPATHCONTEXTMENU` (`fs3.cpp:894`).
- For `fscmPathInPanel` / `fscmPanel` (path/panel menu): uses `GetSubMenu(main,0)`
  (`fs3.cpp:905`) and decorates it with FTP-specific check/enable state:
  `SetTransferModeCheckMarksInSubMenu`, `SetListHiddenFilesCheckInMenu`,
  `SetShowCertStateInMenu` (`fs3.cpp:910-912`).
- For `fscmItemsInPanel` (**the per-file/dir menu**): builds a **fresh empty
  popup** with `CreatePopupMenu()` (`fs3.cpp:901`) and fills it ENTIRELY from
  Salamander's standard commands — FTP adds **no** custom per-item commands:
  - iterates `SalamanderGeneral->EnumSalamanderCommands(&index, &salCmd, nameBuf, 200, &enabled, &type2)` (`fs3.cpp:928`).
  - skips disabled commands, skips `SALCMD_OPEN` on files, and filters by command
    type to only `sctyForFocusedFile` / `sctyForFocusedFileOrDirectory` /
    `sctyForSelectedFilesAndDirectories` (`fs3.cpp:930-936`).
  - inserts separators between type groups (`fs3.cpp:938-945`).
  - each Salamander command is inserted with **`wID = salCmd + 5000`**
    (`fs3.cpp:953`) so they don't collide with FTP's own `CM_*` IDs.
- Shows the menu: `TrackPopupMenuEx(subMenu, TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, menuX, menuY, parent, NULL)` (`fs3.cpp:961-962`).
- **Dispatch** (`fs3.cpp:963-1032`):
  - `if (cmd >= 5000) SalamanderGeneral->PostSalamanderCommand(cmd - 5000);`
    (`fs3.cpp:963-964`) — i.e. a chosen standard command (Copy, Move, Delete,
    View, **Change Attributes**, …) is re-posted to the core, which then routes it
    back through the normal path (e.g. Change Attributes → `ChangeAttributes` FS method).
  - otherwise a `switch(cmd)` on FTP `CM_*` IDs for the path/panel menu items:
    `CM_DISCONNECTSRV` (posts `FTPCMD_DISCONNECT` via `PostMenuExtCommand`),
    `CM_REFRESHPATH`, `CM_ADDBOOKMARK`, `CM_SENDFTPCOMMAND`, `CM_SHOWRAWLISTING`,
    `CM_LISTHIDDENFILES`, `CM_SHOWLOG` (posts `FTPCMD_SHOWLOGS`),
    `CM_TRMODEAUTO/ASCII/BINARY`, `CM_SHOWCERT` (`fs3.cpp:967-1031`).

### KEY TAKEAWAY on Change Attributes / Properties in the menu

"Change Attributes" appears in the FTP item context menu **for free**: it is the
standard Salamander command `SALCMD_CHANGEATTRS` (see §5), enumerated by
`EnumSalamanderCommands` because FTP set `FS_SERVICE_CHANGEATTRS`; picking it does
`PostSalamanderCommand(SALCMD_CHANGEATTRS)`, and the core re-enters
`CPluginFSInterface::ChangeAttributes`. FTP does not add a bespoke "Change
Attributes" item. Same for Copy/Move/Delete/View/Rename. Properties does NOT
appear because FTP left `FS_SERVICE_SHOWPROPERTIES` off (an enumerated command is
only offered when its underlying service is enabled).

`SetTransferModeByMenuCmd` etc. are pure state helpers (`fs3.cpp:1041-1123`).

---

## 4. FTP main-menu commands (`CPluginInterfaceForMenuExt`, `menu.cpp`)

`src\plugins\ftp\menu.cpp`. These are the plugin's entries in Salamander's main
Plugins menu — **connection-level**, NOT per-file. None of them are Change
Attributes / Properties.

- `GetMenuItemState(id, eventMask)` (`menu.cpp:15-62`): enable/check/hide logic.
  E.g. `FTPCMD_DISCONNECT` returns `MENU_ITEM_STATE_HIDDEN | (eventMask &
  MENU_EVENT_THIS_PLUGIN_FS ? ENABLED : 0)` (`menu.cpp:32-35`); transfer-mode and
  cert items query the active FS via `GetPanelPluginFS(PANEL_SOURCE)`.
- `ExecuteMenuItem(salamander, parent, id, eventMask)` (`menu.cpp:64-267`):
  big `switch(id)` dispatching `FTPCMD_CONNECTFTPSERVER`, `_ORGANIZEBOOKMARKS`,
  `_DISCONNECT[_F12]`, `_ADDBOOKMARK`, `_REFRESHPATH`, `_SHOWLOGS*`,
  `_SENDFTPCOMMAND`, `_SHOWRAWLISTING`, `_LISTHIDDENFILES`, `_TRMODE*`,
  `_SHOWCERT`, plus internal posted helpers (`_CLOSECONNOTIF`, `_CANCELOPERATION`,
  `_RETURNCONNECTION`, `_REFRESH*PANEL`, `_ACTIVWELCOMEMSG`,
  `_CHANGETGTPANELPATH`). Most resolve the FS with `GetPanelPluginFS(...)` and call
  a method on it.
- `HelpForMenuItem` (`menu.cpp:269-317`).
- These items are registered in `CPluginInterface::Connect` via
  `salamander->AddMenuItem(...)` (FTP registers them; SFTP equivalent below).
- FTP re-posts core commands here too, e.g. `FTPCMD_DISCONNECT_F12` does
  `PostSalamanderCommand(SALCMD_DISCONNECT)` (`menu.cpp:82-86`) — same
  post-a-standard-command trick used by ContextMenu.

There is **no** `BuildMenu` override in FTP; menu items are static via `AddMenuItem`.

---

## 5. Standard Salamander command IDs & the enum API

`src\plugins\shared\spl_gen.h`:
- Command-type constants `sctyXXX` (`spl_gen.h:596-602`):
  `sctyForFocusedFile 1`, `sctyForFocusedFileOrDirectory 2`,
  `sctyForSelectedFilesAndDirectories 3`, `sctyForCurrentPath 4`,
  `sctyForConnectedDrivesAndFS 5`.
- Command IDs `SALCMD_*` (`spl_gen.h:607-631`), notably:
  `SALCMD_QUICKRENAME 21` (F2), `SALCMD_PROPERTIES 44` (Alt+Enter),
  `SALCMD_CHANGEATTRS 46` (Ctrl+F2), `SALCMD_COPY/MOVE/DELETE 40/41/43`,
  `SALCMD_VIEW 0`, `SALCMD_DISCONNECT 90`.
- `EnumSalamanderCommands(index, salCmd, nameBuf, size, enabled, type)`
  (`spl_gen.h:2268`) and `PostSalamanderCommand(salCmd)` (`spl_gen.h:2289`) — the
  two primitives FTP's ContextMenu uses.

---

## 6. FTP ChangeAttributes (the chmod path) — detail

`src\plugins\ftp\fs5.cpp:12-253`.
- Rejects "simple listing" panels (raw, unparsed) — `fs5.cpp:24-31`.
- Reads current mode by finding the "Rights" column
  (`dataIface->FindRightsColumn()`, `fs5.cpp:49`) and converting the rwx string to
  a number via `GetAttrsFromUNIXRights(&actAttr, &attrDiff, rights)`
  (`fs5.cpp:70`, impl `ftputils.cpp:1432`). Across the selection it accumulates a
  common `attr` + a `attrDiff` bitmask of bits that differ (`fs5.cpp:72-78`).
- If not a Unix rights model → warns (`IDS_CHATTRNOTUNIXSRV`, `fs5.cpp:96-100`).
- Dialog: `CChangeAttrsDlg dlg(parent, subject, attr, attrDiff, selDirs); dlg.Execute()` (`fs5.cpp:105-106`).
- Builds a queued async operation: `new CFTPOperation` → `oper->SetOperationChAttr(path, delimiter, TRUE, dlg.IncludeSubdirs, (WORD)dlg.AttrAndMask, (WORD)dlg.AttrOrMask, dlg.SelFiles, dlg.SelDirs, Config.OperationsUnknownAttrs)` (`fs5.cpp:128-130`; impl `operats2.cpp:270`).
- Per selected item builds a queue item via `CreateItemForChangeAttrsOperation(...)` (`fs5.cpp:168`, impl `fs4.cpp:1196`) and either queues (`Config.ChAttrAddToQueue`) or runs immediately through `RunOperation` (worker + progress dialog, `fs5.cpp:220-229`, `RunOperation` at `fs5.cpp:255`).

### The chmod dialog model (rwx / octal) — `dialogs4.cpp:1406-1565`, `dialogs.h:946-965`
- 9 tri-state checkboxes (owner/group/others × read/write/exec): value `0`=clear,
  `1`=set, `2`=indeterminate/mixed. `AttrDiff` seeds indeterminate boxes
  (`Transfer`, `dialogs4.cpp:1497-1514`).
- Produces two masks: `AttrAndMask` (bits to KEEP; default `0777`, cleared per
  unchecked bit) and `AttrOrMask` (bits to SET) — `dialogs4.cpp:1516-1556`. Octal
  bit layout: owner rwx `0400/0200/0100`, group `0040/0020/0010`, others
  `0004/0002/0001`.
- Extra options: `IncludeSubdirs` (recursion), `SelFiles`/`SelDirs` (apply to
  files/dirs), plus an "add to queue" checkbox — `dialogs4.cpp:1561-1564`,
  `dialogs.h:960-962`.
- A synced numeric octal edit box (`IDE_NUMATTRVALUE`, `RefreshNumValue`
  `dialogs4.cpp:1421-1455`).
- Because it uses AND/OR masks (not an absolute mode), FTP can apply a *relative*
  permission change across a heterogeneous multi-selection.

### FTP wire operation
`SITE CHMOD` — `src\plugins\ftp\ctrlcon2.cpp:118-129`:
`ftpcmdChangeAttrs` → `"SITE CHMOD %03o %s"`; `ftpcmdChangeAttrsQuoted` →
`"SITE CHMOD %03o \"%s\""` (enum `ctrlcon.h:35-36`). Recursion (`IncludeSubdirs`)
is realized by the FTP operation engine expanding directories into per-file
`SITE CHMOD` queue items (operats*).

### FTP QuickRename — `fs5.cpp:518-...`
Two-phase (`mode==1` lets the standard dialog open; `mode==2` receives the new
name). Handles rename masks (`SalamanderGeneral->MaskName`, `fs5.cpp:543-548`),
overwrite detection by re-parsing the listing, then
`ControlConnection->QuickRename(...)` (`fs5.cpp:606`, impl `ctrlcon5.cpp:624`).
Gated by `FS_SERVICE_QUICKRENAME`; core dispatch is `SALCMD_QUICKRENAME` / F2.

### OWNER / GROUP (chown) in FTP
**Not supported.** Grep for `CHOWN`/`chown`/owner-set across `src\plugins\ftp`
finds only `SITE CHMOD`; there is no chown wire command and no owner/group field
in `CChangeAttrsDlg`. FTP changes permissions only.

---

## 7. SFTP plugin — what already exists vs. what is missing

### GetSupportedServices — `src\plugins\sftp\fs.cpp:674-682`
```
FS_SERVICE_COPYFROMFS | MOVEFROMFS | COPYFROMDISKTOFS | MOVEFROMDISKTOFS |
DELETE | QUICKRENAME | CREATEDIR | CHANGEATTRS | VIEWFILE |
GETCHANGEDRIVEORDISCONNECTITEM | GETFSICON | GETNEXTDIRLINEHOTPATH |
GETPATHFORMAINWNDTITLE | ACCEPTSCHANGENOTIF | COMMANDLINE
```
**Missing `FS_SERVICE_CONTEXTMENU`** (and, like FTP, no SHOWPROPERTIES). This is
the root cause that right-click does nothing: `shellsup.cpp:1490-1492` requires
the bit before it will ever call `ContextMenu`.

### Method status (`src\plugins\sftp\fs.h`)
- `ChangeAttributes` — DECLARED `fs.h:125`, IMPLEMENTED `fs.cpp:1011-1020`
  (works today via Ctrl+F2 because CHANGEATTRS is set).
- `ShowProperties` — empty inline stub `fs.h:127`.
- `ContextMenu` — empty inline stub `fs.h:128-129`.
- `HandleMenuMsg` — returns FALSE `fs.h:130`.
- `QuickRename` — DECLARED `fs.h:109`, IMPLEMENTED `fs.cpp:792`.

### SFTP ChangeAttributes flow (already complete for chmod)
`fs.cpp:1011` → `EnsureConnected` → `SFTPChangeAttrsFromPanel(parent, &Session, panel, Path)` (`operats.cpp:654-696`) → `PostRefreshPanelFS`.

- **Selection gather**: `CollectPanelItems(panel, &items)` (`operats.cpp:281-317`)
  — enumerates `GetPanelSelectedItem`, falls back to `GetPanelFocusedItem`
  (skips ".."); copies real name + Mode/HasMode/Size into `CSFTPDirEntry` **before**
  showing the modal dialog (panel `CFileData*` go stale once the dialog pumps
  messages — noted at `operats.cpp:656-658`).
- **Dialog**: `ShowChmodDialog(parent, label, multiple, &mode, &recurse, &setTime, &mtime)` (`operats.cpp:676`, decl `dialogs.h:43`, impl `dialogs.cpp:460`). Returns an **absolute** octal `mode` (`items[0]->Mode & 07777` seeds it, `operats.cpp:665-668`), a `recurse` flag, and optional mtime. The recurse checkbox already exists in the resource: `IDC_RECURSE "Apply to subdirectories recursively"` (`lang\lang.rc2:199`).
- **Recursive apply (client-side walk)**: `ChmodRecursive(ctx, remotePath, mode, isDir, recurse)` (`operats.cpp:625-652`): `Session->Chmod` on the node, then if dir+recurse it `Session->ListDir`s and recurses, **skipping symlinks** (`operats.cpp:644-645`). Driven per selected item at `operats.cpp:683-692`, inside a `CreateSafeWaitWindow`/`DestroySafeWaitWindow` cancel window (`operats.cpp:680-694`).
- **Wire op**: `CSFTPSession::Chmod(path, mode)` (`session.cpp:693-702`):
  `attrs.flags = LIBSSH2_SFTP_ATTR_PERMISSIONS; attrs.permissions = mode;`
  `libssh2_sftp_stat_ex(Sftp, path, len, LIBSSH2_SFTP_SETSTAT, &attrs)`.
  (Also `SetMTime` uses `LIBSSH2_SFTP_ATTR_ACMODTIME`, `session.cpp:704-714`.)

### SFTP menu-ext (`sftp.cpp`) — analogous to FTP menu.cpp
- Registered in `CPluginInterface::Connect`: `AddMenuItem` for `SFTPCMD_CONNECT`,
  `_ORGANIZEBOOKMARKS`, `_CREATESYMLINK` (gated `MENU_EVENT_THIS_PLUGIN_FS`),
  `_SHOWLOGS` (`sftp.cpp:638-645`).
- `GetMenuItemState` currently returns `MENU_ITEM_STATE_ENABLED` for everything
  (`sftp.cpp:679-682`).
- `ExecuteMenuItem` switch (`sftp.cpp:684-711`): connect / organize bookmarks /
  show logs / create-symlink (stub message) / `SFTPCMD_DEFERREDCD`.
- Command IDs (`sftp.h:174-178`): `SFTPCMD_CONNECT 1`, `_ORGANIZEBOOKMARKS 2`,
  `_SHOWLOGS 3`, `_CREATESYMLINK 4`, `_DEFERREDCD 5`. No per-file command yet.

### chown in SFTP: NOT implemented
SFTP reads owner/group for display only — `CSFTPDirEntry`/`CSFTPItemData` carry
`Uid/Gid/Owner/Group` (`listing.h:12-15`, `listing.cpp:144-147`) and render the
Owner/Group columns (`listing.cpp:41-72,214-252`). There is **no** session method
that writes UID/GID (no `Chown`; only `Chmod` + `SetMTime` in `session.cpp`).

---

## 8. SFTP mapping recommendation (concrete)

1. **Enable the service**: add `FS_SERVICE_CONTEXTMENU` to
   `GetSupportedServices()` (`sftp.cpp`/`fs.cpp:674-682`). Without it, core never
   calls `ContextMenu` (`shellsup.cpp:1490-1492`).

2. **Implement `CPluginFSInterface::ContextMenu`** (replace the `fs.h:128-129`
   stub with an out-of-line impl in `fs.cpp`), mirroring FTP `fs3.cpp:888-1039`:
   - For `type == fscmItemsInPanel`: `CreatePopupMenu()`, fill from
     `EnumSalamanderCommands` with `wID = salCmd + 5000` and the same type filter
     (`sctyForFocusedFile / …OrDirectory / …SelectedFilesAndDirectories`); this
     brings View/Copy/Move/Delete/Rename **and Change Attributes** for free
     (SFTP already has CHANGEATTRS). Then **append SFTP-specific items** for the
     NEW commands (own `CM_*` IDs below a private base, disjoint from the +5000
     range), e.g. `CM_SFTP_CHOWN` (Change Owner/Group). Enable/disable by
     selection using the `selectedFiles`/`selectedDirs` args plus
     `GetPanelFocusedItem` (as FTP does at `fs3.cpp:920`).
   - `TrackPopupMenuEx(..., TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON, ...)`.
   - Dispatch: `if (cmd >= 5000) PostSalamanderCommand(cmd-5000);` else `switch`
     on the SFTP `CM_*` IDs → call the new handler.
   - Optionally handle `fscmPathInPanel`/`fscmPanel` with connection commands
     (Disconnect/Refresh/Show Log/Organize Bookmarks) if a path/panel menu is
     wanted; otherwise ignore those types.

3. **Change Attributes**: leave as-is — it already works via CHANGEATTRS routing
   (`fileswn5.cpp:648`) and will now also appear in the item context menu
   automatically through `EnumSalamanderCommands`. Do NOT add a manual item for it.

4. **NEW owner/group change (chown) with recursion** — the genuinely new work:
   - Add `CSFTPSession::Chown(path, uid, gid)` beside `Chmod`
     (`session.cpp:693`): set `attrs.flags = LIBSSH2_SFTP_ATTR_UIDGID;
     attrs.uid = uid; attrs.gid = gid;` then
     `libssh2_sftp_stat_ex(Sftp, path, len, LIBSSH2_SFTP_SETSTAT, &attrs)`
     (same call shape as `Chmod`, different flag — brief's "UIDGID"). Optionally
     combine PERMISSIONS|UIDGID|ACMODTIME in one setstat.
   - Add an owner/group dialog (numeric uid/gid; names→id resolution is a server
     concern, so numeric is the safe default) with a "recurse" checkbox — reuse
     the `IDC_RECURSE` pattern from `ShowChmodDialog` (`dialogs.cpp:460`,
     `lang.rc2:199`).
   - Add a recursive walk mirroring `ChmodRecursive` (`operats.cpp:625-652`):
     `Session->Chown` per node, `ListDir` + recurse for dirs, skip symlinks; wrap
     in `CreateSafeWaitWindow`/cancel like `SFTPChangeAttrsFromPanel`
     (`operats.cpp:679-694`). Gather the working set with the existing
     `CollectPanelItems` (`operats.cpp:281`).
   - Route it either as a private context-menu `CM_*` handled directly in
     `ContextMenu`, or (FTP-style) as a menu-ext command posted via
     `PostMenuExtCommand(SFTPCMD_CHOWN, TRUE)` + a new `SFTPCMD_*` in `sftp.h` and
     a case in `ExecuteMenuItem` (`sftp.cpp:684`). Direct handling in ContextMenu
     is simpler and keeps the panel/selection context.

5. **ShowProperties / FS_SERVICE_SHOWPROPERTIES**: not needed for feature 018 —
   FTP leaves it off and so can SFTP. Alt+Enter will remain a no-op.

---

## Compact summary

**MECHANISM** — Salamander core, not the plugin, decides when to call the FS
methods, each gated by a `GetSupportedServices()` bit. Right-click →
`shellsup.cpp:1490-1521` calls `CPluginFSInterface::ContextMenu(..., fscmItemsInPanel, ...)`
only if `FS_SERVICE_CONTEXTMENU` is set. The plugin builds+shows the popup itself.
FTP (`fs3.cpp:888-1039`): for items it `CreatePopupMenu()` and fills it purely
from `EnumSalamanderCommands` (`fs3.cpp:928`) with `wID = salCmd + 5000`
(`fs3.cpp:953`), `TrackPopupMenuEx` returns the id, dispatched as
`PostSalamanderCommand(cmd-5000)` for `cmd>=5000` (`fs3.cpp:963-964`) or a
`switch` of FTP `CM_*` for path/panel items (`fs3.cpp:967-1031`). "Change
Attributes" therefore appears for free (it is `SALCMD_CHANGEATTRS 46`,
`spl_gen.h:622`) and re-enters `ChangeAttributes`. Ctrl+F2 reaches the FS directly
at `fileswn5.cpp:625-649` (gated by `FS_SERVICE_CHANGEATTRS`); Properties at
`shellsup.cpp:1480-1487` (gated by `FS_SERVICE_SHOWPROPERTIES`, which FTP leaves
OFF — `fs3.cpp:248`, stub `ftp.h:1731`). FTP's own `menu.cpp`
(`CPluginInterfaceForMenuExt`) items are connection-level only, never per-file.

**ATTRS** — FTP `ChangeAttributes` `fs5.cpp:12-253`: reads mode from the Rights
column via `GetAttrsFromUNIXRights` (`ftputils.cpp:1432`), shows tri-state
rwx dialog `CChangeAttrsDlg` (`dialogs4.cpp:1406-1565`) producing `AttrAndMask`
(keep) + `AttrOrMask` (set) + `IncludeSubdirs`/`SelFiles`/`SelDirs`, then a queued
`CFTPOperation` (`SetOperationChAttr` `operats2.cpp:270`) whose wire op is
`SITE CHMOD %03o <name>` (`ctrlcon2.cpp:118-129`); recursion = engine expands
dirs. SFTP already has the full equivalent: `ChangeAttributes` (`fs.cpp:1011`) →
`SFTPChangeAttrsFromPanel` (`operats.cpp:654`) → `ShowChmodDialog`
(`dialogs.cpp:460`, absolute octal + `recurse` + mtime) → `ChmodRecursive`
(`operats.cpp:625`, client-side ListDir walk, skips symlinks) → `Session->Chmod`
(`session.cpp:693`, `libssh2_sftp_stat_ex` SETSTAT + `ATTR_PERMISSIONS`).

**OWNER_GROUP** — FTP: **not supported** (only `SITE CHMOD`; no chown wire cmd, no
owner/group in its dialog). SFTP: reads Uid/Gid/Owner/Group for display only
(`listing.h:12-15`, `listing.cpp:144-147`); **no chown write** — only `Chmod` +
`SetMTime` exist in `session.cpp`. So owner/group change is genuinely new.

**SFTP_MAPPING** — (1) add `FS_SERVICE_CONTEXTMENU` to `GetSupportedServices`
(`fs.cpp:674-682`); (2) implement `ContextMenu` (replace stub `fs.h:128-129`)
FTP-style: EnumSalamanderCommands+5000 for standard cmds (gives Change
Attributes/Copy/Move/Delete/Rename/View for free) plus appended private `CM_*`
items; TrackPopupMenuEx; dispatch `PostSalamanderCommand(cmd-5000)` or local
switch; (3) leave Change Attributes as-is (already routed); (4) add NEW chown:
`CSFTPSession::Chown` using `LIBSSH2_SFTP_ATTR_UIDGID` setstat (mirror `Chmod`
`session.cpp:693-702`), an owner/group dialog with a recurse checkbox (reuse
`IDC_RECURSE` pattern), a `ChownRecursive` mirroring `ChmodRecursive`
(`operats.cpp:625`), gathering the selection with existing `CollectPanelItems`
(`operats.cpp:281`); wire via a private context-menu id or a new `SFTPCMD_*` +
`ExecuteMenuItem` case (`sftp.cpp:684`). SHOWPROPERTIES not needed.

**NOTES** — (a) Command-id space: standard cmds use `+5000`; keep SFTP private ids
in a separate low range (FTP's `CM_*`) to avoid collision. (b) `CollectPanelItems`
must snapshot names/modes **before** any modal dialog (panel `CFileData*` go stale,
`operats.cpp:656-658`). (c) Recursive walks must skip symlinks (as
`ChmodRecursive` does, `operats.cpp:644-645`) to avoid chmod/chown through links.
(d) An enumerated standard command only shows in the item menu when its service
bit is on — that's why Properties is absent from FTP's menu. (e) SFTP's
`ShowChmodDialog` already returns an **absolute** mode (simpler than FTP's
AND/OR masks); an owner/group dialog should likewise take absolute uid/gid, using
numeric ids to avoid server-side name resolution. (f) `HandleMenuMsg`
(`fs.h:130`) only matters if the popup gets owner-draw/shell IContextMenu items;
the plain-string menu here does not need it.
