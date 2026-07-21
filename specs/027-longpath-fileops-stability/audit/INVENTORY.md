# Feature 027 — Long-Path Buffer Audit Inventory

Completes the never-run feature-014 exhaustive audit. Eight subsystem passes
(A–H) over the whole core file/path surface, using the 014 `_BRIEF.md`
methodology and verdict vocabulary (CRASH / BOUNDED / COMPONENT / EXTERNAL /
FIXED). Every CRASH verdict was fixed in feature 027; the static check
(`check.ps1`) proves the resolved sites stay resolved.

Verdict counts: **CRASH found = 31 logical sites → 31 fixed (0 remaining)**.
Pass B (the copy/move/delete engine) found **0** CRASH sites — it was already
fully heap-backed and recursion-safe from features 011/012/014, confirming the
F5/F6 core is solid.

## CRASH sites found and FIXED (feature 027)

| # | file:line (pre-fix) | site | fix | commit |
|---|---|---|---|---|
| 1 | drivelst.cpp:1069 | CDrivesList::CurrentPath[MAX_PATH] ← panel path (Alt+F1) | widen SAL + lstrcpyn | ea07883 |
| 2 | execute.cpp:580 | CExecuteExpData::Buffer[MAX_PATH] ← $() expand (F4) | widen SAL + bound writers | ea07883 |
| 3 | execute.cpp:946 | CFileDataExpData::Buffer[2000] ← $(Path) Make File List | widen SAL | (audit) |
| 4 | viewer3.cpp:1594 | text[300] ← sprintf save-exists (F3 save) | widen + _snprintf_s | (audit) |
| 5 | dialogs.cpp:1950 | text[300] ← sprintf save-exists (mirror) | widen + _snprintf_s | (audit) |
| 6 | salamdr5.cpp:687 | errBuf ← sprintf(path) NOTPLUGINFS (sibling :910 was hardened) | _snprintf_s _TRUNCATE | (audit) |
| 7 | salamdr3.cpp:1539 | errBuf ← sprintf in-archive path | _snprintf_s _TRUNCATE | (audit) |
| 8 | salamdr3.cpp:3534/3542/3570 | CFileHistory name[2*MAX_PATH] ← lstrcpy+sprintf FileName | widen SAL + bounded ops | (audit) |
| 9 | salamdr3.cpp:2098/2107/2116 | CPathHistory::SaveToRegistry path[2*MAX_PATH] ← history path | widen SAL + lstrcpyn/StrNCat | (audit) |
| 10 | salamdr3.cpp:2143 | CPathHistory::LoadFromRegistry path[2*MAX_PATH] (round-trip) | widen SAL | (audit) |
| 11 | shellsup.cpp:2593 | execName[MAX_PATH+200] ← strcpy panel path (SalOpen) | widen SAL + lstrcpyn | (audit) |
| 12 | mainwnd5.cpp:806/810 | message[MAX_PATH+200] ← GetPath (Compare Dirs) | lstrcpyn | (audit) |
| 13 | mainwnd5.cpp:996/1001 | left/rightFilePath[2*MAX_PATH] ← GetPath | lstrcpyn | (audit) |
| 14 | mainwnd5.cpp:1366/1367 | left/rightFilePath[MAX_PATH] ← GetPath | lstrcpyn | (audit) |
| 15 | mainwnd5.cpp:1539/1545/1563/1569 | left/rightSubDir[MAX_PATH] ← Name / GetZIPPath | lstrcpyn | (audit) |
| 16 | mainwnd4.cpp:390/411 | GetNextFileFromPanel path/name ← GetPath/Name (User Menu) | lstrcpyn MAX_PATH | (audit) |
| 17 | mainwnd4.cpp:506 | ExpandCommand2 memcpy(fileName[MAX_PATH], path) | l>=MAX_PATH guard | (audit) |
| 18 | mainwnd3.cpp:7029 | FMExt buff[MAX_PATH] ← GetPath+Name (WinFile ABI) | lstrcpyn + SalPathAppend | (audit) |
| 19 | finddlg2.cpp:813 | EDTLB_DISPINFO::Buffer[MAX_PATH] ← ItemName (Find Options) | lstrcpyn (ABI: bound) | (audit) |
| 20 | fileswn5.cpp:356 | ChangeAttr path[MAX_PATH] ← AlterFileName | widen SAL_FIND_NAME_U8 | (audit) |
| 21 | fileswn5.cpp:2530 | RenameFile plugin newName[MAX_PATH] ← name | lstrcpyn MAX_PATH (plugin ABI) | (audit) |
| 22 | fileswn7.cpp:401 | UnpackZIP path[MAX_PATH+200] ← AlterFileName | widen SAL_FIND_NAME_U8+200 | (audit) |
| 23 | fileswn7.cpp:752 | DeleteFromZIP name[2*MAX_PATH] ← archive+path+name | widen SAL | (audit) |
| 24 | fileswn7.cpp:1298 | Pack path[MAX_PATH] ← AlterFileName (+ fileBuf inherit guard) | widen SAL_FIND_NAME_U8 + gate | (audit) |
| 25 | fileswn7.cpp:1696 | Unpack fileName[MAX_PATH] ← AlterFileName | widen SAL_FIND_NAME_U8 | (audit) |
| 26 | fileswn8.cpp:113 | DeleteThroughRecycleBin textBuf ← sprintf Name (sibling :98 hardened) | _snprintf_s _TRUNCATE | (audit) |
| 27 | fileswn8.cpp:716 | FilesAction textBuf ← sprintf(path) archive error | _snprintf_s _TRUNCATE | (audit) |
| 28 | fileswn8.cpp:1003 | FilesAction subject ← sprintf formatedFileName | _snprintf_s _TRUNCATE | (audit) |
| 29 | fileswn8.cpp:1326 | EmailFiles path[MAX_PATH] ← GetPath+Name | widen SAL | (audit) |
| 30 | fileswn0.cpp:337 | FocusShortcutTarget junctionOrSymlinkTgt[MAX_PATH] ← fullName | lstrcpyn + guard | (audit) |
| 31 | fileswn1.cpp:481/665/2273/2301/2316 | icon-reader path/fileName + OpenActiveFolder dirName/itemName | guard / widen SAL_FIND_NAME_U8 | (audit) |
| 32 | fileswn3.cpp:1264/1542 | ReadDirectory ext-lowercase buf[MAX_PATH] | widen SAL_FIND_NAME_U8 | (audit) |
| 33 | fileswn3.cpp:2020/2116/2194/2422/2501/2525 | ChangeDir errBuf ← sprintf(path) | _snprintf_s _TRUNCATE | (audit) |
| 34 | fileswn9.cpp:1608/1942/1964 | CreateDragImage/CopyFocusedName ← AlterFileName | widen SAL_FIND_NAME_U8 | (audit) |
| 35 | gui.cpp:1074 | CStaticText WM_GETTEXT off-by-one OOB write (non-path) | [len] not [len+1] | (audit) |

