# Long-Path Buffer Audit — Shared Brief (Feature 014)

You are auditing **Open Salamander** (WinAPI C++ file manager) for a
**program-wide class of crashes**: fixed-size path buffers overrun by long
paths. Read this brief fully, then audit ONLY the files assigned to you.

## The defect class

- The app is an **ANSI build** (no `UNICODE`). Internal file names and paths
  are **UTF-8 stored in `char*`**. A path can be up to
  `SAL_MAX_PATH_UTF8` = `3*32767+1` = **98302 bytes**. `MAX_PATH` = **260**;
  `2*MAX_PATH` = 520.
- The **panel path** (`CFilesWindow` current path) and **file names**
  (`CFileData::Name`, a heap `char*`) can now exceed 260 bytes — because the
  directory path is long and/or because a Unicode name takes up to 3 UTF-8
  bytes per character.
- **The bug**: a fixed-size stack or struct buffer — `char x[MAX_PATH]`,
  `char x[2*MAX_PATH]`, `char x[MAX_PATH+n]` — that receives a **full path or
  full file name** through an **unbounded copy**: `strcpy`, `lstrcpy`,
  `sprintf`/`wsprintf`, `memcpy(dst, src, strlen(src))`, or
  `SalPathAppend(x, y, MAX_PATH)`. On the Release build (compiled with `/GS`)
  the overrun smashes the stack cookie → **`STATUS_STACK_BUFFER_OVERRUN`
  (0xC0000409)** hard crash. If the copy is **bounded** (`lstrcpyn`,
  `_snprintf_s(..,_TRUNCATE,..)`, `strncpy` with the real buffer size) it does
  NOT crash but may **truncate** → wrong path → "file not found" / a spurious
  "name too long" popup / the panel silently navigating to an ancestor.

## Fix vocabulary (what each verdict maps to)

- **CRASH** — unbounded copy of a full path/name into a fixed buffer. MUST be
  fixed. Fix options:
  - `widen to SAL_MAX_PATH_UTF8` — for a single stack buffer on a
    **non-recursive** frame, or a struct member allocated once.
  - `heap` — allocate `malloc(strlen(src)+1)` (or a RAII helper) instead of a
    stack array. **REQUIRED on recursive routes** (directory-tree walkers)
    where a 98 KB stack array per frame would exhaust the 1 MB stack.
  - `eliminate intermediate` — if the buffer is just a redundant copy of a
    `const char*` argument that is then re-copied/heap-duped, drop it and use
    the argument directly (best: no buffer at all).
- **BOUNDED** — copy is already length-limited, so no crash, but it truncates.
  Flag it if truncation yields a wrong path the app could otherwise handle
  (a correctness bug, lower priority than CRASH). Otherwise "leave".
- **COMPONENT** — buffer holds only a single file/dir **name** (≤255) or a
  drive root (≤~4). Leave it; do not enlarge.
- **EXTERNAL** — buffer feeds an external Windows API that itself imposes the
  260 limit (`ShellExecute`/`ShellExecuteEx`, `IShellLink`, `SHFileOperation`,
  the shell "New" menu, the shell-extension shared-memory IPC to
  `salextx64.dll`). Leave as **documented safe degradation**, but VERIFY the
  copy into it is bounded (no crash) — if it's an unbounded copy, the fix is
  "make bounded", not "widen".
- **FIXED** — already widened to SAL / heap by features 011/012/013 (see
  below). Confirm and mark FIXED; do not report as broken.

## Helpers available (do not reinvent)

- `common/salpath.h`: `SAL_MAX_PATH_UTF8`, `SalPathAppend`, `SalPathAddBackslash`,
  `SalPathToWExtAlloc` (adds `\\?\`).
- `common/salunicode.h`: `SalU8ToW`/`SalWToU8`/`SalU8ToWAlloc`/`SalWToU8Alloc`.
- Long-path file wrappers: `SalGetFileAttributes`, `SalCreateFile`,
  `SalFindFirstFile`, `SalFindNextFile`, `SalMoveFile`, `SalRemoveDirectory`…
- `common/winlib.h`: `SalSetWindowTextU8`, `SalGetWindowTextU8`,
  `SalSetDlgItemTextU8`, `SalComboAddStringU8`, `SalListBoxAddStringU8`,
  `SalListViewSetItemTextU8`, `SalInsertMenuItemU8`.
- Idiom for a safe bounded copy: `_snprintf_s(buf, _countof(buf), _TRUNCATE, "%s", src)`.

## Already fixed (features 011/012/013 — mark FIXED, look for NEW adjacent sites)

mainwnd1 `UpdateDefaultDir`; fileswn5 `ViewFile`/`EditFile` path + external
cmdline; fileswn6 `BuildScriptDir`/`BuildScriptFile`/`BuildScriptMain2` (heap);
salamdr1 `BuildName`; salamdr2 `GetErrorText`; salamdr5 `SalCheckPath`
`ThreadPath`; fileswn7 `AcceptChangeOnPathNotification`; viewer2
`ThreadViewerMessageLoopBody` `name`/`captionBuf`; shellib `CImpDropTarget`
`CurDir`/`SrcPath`/`OldDataObjectSrcFSPath` + `SetDirectory`; mainwnd2
panel-path restore; worker.h `WorkPath1`/`WorkPath2`; mainwnd.h
`CChangeNotifData::Path`; consts.h `CFileNamesEnumData::FileName`/`LastFileName`.

## Confirmed still-broken (from a 2026-07-18 07:40 crash dump)

`viewer2.cpp:676` `char fileName[MAX_PATH]; strcpy(fileName, file);` in
`CViewerWindow::OpenFile` — the long name now reaches here and overruns.
(This tells you the pattern: fixing one buffer pushes the long value to the
next one downstream. Trace the WHOLE chain.)

## How to work

1. `grep -nE "char [A-Za-z_]+\[(2 ?\* ?)?MAX_PATH" <yourfiles>` to seed the
   buffer list; also scan struct/class members and `WCHAR x[MAX_PATH]`.
2. For each buffer, **read the surrounding code** and determine how it is
   filled and where that data comes from (panel path? file name? a path
   argument? another already-classified buffer?). Cite `file:line`.
3. Classify with the vocabulary above and specify the concrete fix.
4. Flag every **recursive** function (directory-tree walker) — a big stack
   buffer there is dangerous; the fix must be heap.
5. **Do NOT edit any source file.** You only audit.

## Output

Write your findings as a Markdown table to
`specs/014-longpath-systematic-sweep/audit/<YOUR_LETTER>.md`:

```
# Audit <LETTER> — <subsystem>

| # | file:line | buffer | filled by | data source | verdict | fix |
|---|-----------|--------|-----------|-------------|--------|-----|
| 1 | viewer2.cpp:676 | fileName[MAX_PATH] | strcpy(fileName,file) | full path arg | CRASH | eliminate intermediate — use `file` directly for the malloc/strcpy into FileName |
```

Then **RETURN ONLY** (keep it compact, this is all that goes back to the
orchestrator):
- `CRASH_COUNT: <n>`
- a bullet list of every CRASH site: `file:line — buffer — one-line fix`
- `RECURSION_WARNINGS:` any recursive function needing a heap fix
- `NOTES:` anything structural the orchestrator must know (shared headers,
  cross-file buffers, ambiguous cases)
