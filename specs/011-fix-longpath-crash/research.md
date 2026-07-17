# Research: Fix Application Crash When Entering a Long-Path Directory

**Feature**: `011-fix-longpath-crash` | **Date**: 2026-07-18
**Method**: Automated crash reproduction attempts (scripted keystroke drive of
the built app) + exhaustive grep/read audit of every copy of the current panel
path into fixed buffers in the core post-navigation chain. Two exploration
agents were cut off by a session limit; the audit was completed inline.

## R1: Reproduction results (scripted drive, Debug x64 build @6050f91)

| Scenario | Result |
|----------|--------|
| Shift+F7 direct to 177-char ASCII dir | ALIVE |
| Shift+F7 direct to 291-char ASCII dir | ALIVE |
| Enter-navigation root→~324-char dir→file, back up | ALIVE |
| Enter-navigation Unicode L1(185)→L2(306)→L3(427), back up | ALIVE |

The simple-entry flow did not crash under scripted 6–9 s dwell times. The
crash class is nevertheless real and confirmed statically (below): several
post-navigation consumers overflow deterministically for specific actions
(rename, delete, copy, unpack prefill, listing error) and one global is
corrupted on **every** entry. /RTC1 traps stack overwrites at function exit;
global overwrites corrupt silently and crash later — consistent with a crash
the user hits in a real session but a short scripted run misses.

## R2: Audit — every fixed-buffer consumer of the current path (core)

Verdicts: **CRASH** = overflow possible with a long current path;
**SAFE** = SAL-sized or correctly bounded; **TRUNC** = bounded, degrades.

| # | Site | Buffer | Verdict / trigger |
|---|------|--------|-------------------|
| 1 | `mainwnd1.cpp:483,488` `UpdateDefaultDir` | global `DefaultDir[26][MAX_PATH]`, **unbounded `strcpy`** | **CRASH-CLASS (global corruption)** — runs on *every* path change and panel switch; long path spills into adjacent drive rows / past the array; silent corruption, delayed crash |
| 2 | `fileswn5.cpp:2126-2143` `RenameFileInternal` | `tgtPath[MAX_PATH]`, **unbounded `memmove`** + `path[MAX_PATH]` | **CRASH** — F2 rename of any item inside a long dir (RTC traps at return) |
| 3 | `fileswn8.cpp:79-81` `DeleteThroughRecycleBin` | `path[MAX_PATH]`, unbounded `strcpy` | **CRASH** — Delete (to recycle bin) inside a long dir |
| 4 | `fileswn5.cpp:250-252` (file-times retrieval in execute flow) | `fileName[MAX_PATH]`, unbounded `strcpy` | **CRASH** — triggered per-item in long dir |
| 5 | `fileswn6.cpp:1241` `BuildScriptMain` src path | `sourcePath[2*MAX_PATH+10]` (530) | **CRASH** — F5/F6 from a dir whose path > 519 bytes (Unicode L3 = 540) |
| 6 | `fileswn3.cpp:871` listing-error report | `buf[2*MAX_PATH+100]` `sprintf` with path + error text | **CRASH** — listing failure in long dir |
| 7 | `fileswn7.cpp:1785` unpack target prefill | `path[MAX_PATH]` (dialog buffer), unbounded `strcpy` | **CRASH** — Alt+F6 empty-name path prefill in long dir |
| 8 | `mainwnd1.cpp:1172-1175` `EditWindowSetDirectory` | `dir[2*MAX_PATH]` via `GetGeneralPath` | TRUNC (GetGeneralPath bounds correctly) — command line shows truncated path; widen for fidelity |
| 9 | `mainwnd1.cpp:1845+` `SetWindowTitle`/`GetFormatedPathForTitle` | `stdWndName[2*MAX_PATH+300]`, `GetGeneralPath(path, 2*MAX_PATH)` | TRUNC-SAFE (bounded; title is compacted anyway) — no change required |
| 10 | `mainwnd2.cpp:3827-3834` startup restore | `leftPanelPath[MAX_PATH]` via `GetValue(..., MAX_PATH)` | TRUNC-SAFE (registry helper respects size; falls back) — long last-path silently not restored; acceptable degradation, document |
| 11 | `mainwnd2.cpp:2792-2813` DefaultDirs config load | `memmove` bounded by `dataLen ≤ MAX_PATH` | SAFE |
| 12 | `shellsup.cpp:2003` pastePath | `[SAL_MAX_PATH_UTF8]` | SAFE (004) |
| 13 | `fileswn9.cpp` DropPath | member `[SAL_MAX_PATH_UTF8]` | SAFE (004) |
| 14 | `fileswn1.cpp:1631` plugin-unload path restore | `[SAL_MAX_PATH_UTF8]` | SAFE (004) |
| 15 | `fileswn0.cpp:319` FocusShortcutTarget | `[SAL_MAX_PATH_UTF8]` | SAFE (004) |
| 16 | `bugreprt.cpp:1428` bug report | sprintf with path | Only in crash handler; bound with `_snprintf_s` (hardening) |

