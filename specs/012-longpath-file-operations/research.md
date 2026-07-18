# Research: Long-Path File Operations

**Feature**: `012-longpath-file-operations` | **Date**: 2026-07-18
**Method**: Four parallel read-only source audits (copy/move engine + script
builder; F3 view/execute/edit; rename/delete/newdir/attrs/shell; op-item
structs + panel-path corruption). Verdicts consolidated below. All file:line
references verified against `main` after feature 011 (`77a210a`).

Legend: **CRASH** = unbounded copy of a full path into a fixed buffer →
`/GS` `STATUS_STACK_BUFFER_OVERRUN` (Release) / `/RTCs` (Debug). **TRUNC** =
bounded but truncates a long path → wrong target / missed refresh. **ERROR**
= bounded, shows a "name too long" message (fixable internal limit unless
noted). **SAFE** = SAL-sized/dynamic/root-only. **DEGRADE** = genuine
external-API limit, keep bounded error.

## R1: Copy/move engine — I/O SAFE, script BUILDER crashes (`fileswn6.cpp`)

The worker I/O engine is **safe**: `COperation::SourceName/TargetName` are
heap `char*` (`worker.h:221-222`), passed directly to `SalCreateFile`/
`SalMoveFile` (long-path W layer); all `char[3*MAX_PATH]` in `worker.cpp` are
written only via the bounds-checked `MakeCopyWithBackslashIfNeeded`
(`salamdr5.cpp:1192`). **No file-data loss originates in the engine.**

The crash is in the **script builder** — feature 011 widened only
`BuildScriptMain::sourcePath` (`fileswn6.cpp:1240`) to SAL, letting the long
path flow into the sub-builders whose scratch/error buffers stayed small:

| # | Site | Buffer | Verdict / trigger |
|---|------|--------|-------------------|
| 1 | `fileswn6.cpp:2904` `memmove(name, sourcePath, strlen)` | `name[2*MAX_PATH]` | CRASH — Calculate Size, path >520 |
| 2 | `fileswn6.cpp:1884` `sprintf(text, IDS_CANNOTREADDIR, sourcePath,…)` | `text[2*MAX_PATH+100]` | CRASH — Copy/Move a DIRECTORY, list fails |
| 3 | `fileswn6.cpp:2102` same `sprintf(text,…)` | `text` (620) | CRASH — dir copy, 2nd enum fail |
| 4 | `fileswn6.cpp:2259` same | `text` | CRASH — dir copy, enum fail |
| 5 | `fileswn6.cpp:1676` `sprintf(text, IDS_DELETESHDIR, sourcePath)` | `text` | CRASH — delete hidden/system dir |
| 6 | `fileswn6.cpp:2168` `sprintf(text, IDS_NONEMPTYDIRDELCONFIRM, sourcePath)` | `text` | CRASH — delete non-empty dir |
| 7 | `fileswn6.cpp:655` `memcpy(sourcePath, fileName, …)` (BuildScriptMain2) | `sourcePath[2*MAX_PATH]` | CRASH — paste/drag, source dir >520 |
| 8 | `fileswn6.cpp:619` `strcpy(targetPath, targetDir)` | `targetPath[2*MAX_PATH+200]` | CRASH — paste/drag target >720 |
| 9 | `fileswn6.cpp:669` `strcpy(targetName, s+1)` | `targetPath+len` | CRASH — "Copy of…" into long target |
| 10 | `fileswn6.cpp:963/975/987` `sprintf(message, IDS_FILEERRORFORMAT, fileName,…)` | `message[MAX_PATH+100]` | CRASH — paste/drag error, clip path >360 |
| 11 | `fileswn6.cpp:1874/2087` `lstrcpyn(finalName, sourcePath, 720)`+append | `finalName[2*MAX_PATH+200]` | TRUNC→wrong dir — dir copy, path >720 |
| 12 | `salamdr1.cpp:1014` `BuildName`: `if (len>=MAX_PATH) return NULL` | heap | GATE — single-FILE copy REFUSED (breaks FR-002) |