(Some rows bundle multiple identical statements in one site; the 31-site
count is of logical defects. Rows 20/22/24/25/31/32/34 are the dominant
pattern — `AlterFileName` copies a full name unbounded into a buffer left at
`MAX_PATH` when feature 004 grew names to `SAL_FIND_NAME_U8`.)

## Fix vocabulary applied

- **widen SAL_MAX_PATH_UTF8** — full-path buffers (history persistence,
  execName, email, viewer, Make File List).
- **widen SAL_FIND_NAME_U8** — single-name buffers fed by `AlterFileName`.
- **_snprintf_s(_TRUNCATE)** — error/confirm message `sprintf` sites.
- **lstrcpyn / bound** — MAX_PATH-ABI destinations (Find-Options display,
  WinFile FMExt, User Menu, plugin QuickRename) where the fixed size is the
  external contract → safe degradation, no crash.
- **guard** — icon-reader / reparse / Explorer-redirect paths that feed
  MAX_PATH shell APIs → skip cleanly when too long.

## §External — operations that degrade (documented, not crashes)

Carried from features 012/013/014, verified this pass:
- `ShellExecute`/`ShellExecuteEx` command line; `IShellLink` (.lnk) + Paste
  Shortcut; `SHFileOperation` Recycle-Bin delete; shell "New" menu;
  launched-process working directory (OS caps cwd at MAX_PATH).
- Shell-extension IPC `SalShExtSharedMem::TargetPath` (`shexreg.h:218`,
  fixed ABI) — feature 027 added refusal gates at both write sites
  (`shellib.cpp` ~1238/1316): a too-long target is refused with a clear
  message, never truncated to a wrong path.
- Explorer-side paste of Salamander's long-path clipboard data (Explorer's
  own limit).
- `EncryptFile`/`DecryptFile`, `GetShortPathName`, common-dialog `nMaxFile`,
  MAPI email, WinFile FMExt, User Menu — MAX_PATH by external API/ABI.
- `SalGetTempFileName` caps the base path at MAX_PATH (in-place `DoConvert`
  degrades cleanly).
- `DefaultDir[26][MAX_PATH]` global stays MAX_PATH by the plugin-consumer
  contract (feature 011 decision D1).

## §Bounded — truncating, no crash (noted, lower priority)

Long-path *correctness* limitations that do not crash (silent truncation of a
path the app could otherwise handle) — candidates for a future pass, out of
the no-crash scope of 027:
- Compare Directories (mainwnd5) — bounded to MAX_PATH/2*MAX_PATH; long-path
  compare truncates.
- `EditNewFile` (Shift+F4) — `SalGetFullName` default MAX_PATH bufSize →
  "too long" in a >MAX_PATH dir.
- `ChangePathToArchive` backup1/backup2, archive `CSalShExtPastedData`
  fields, `ClipboardPastePath` buff, hot-path save, `OpenFocusedInOtherPanel`
  — archive/plugin/hot-path routes bounded at MAX_PATH/2*MAX_PATH.
- Window title (`SetWindowTitle`) — long path truncated in the caption
  (display only).

## §Not covered (documented scope boundary)

- Bundled-plugin own file-operation UI (feature 014 T012) — core-app scope
  here; deferred.
- Archive-subsystem internal path buffers beyond the CRASH sites above
  (bounded today; full long-path archive support is a separate feature).

## Verification

- `check.ps1` — static exhaustion check: asserts the resolved CRASH sites
  stay resolved (widened decls present, bare copies gone, W-backed calls) +
  an advisory heuristic scan whose current hits are all scope-aliasing false
  positives or §External/§Bounded safe-by-source sites. Exit 0.
- `saltests` — 427 checks, 0 failed (incl. feature-027 DROPFILES + path
  canonicalization tests).
- Debug x64 + Release x64 build clean.