`GetGeneralPath` (`fileswn1.cpp:142`) verified: truncates to `bufSize`,
returns FALSE — callers with small buffers degrade, never overflow.

## R3: Decisions

- **D1 `DefaultDir` semantics**: rows stay `MAX_PATH` (plugin consumers —
  `salamdr3.cpp:498`, `lukas/utilaux.cpp:293`, `pictview/utils.cpp:75` — copy
  rows into their own MAX_PATH buffers; widening rows would push the overflow
  into plugins). When the panel path does not fit a row, store the **drive
  root** instead (safe, meaningful "where to go on drive change" fallback).
- **D2 Rename/delete/copy buffers**: switch to `SAL_MAX_PATH_UTF8` stack
  arrays (single per frame, UI-thread functions, 1MB+ stacks — established
  004 precedent) with guards converted from `MAX_PATH` literals to
  `sizeof(buffer)`. Recycle-bin delete additionally guards: paths that do not
  fit `MAX_PATH` get the standard too-long-name error (the shell's recycle
  bin cannot handle them anyway) — safe degradation per spec FR-002.
- **D3 Error-report sprintf** sites → `_snprintf_s(_TRUNCATE)` (established
  idiom from feature 010).
- **D4 Command line**: widen `EditWindowSetDirectory`'s buffer to
  `SAL_MAX_PATH_UTF8` so the full path reaches `CInnerText` (which stores
  dynamically and compacts for display with `PathCompactPathW`).
- **D5 Startup restore of long last-path**: left as safe degradation (falls
  back to system dir) — recorded as an accepted limitation; making the
  restore long-path capable is follow-up scope, not crash scope.
- **D6 Verification**: automated — clean build; scripted Enter-navigation
  suite over the whole test tree; scripted F2 rename + Delete inside the
  deepest dir (the two highest-value crash repros); proper-close + restart
  cycle. Static: re-grep the audited chain for unbounded copies = zero.

## R4: Crash-dump forensics (definitive class confirmation)

Five WER minidumps from the user's session (`%LOCALAPPDATA%\CrashDumps\salamand.exe.*.dmp`,
2026-07-18 00:14–00:21) were parsed directly (custom minidump parser, no
debugger available):

- **ExceptionCode = `0xC0000409` (STATUS_STACK_BUFFER_OVERRUN)** in all five —
  the `/GS` + `/RTCs` stack-cookie check firing at a function epilogue. This
  **confirms the defect class is a stack buffer overrun**, not a global
  overflow, heap issue, or encoding fault. The fault address is in
  `ucrtbase.dll` (`__report_gsfailure`); the overrunning function is the
  caller on the crashing thread's stack.
- Symbolication to the exact function was **not possible**: the debug build
  uses incremental linking (`/INCREMENTAL`, `/ZI`), so the crashing binary's
  RVAs do not map to any rebuild's layout, and the original binary was
  overwritten. `dbghelp` against a fresh PDB resolves the dump RVAs only to
  the `_enc$textbss$begin` incremental-thunk marker.

