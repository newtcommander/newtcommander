# Audit B — FTP attributes/chmod + owner/group + recursion → SFTP mapping (Feature 018)

Scope: how the FTP plugin changes Unix file mode (chmod), the dialog + octal/rwx
model, the recursive-apply-to-tree logic, the wire op, and any owner/group
support; then what the SFTP plugin already does vs. what is missing.

All paths absolute under `E:\Projects\salamander\`.

---

## PART 1 — FTP plugin

### 1.1 Entry point (FS command → dialog → operation)

`CPluginFSInterface::ChangeAttributes(fsName, parent, panel, selectedFiles, selectedDirs)`
— `src\plugins\ftp\fs5.cpp:12`. This is the standard Salamander FS method that
Salamander routes the panel "Change Attributes" command to (declared on the FS
interface; the SFTP twin is `src\plugins\sftp\fs.cpp:1011`).

Flow inside it:
1. Requires a *parsed* listing (not the simple/raw fallback) — `fs5.cpp:24-31`.
2. Locates the "Rights" column via `CFTPListingPluginDataInterface::FindRightsColumn()`
   — `fs5.cpp:49`. If no Rights column, it warns "probably not a Unix server"
   (`IDS_CHATTRNOTUNIXSRV`) — `fs5.cpp:96-100`.
3. Seeds the dialog by iterating the current selection (or focused item),
   reading each item's rights string and converting via
   `GetAttrsFromUNIXRights(&actAttr, &attrDiff, rights)` — `fs5.cpp:57-92`.
   - `attr` = the first item's mode; for later items, bits that differ are OR-ed
     into `attrDiff` (`fs5.cpp:76-78`). `attrDiff` therefore = "bits that are not
     the same across the whole selection" → drives tri-state.
4. Shows `CChangeAttrsDlg dlg(parent, subject, attr, attrDiff, selDirs)` —
   `fs5.cpp:105`.
5. On IDOK, builds a `CFTPOperation` + a `CFTPQueue` of items and runs it
   (queue/worker model) — `fs5.cpp:106-249`.

### 1.2 The Change Attributes dialog — `CChangeAttrsDlg`

Declared `src\plugins\ftp\dialogs.h:949-975`; implemented
`src\plugins\ftp\dialogs4.cpp:1406-1663`. Resource `IDD_CHANGEATTRSDLG`.

Controls (all IDs referenced in dialogs4.cpp):
- 9 permission checkboxes, **3-state** (BST_CHECKED / BST_UNCHECKED / BST_INDETERMINATE=2):
  `IDC_READOWNER, IDC_WRITEOWNER, IDC_EXECUTEOWNER`,
  `IDC_READGROUP, IDC_WRITEGROUP, IDC_EXECUTEGROUP`,
  `IDC_READOTHERS, IDC_WRITEOTHERS, IDC_EXECUTEOTHERS` — `dialogs4.cpp:1423-1431`.
- `IDE_NUMATTRVALUE` — **3-digit octal** edit field (EM_LIMITTEXT 3, `dialogs4.cpp:1592`;
  must be exactly 3 digits, `dialogs4.cpp:1486`).
- `IDC_INCLUDESUBDIRS` — "apply recursively to subdirectories" — `dialogs4.cpp:1561,1589`.
- `IDC_CHATTRSETFILES` / `IDC_CHATTRSETDIRS` — apply-to-files / apply-to-dirs
  toggles (only meaningful when a dir is selected) — `dialogs4.cpp:1476-1477,1590-1591`.
- `IDC_CHATTRADDTOQUEUE` — "add to queue instead of run now" (`Config.ChAttrAddToQueue`)
  — `dialogs4.cpp:1564`.
- `IDT_CHATTRSUBJECT` — static text of the subject (UTF-8 → W conversion, feature 010)
  — `dialogs4.cpp:1581-1588`.
- **NO setuid / setgid / sticky checkboxes. NO owner / group fields.** Only the
  9 rwx bits are exposed.

Mode ↔ controls conversion:
- **controls → octal string** `RefreshNumValue()` `dialogs4.cpp:1421-1455`: each octal
  digit = `(read<<2)|(write<<1)|exec`; if any of the 3 bits in a group is
  indeterminate (value==2) the digit is shown as `'-'`.
- **octal string → controls** in `EN_CHANGE` handler `dialogs4.cpp:1615-1655`: a
  non-octal char in a position marks that whole group tri-state via `attrDiff`.
- **initial fill** `Transfer(ttDataToWindow)` `dialogs4.cpp:1495-1514`: for each bit,
  `state = (AttrDiff & bit) ? 2(indeterminate) : ((Attr & bit) != 0)`. So a bit
  that differed across the multi-selection starts grayed.
- **read-out** `Transfer(ttDataFromWindow)` `dialogs4.cpp:1516-1557`: produces two
  masks, **not** a plain mode:
  - `AttrAndMask` (default `0777`) — clears a bit when the checkbox is explicitly
    unchecked (state 0).
  - `AttrOrMask` (default `0`) — sets a bit when the checkbox is explicitly
    checked (state 1).
  - A grayed bit (state 2) leaves that bit **untouched** (stays in AND-mask, absent
    from OR-mask) → preserved per-file. Fields: `dialogs.h:961-962`.

Validation `Validate()` `dialogs4.cpp:1457-1493`: rejects "all 9 indeterminate"
(nothing to do), rejects "neither files nor dirs", requires exactly 3 octal digits.

### 1.3 Per-item mode computation (the AND/OR mask model)

Applied per file/dir in `CreateItemForChangeAttrsOperation()` `src\plugins\ftp\fs4.cpp:1196-1354`
and again for the explored directory itself in `src\plugins\ftp\operats7.cpp:1374-1452`.
Core formula (`fs4.cpp:1230-1244`):
```
changeMask = (~attrAndMask | attrOrMask) & 0777;   // bits that actually change
newAttr    = (actAttr & attrAndMask) | attrOrMask; // merge, not overwrite
```
- If nothing in `changeMask` differs from current → item is **skipped**
  (`fs4.cpp:1231-1236`).
- Bits left grayed are preserved because AND-mask keeps them and OR-mask doesn't set them.

### 1.4 The rwx bit model (setuid/setgid/sticky handling)

`GetAttrsFromUNIXRights()` `src\plugins\ftp\ftputils.cpp:1432-1487`:
- Parses the 10-char `drwxrwxrwx` string. Only maps the 9 bits to `0400..0001`.
- For the 3 exec positions, only plain `x`/`-` are understood. Any special char
  (`s`,`S`,`t`,`T`,`X`) sets the corresponding exec bit in **`attrDiff`** (an
  "unknown/unrepresentable attribute here") — `ftputils.cpp:1471-1483`.
- ACL rows (11 chars, trailing `+`) are treated as unknown (returns FALSE).

Consequence: FTP does **not** model setuid/setgid/sticky as settable bits. If a
change would disturb such a bit, the item is flagged `attrErr` and routed through
`Config.OperationsUnknownAttrs` policy = `UNKNOWNATTRS_IGNORE` / `_SKIP` /
`_USERPROMPT` (`fs4.cpp:1271-1295, 1316-1340`; problem `ITEMPR_UNKNOWNATTRS`). The
wire command is a bare 3-digit `SITE CHMOD` (no 4th digit).

### 1.5 Recursion into directory trees (queue/worker model)

FTP recursion is **queue-driven**, not a synchronous walk. Item types
(`src\plugins\ftp\operats.h:25-47`):
- `fqitChAttrsExploreDir` — explore a dir for attr change; also enqueues the dir's
  own attr item (class `CFTPQueueItemChAttrExplore`, stores the dir's original rights).
- `fqitChAttrsResolveLink` — probe whether a symlink points to a dir (CWD test).
- `fqitChAttrsExploreDirLink` — explore a symlinked dir.
- `fqitChAttrsFile` — chmod a file (`CFTPQueueItemChAttr`).
- `fqitChAttrsDir` — chmod a dir (`CFTPQueueItemChAttrDir`).

Item classes: `operats.h:435-497` (each carries `WORD Attr` = precomputed target
mode, `BYTE AttrErr`, `char* OrigRights`).

Walk mechanics:
1. Initial queue built from panel selection — `fs5.cpp:153-197` via
   `CreateItemForChangeAttrsOperation` (`fs4.cpp:1196`):
   - dir + IncludeSubdirs → `fqitChAttrsExploreDir` (`fs4.cpp:1258-1266`)
   - dir, no subdirs, SelDirs → `fqitChAttrsDir` (`fs4.cpp:1298-1306`)
   - file + SelFiles → `fqitChAttrsFile` (`fs4.cpp:1343-1346`)
   - symlink + IncludeSubdirs → `fqitChAttrsResolveLink`; else skipped
     (`fs4.cpp:1212-1221`, "link attrs can't be changed").
2. A worker pulling a `fqitChAttrsExploreDir` item CWDs into the dir, LISTs it via
   the shared explore state machine `HandleEventInWorkingState2`
   (`operats4.cpp:238-244`), parses each child, and calls
   `CreateItemForChangeAttrsOperation` **again** per child
   (`operats6.cpp:1526-1536`) → subdirs become deeper `fqitChAttrsExploreDir`
   (recursion), files become `fqitChAttrsFile`.
3. **After** listing, it appends the explored directory's OWN `fqitChAttrsDir`
   item computed from the saved `OrigRights` (`operats7.cpp:1374-1452`). Because
   children are enqueued before the parent's own chmod, children are chmod'ed
   before the parent (avoids locking yourself out of a dir by dropping +x/+r first).
4. Symlink resolution: `fqitChAttrsResolveLink` sends CWD to the link; on success
   it becomes `fqitChAttrsExploreDirLink`, on failure the resolve item is dropped
   (`operats4.cpp:293-408`).

### 1.6 The wire operation (SITE CHMOD)

Worker state `fwssWorkSimpleCmdStartWork` for `fqitChAttrsFile`/`fqitChAttrsDir`
sends `ftpcmdChangeAttrs` — `src\plugins\ftp\operats4.cpp:672-681`.
Command string built in `src\plugins\ftp\ctrlcon2.cpp:118-131`:
```
SITE CHMOD %03o <name>            (ftpcmdChangeAttrs)        ctrlcon2.cpp:122
SITE CHMOD %03o "<name>"          (ftpcmdChangeAttrsQuoted)  ctrlcon2.cpp:129
```
- 3-digit octal only. The item's `Name` is relative; the worker CWDs to `Path` first.
- On failure with whitespace in the name, retries with the quoted form
  (`operats4.cpp:826-853`). On other failure → item `sqisFailed`,
  `ITEMPR_UNABLETOCHATTRS` (`operats4.cpp:854-859`). Connection-drop → retry item.
- Errors/skips are surfaced per-item in the operation progress dialog
  (`COperationDlg`) with "Solve error" sub-dialogs; the `attrErr`/unknown-attr
  path routes through `CSolveItemErrUnkAttrDlg` etc. (`dialogs.h:1331-1386`).

### 1.7 Owner / group (chown) on FTP — NOT SUPPORTED

- The FTP command enum (`src\plugins\ftp\ctrlcon.h:24-44`) has **no** CHOWN/CHGRP —
  only `ftpcmdChangeAttrs`/`ftpcmdChangeAttrsQuoted` (both `SITE CHMOD`).
- Case-insensitive search for `SITE CHOWN` / `SITE CHGRP` / `chown` / `chgrp`
  across `src\plugins\ftp\` → zero hits (only `SITE CHMOD`).
- The Change Attributes dialog has no owner/group controls.
- **Conclusion: the FTP plugin cannot change owner or group. It only does chmod.**

---

## PART 2 — SFTP plugin (existing chmod vs. what's missing)

### 2.1 FS wiring

`CPluginFSInterface::ChangeAttributes` — `src\plugins\sftp\fs.cpp:1011-1020` →
calls `SFTPChangeAttrsFromPanel(parent, &Session, panel, Path)` then
`PostRefreshPanelFS`. (Right-click `ContextMenu` is a separate empty stub — out of
this audit's scope; see Agent A.)

### 2.2 The chmod dialog — `ShowChmodDialog`

Declared `src\plugins\sftp\dialogs.h:43-44`; implemented
`src\plugins\sftp\dialogs.cpp:314-477`. Resource `IDD_CHMOD` (=520,
`src\plugins\sftp\lang\lang.rh:149`).

Controls (`lang.rh:149-166`, dialogs.cpp):
- 9 rwx checkboxes `IDC_UR/UW/UX, IDC_GR/GW/GX, IDC_OR/OW/OX` (650-658).
- **`IDC_SETUID` (659), `IDC_SETGID` (660), `IDC_STICKY` (661)** — special bits are
  fully supported here (unlike FTP). `dialogs.cpp:335-337,361-366`.
- `IDE_OCTAL` (662) — **4-digit** octal (`FormatOctalMode` = `%04o` over `07777`,
  `src\plugins\sftp\sftputils.cpp:61-67`), two-way synced with the checkboxes
  (`dialogs.cpp:404-419`).
- `IDC_RECURSE` (663) — recurse into subdirs. `dialogs.cpp:432`.
- `IDT_CHMODTARGET` (664) — subject label (UTF-8). `dialogs.cpp:379`.
- `IDC_SETTIME` (665) + `IDE_MTIME` (666) — optional mtime set (Unix epoch seconds;
  empty = now). `dialogs.cpp:420-421,434-447`.
- **NO owner/group controls** (highest ID is 666; next free ≈ 667+).

Model differences vs FTP:
- SFTP passes a **single absolute mode** (`unsigned long*`), not AND/OR masks. The
  dialog reads octal-or-checkboxes into one value (`dialogs.cpp:423-431`).
- Tri-state for multi-selection is **NOT implemented**: `ChmodModeToControls`
  only sets BST_CHECKED/BST_UNCHECKED (`dialogs.cpp:324-338`). The `multiple` param
  is accepted (`dialogs.h:43`, `dialogs.cpp:465`) but unused in the proc.
- No "apply to files only / dirs only" toggles; no "add to queue".

### 2.3 The operation — `SFTPChangeAttrsFromPanel`

`src\plugins\sftp\operats.cpp:654-696`. Synchronous, on the calling thread, under a
cancellable `CreateSafeWaitWindow` (NOT the FTP queue/worker model — see
`operats.h:8-14`).
- Collects panel items into a private copy FIRST (`CollectPanelItems`,
  `operats.cpp:659-660`) because the modal dialog invalidates panel `CFileData`.
- Seeds mode from **`items[0]->Mode & 07777` only** (the first item; no
  cross-selection diff/merge) — `operats.cpp:667-668`.
- Label = first name, or `"%d item(s)"` for multi — `operats.cpp:669-672`.
- Per selected item → `ChmodRecursive(&ctx, rpath, mode, itemIsDir, recurse)`;
  then, if `setTime`, `session->SetMTime(rpath, mtime)` on the **top-level item only
  (not recursive for mtime)** — `operats.cpp:683-692`.

Recursion — `ChmodRecursive` `src\plugins\sftp\operats.cpp:625-652`:
- Chmods `remotePath` to `mode`; on error logs `IDS_ERR_CHMOD` (no per-item UI/skip
  dialog) — `operats.cpp:630-635`.
- If dir + recurse: `ListDir`, then for each child recurse; **skips symlinks**
  (`SFTP_S_ISLNK`, `operats.cpp:644-645`). Depth-first; applies the **same absolute
  `mode`** to every child (no preservation, no file/dir distinction, no octal
  special handling per child).
- **So recursion already exists and works** — but with the coarse "same mode to
  everything" semantics, unlike FTP's per-item AND/OR-mask preservation and
  children-before-parent ordering.

### 2.4 Wire op + owner/group reachability (libssh2)

- chmod: `CSFTPSession::Chmod` `src\plugins\sftp\session.cpp:693-702`:
  `libssh2_sftp_stat_ex(Sftp, path, len, LIBSSH2_SFTP_SETSTAT, &attrs)` with
  `attrs.flags = LIBSSH2_SFTP_ATTR_PERMISSIONS; attrs.permissions = mode;`.
- mtime: `CSFTPSession::SetMTime` `session.cpp:704-714`: same SETSTAT with
  `LIBSSH2_SFTP_ATTR_ACMODTIME` (sets atime=mtime).
- **Owner/group IS reachable but NOT implemented.** The very same SETSTAT call with
  `attrs.flags = LIBSSH2_SFTP_ATTR_UIDGID; attrs.uid = ...; attrs.gid = ...;` sets
  owner/group. libssh2's `LIBSSH2_SFTP_ATTRIBUTES` already exposes `uid`/`gid`, and
  the listing path already READS them (`session.cpp:583-587`,
  `LIBSSH2_SFTP_ATTR_UIDGID`). There is currently **no** `Chown` method anywhere in
  `src\plugins\sftp\` (only the listing read at `session.cpp:583`).
- Data available for a chown UI: `CSFTPDirEntry` (`src\plugins\sftp\session.h:12-28`)
  carries numeric `Uid`/`Gid` plus optional name strings `Owner`/`Group`; the panel
  already has Owner/Group columns (`IDS_COL_OWNER`=112, `IDS_COL_GROUP`=114,
  `lang.rh:86-89`). Note: SFTP v3 SETSTAT takes **numeric uid/gid on the wire**;
  there is no server-side name→number lookup over the protocol.

---

## PART 3 — Summary tables

### FTP vs SFTP chmod dialog
| Feature | FTP `CChangeAttrsDlg` | SFTP `ShowChmodDialog` |
|---|---|---|
| 9 rwx checkboxes | yes, 3-state | yes, 2-state only |
| octal field | 3-digit (`SITE CHMOD %03o`) | 4-digit (`%04o`) |
| setuid/setgid/sticky | NO (flagged as unknown-attr error) | YES (IDC_SETUID/SETGID/STICKY) |
| recurse into subdirs | IDC_INCLUDESUBDIRS | IDC_RECURSE |
| apply-to-files / apply-to-dirs | yes (IDC_CHATTRSETFILES/DIRS) | no |
| set mtime | no | yes (IDC_SETTIME/IDE_MTIME) |
| owner/group | no | no |
| multi-select tri-state | yes (AttrDiff → grayed) | NOT implemented (`multiple` unused) |
| model | AND/OR mask (preserve) | single absolute mode (overwrite) |

### Recursion
| | FTP | SFTP |
|---|---|---|
| engine | queue + worker (explore items) | synchronous `ChmodRecursive` on UI thread |
| skips symlinks | resolves then explores dir-links | skips symlinks entirely |
| per-item mode | recomputed w/ AND/OR mask, preserves | same absolute mode everywhere |
| order | children before parent dir | parent then children (depth-first) |
| mtime recursion | n/a | top-level only |

---

## PART 4 — SFTP implementation recommendation (feature 018)

1. **Owner/group is the only genuinely new capability.** Add
   `CSFTPSession::Chown(path, uid, gid)` = SETSTAT with `LIBSSH2_SFTP_ATTR_UIDGID`
   (mirror `Chmod` at `session.cpp:693-702`). Numeric uid/gid only (SFTP v3).
2. **Dialog:** add owner/group inputs to `IDD_CHMOD` (or a sibling dialog). Seed
   from `CSFTPDirEntry.Uid/Gid` (+ show `Owner/Group` names for reference). Accept
   numeric uid/gid; "leave unchanged" state needed so chmod-only still works.
   Reuse `IDC_RECURSE` for a recursive chown; next free control IDs ≈ 667+.
3. **Recursion:** `ChmodRecursive` (`operats.cpp:625`) is the natural place — extend
   it (or add a parallel `ChownRecursive`) to also SETSTAT uid/gid; keep the
   symlink skip. If owner/group set recursively, apply to children too (unlike the
   current mtime-only-top-level behavior — decide intentionally).
4. **Optional parity gaps** (not required by 018 but noted): SFTP lacks the FTP
   AND/OR-mask preservation and multi-select tri-state — a multi-selection chmod
   overwrites every item to one mode. If desired, port the `AttrAndMask/AttrOrMask`
   + `attrDiff` tri-state model from FTP (`dialogs4.cpp` / `fs4.cpp:1230-1244`).
</content>
</invoke>