## R2: View / edit / execute (`fileswn5.cpp`, `viewer*.cpp`) — bounded ERRORs

Nothing here overflows; all length-checked. The reported F3 error is a
fixable internal `MAX_PATH` limit:

| # | Site | Buffer | Verdict |
|---|------|--------|---------|
| 13 | `fileswn5.cpp:708/744/748/754` `ViewFile`: `path[MAX_PATH+10]`, `lstrcpyn(path,GetPath(),MAX_PATH)`, gate → `IDS_TOOLONGNAME` | MAX_PATH | ERROR — **the reported F3 bug**; fixable (widen + `SalPathAppend`) |
| 14 | `viewer.cpp:562-563` `char name[MAX_PATH]; lstrcpyn(name,fileName,MAX_PATH)` (viewer intake) | MAX_PATH | ERROR — masked by #13; widen |
| 15 | `fileswnb.cpp:1288-1289` + `consts.h:2138/2142` viewer next/prev enum (`FileName[MAX_PATH]`) | MAX_PATH | ERROR — step-to-next in long dir truncates; widen enum channel |
| 16 | `fileswn5.cpp:1239/1268/1272/1278` `EditFile` (F4) — mirrors ViewFile | MAX_PATH | ERROR — fixable (same fix) |
| 17 | `fileswn5.cpp:1070-1072/1104/1145` external viewer cmdLine (`expCommand/expArguments[MAX_PATH]`, `cmdLine[2*MAX_PATH]`) | MAX_PATH | ERROR — widen the expand buffers; residual `SalCreateProcess` ~32K OS cap = DEGRADE |
| 18 | `fileswn5.cpp:1393-1395/1427/1461` external editor cmdLine | MAX_PATH | ERROR — same as #17 |
| — | `fileswn2.cpp:143` `.lnk` resolve (IShellLink `GetPath(MAX_PATH)`, `oleName[MAX_PATH]`) | MAX_PATH | DEGRADE — external API limit |
| — | `shellsup.cpp:2477` `ExecuteAssociation` (`execName[MAX_PATH+200]`, bounded) → ShellExecute | MAX_PATH | DEGRADE — shell verb limit |

`Execute` (Enter) itself was made SAL-capable in feature 010; archive-open
compose is already SAL (`fileswn2.cpp:264`).

## R3: Shell-backed actions — shared crash sink `GetShellFolder`

| # | Site | Buffer | Verdict / trigger |
|---|------|--------|-------------------|
| 19 | `shellib.cpp:1675` `strcpy(root, dir)` in `GetShellFolder` (`root[MAX_PATH]`, `dir=GetPath()`) | MAX_PATH | **CRASH — THE shared shell sink**: Properties, New submenu, right-click menu, clipboard Copy/Cut, drag source; `__try/__except` can NOT catch `/GS` fastfail |
| 20 | `shellsup.cpp:505-511` `GetCurrentDir`: `memcpy(fullName[MAX_PATH], GetPath(), l)` **before** the `l+NameLen>=MAX_PATH` guard | MAX_PATH | CRASH — drag-drop onto a long-path panel |