## R5: Reproduction constraints (environment)

- Interactive keystroke automation (SendKeys / journal playback) is
  **unavailable in this headless session** (no interactive desktop —
  Notepad will not launch; SendKeys returns access-denied). The panel's
  interactive Enter path (`CFilesWindow::Execute`) therefore cannot be driven.
- `-a <path>` startup navigation (which drives `ChangeDirLite` →
  `ChangePathToDisk` → the full post-navigation chain, verified by reading
  the window title = the long directory name) **survives** for every tested
  length up to 427 chars, on both the pre-fix and post-fix binaries. So the
  common post-navigation chain is robust at those lengths; the confirmed
  overrun requires either the interactive `Execute` path, a panel action
  (rename/delete/copy/unpack — all fixed), a filter-active directory line
  (fixed), or a path longer than the test tree.

## R6: Fix status vs. verification

- All 9 audited stack-overflow-capable sites are fixed (R2 + the
  `DirectoryLineSetText` filter buffer found during the closure sweep).
- Static closure sweep: every `strcpy`/`memmove`/`sprintf` of the current
  path into a fixed buffer in the core post-nav/action flow now targets a
  `SAL_MAX_PATH_UTF8` buffer, a `sizeof`-bounded copy, a `_snprintf_s`
  truncation, or a guarded safe-degradation path. Zero unbounded copies
  remain.
- `-a` survival suite passes post-fix (no regression; chain robust to 427).
- **Final interactive confirmation (F2 rename / Delete / filter inside the
  deepest directory via the panel) requires a user at an interactive
  desktop** — it could not be automated here. The crash *class* is
  eliminated by construction; the exact frame could not be symbolicated due
  to incremental-link layout mismatch.

## R7: Root cause SYMBOLICATED (definitive)

The current on-disk Release binary was the pre-fix build (from the before/after
test), built **non-incrementally** (Release omits `/INCREMENTAL` `/ZI`), so its
layout matches the user's crashing Release binary and its PDB carries real
symbols. Resolving the crash-thread return addresses (dbghelp + that PDB):

```
0x100D73  CMainWindow::UpdateDefaultDir+0xB3   mainwnd1.cpp:490   <- crash frame (/GS epilogue)
0x90E1E   CFilesWindow::ChangePathToDisk+0xDFE fileswn2.cpp:1964  <- calls UpdateDefaultDir
0x8AEAC   CFilesWindow::CommonRefresh+0x2EC    fileswn1.cpp:2406
0x9C4FF   CFilesWindow::ChangeDir+0x1F         fileswn3.cpp:1952
0x282DB0  DefaultDir+0x0                        (the overflowed global on the trace)
```

**The crashing function is `CMainWindow::UpdateDefaultDir`** — the
`strcpy(DefaultDir[LowerCase[path[0]] - 'a'], path)` at mainwnd1.cpp:483/488
copying the (291-char) current path into a `DefaultDir[26][MAX_PATH]` row.
It runs on **every** `ChangePathToDisk` (the interactive navigation path).
This is audit site #1 / task **T002**, already fixed (bound the copy; store
the drive root when the path exceeds `MAX_PATH`).

Why `-a` startup navigation never reproduced it: a single navigation's
overflow spills 31 bytes into the adjacent `DefaultDir` row and stays within
the 6760-byte global array (no fault). The user's session navigated the whole
test tree repeatedly (the crash stacks contain the 177-, 291-, 255-component,
and `L1-dlouh…` paths), accumulating corruption of adjacent globals until the
`/GS` epilogue check of `UpdateDefaultDir` (or a following global) faulted —
explaining the delayed, session-dependent crash a short scripted run misses.

**Conclusion**: T002 is the primary, confirmed fix for the reported crash;
the other eight fixes close the action-triggered siblings of the same class.