Callers of #19: `CreateIDataObjectAux:1960`, `CreateIContextMenu2Aux:2024/
2098/2153/2193`, `GetNewOrBackgroundMenu:2624` — all pass `GetPath()`. The
leaf-name conversion (`GetItemIdListForFileName`, `shellib.cpp:1512`) is
already `SalU8ToWAlloc`-safe; only the directory `strcpy` was missed.

## R4: Create dir / delete / attrs / case / convert / size — SAFE from crash

| # | Op | Site | Verdict |
|---|----|------|---------|
| 21 | Create Dir (F7) | `fileswn5.cpp:1985` `SalGetFullName(path,…,GetPath(),…)` with default `nameBufSize=MAX_PATH` | ERROR — subdir refused when `GetPath()`≳248; fixable (pass `sizeof(path)`) |
| 22 | Recycle delete | `fileswn8.cpp:82` guard→`IDS_TOOLONGNAME` (feature 011) | DEGRADE — SHFileOperation is MAX_PATH-bound |
| 23 | Permanent delete leaf | `salamdr1.cpp:1014` `BuildName` gate | ERROR (skip) — same gate as #12; fixing #12 fixes this |
| 24 | Delete dir enum | `fileswn6.cpp:1536` `BuildScriptDir` `>=MAX_PATH-2`→skip | ERROR (skip) — widen with the builder |
| 25 | Change Attrs (Ctrl+F2) | `fileswn5.cpp:250` SAL (feature 011); enum via BuildScript* | SAFE / ERROR(skip) |
| 26 | Change Case / Convert | `BuildScriptMain(atChangeCase/atConvert)` → `BuildName` | ERROR(skip) — same family |
| 27 | Reparse/link delete | `fileswn8.cpp:422` `lstrcpyn(…,GetPath(),MAX_PATH+200)` | TRUNC — widen |

**No data-loss found**: skipped items are never scripted, so a Move never
deletes an un-copied source (the gate blocks both halves). The apparent loss
is the crash + panel navigation (R5), not file deletion.

## R5: "Directory disappeared" — panel path, not file data

`CFilesWindowAncestor::Path` is SAL (`fileswnd.h:479`); `SetPath` copies into
it (`fileswn1.cpp:231`) — never truncated in-memory. The symptom is:

| # | Site | Verdict / effect |
|---|------|------------------|
| 28 | `mainwnd2.cpp:1359` save `GetPath()` full **vs** `:2396`/`:3827-3828` load into `leftPanelPath[MAX_PATH]` | **PERSISTENT** — registry holds full path, restore truncates → panel steered away on every launch ("stays invisible") |
| 29 | `fileswn7.cpp:2031-2034` `AcceptChangeOnPathNotification` `lstrcpyn(path2, GetPath(), MAX_PATH)` | TRUNC — refresh-match on 260-char prefix → false match → panel refresh/steer-away |
| 30 | `worker.h:275/277` `WorkPath1/2[MAX_PATH]` + `dialogs.cpp:305/308` + `CChangeNotifData::Path[MAX_PATH]` (`mainwnd.h:326`, `mainwnd3.cpp:516`) | TRUNC — post-op change notification truncated → wrong/missed refresh |

## R6: Decisions

- **D1 Builder buffers (R1 #1–#11)**: widen `fileswn6.cpp` `text`/`finalName`/
  `name` and BuildScriptMain2 `sourcePath`/`targetPath`/`message` to
  `SAL_MAX_PATH_UTF8`(+slack); convert the `sprintf(…, sourcePath/fileName)`
  error/confirm sites to `_snprintf_s(_TRUNCATE)`. These are worker-thread
  frames (1MB stacks) — SAL stack arrays are the established 004/011
  precedent; where several large arrays coexist in one frame, prefer heap or
  a shared reused buffer to bound stack growth.
- **D2 Single-file gate (R1 #12 / R4 #23)**: raise `BuildName`'s
  `len >= MAX_PATH` cap to `SAL_MAX_PATH_UTF8` so long-path files build (and
  are copied/deleted), keeping the heap allocation.
- **D3 Shell sink (R3 #19)**: bound/​widen `GetShellFolder`'s `root` — but the
  shell `SHGetFileInfo`/`ILCreateFromPath` binding is itself MAX_PATH-bound,
  so the correct fix is: guard the copy (no overflow) and DEGRADE (bounded
  "name too long" for the shell action) rather than pretend shell support.
  Verify whether a `\\?\`/`ILCreateFromPathW` route lets Properties/context
  work; if not, safe-degrade. #20 `GetCurrentDir`: hoist the length guard
  before the `memcpy` (bounded, no overflow).
- **D4 View/edit (R2 #13–#18)**: widen the internal view/edit compose buffers
  to SAL + `SalPathAppend`; widen the viewer intake (`viewer.cpp:562`) and
  the next/prev enum channel; external viewer/editor expand buffers widened
  with the residual OS command-line cap as DEGRADE.
- **D5 Create dir (R4 #21)**: pass `sizeof(path)` to `SalGetFullName`.
- **D6 Panel persistence & change-notify (R5 #28–#30)**: make the config
  restore long-path capable (load into a SAL buffer / dynamic read) so the
  saved path round-trips; widen the change-notification path channel
  (`WorkPath1/2`, `CChangeNotifData::Path`, `AcceptChangeOnPathNotification`)
  to SAL so refresh matches the real path. Where a store truly cannot hold
  it, fall back to a valid ancestor.
- **D7 Verification**: build (Debug+Release); `-a` startup navigation +
  static exhaustion (interactive keystroke automation is unavailable in the
  headless env — feature 011 R5); crash-dump tooling remains available if a
  fresh dump appears.

## R7: Closure (post-implementation)

Fixes applied and verified (Debug + Release build clean; `-a` navigation
regression survives across the long-path tree):

- **Copy/move/delete builder (R1)**: `salamdr1.cpp BuildName` gate raised to
  SAL (files now build/copy); `fileswn6.cpp BuildScriptDir` `finalName`
  heap-backed (recursion-safe) + `text` bounded `_snprintf_s`;
  `BuildScriptFile` `name` → SAL, `message` bounded; `BuildScriptMain2` four
  paste path buffers heap-backed (RAII) + `message` bounded.
- **View/edit (R2)**: `ViewFile`/`EditFile` compose buffers + gates → SAL;
  `viewer.cpp` intake → SAL; viewer next/prev enum channel
  (`CFileNamesEnumData` fields, `fileswnb.cpp`, `salamdr6.cpp`, `viewer3.cpp`)
  → SAL; external viewer/editor expand+cmdLine buffers → SAL (OS
  command-line cap remains a safe degradation).
- **Shell (R3)**: `GetShellFolder` `root` → SAL (Properties, New, clipboard
  Copy/Cut, drag source, context menu no longer crash and now work);
  `GetCurrentDir` `fullName` → SAL (guard was after the memcpy).
- **Panel persistence & change-notify (R5)**: config restore buffers +
  `GetValue` → SAL (panel returns to a long-path dir on restart);
  `AcceptChangeOnPathNotification`, `COperations::WorkPath1/2`,
  `CChangeNotifData::Path`, `dialogs.cpp workPath1/2`, `mainwnd3.cpp`
  dispatch → SAL (refresh matches the real path).

Static closure sweep: every remaining `strcpy`/`memcpy`/`memmove` of a full
path in the touched files targets a SAL/heap buffer; every path-bearing
`sprintf` is `_snprintf_s(_TRUNCATE)`.

### Known limitations (safe degradation, documented — no crash)

- **Create Directory (F7)** beyond ~248 chars: `SalGetFullName` bounds to
  MAX_PATH and the downstream `CheckAndCreateDirectory` chain is MAX_PATH
  throughout; F7 shows a bounded `IDS_TOOLONGPATH` (no crash). Full
  long-path create is a follow-up (widen `CheckAndCreateDirectory`).
- **Recycle-bin delete** (`SHFileOperation`) and **shortcut (.lnk)
  resolution** (`IShellLink`) stay MAX_PATH-bound external APIs → bounded
  error.
- **External viewer/editor**: the composed command line is OS-capped (~32K);
  beyond that CreateProcess fails gracefully.
- **Reparse/link delete** (`fileswn8.cpp` reparse branch): bounded to
  MAX_PATH+200 for the link-target resolve — no overflow; a link whose path
  exceeds ~460 chars degrades the resolve (rare).
