# Research: Long-Path Viewer & Shell Crash

**Feature**: `013-longpath-shell-viewer-crash` | **Date**: 2026-07-18
**Method**: Crash-dump forensics on the user's fresh Release minidumps
(2026-07-18 07:14–07:15) — parsed for exception code and crash-thread return
addresses, symbolicated against the matching Release PDB (build 07:12) with
dbghelp. Two distinct signatures, both `STATUS_STACK_BUFFER_OVERRUN`
(0xC0000409). All file:line verified on `main` after feature 012 (`195b55b`).

## R1: Signature A — F3 / internal viewer (dumps 33924, 31748)

Symbolicated crash thread: `ThreadViewerMessageLoop` → `…Body` (viewer2.cpp:154/52).

**Root cause — `viewer2.cpp:49-50`:**
```
char name[MAX_PATH];
strcpy(name, data->Name);   // unbounded copy of the (now long) file name
```
`data->Name` is the file path handed to the viewer thread. Feature 012 fixed
the panel-side `ViewFile` compose to pass the *full* long name; the viewer
thread then overruns this `MAX_PATH` buffer. Also `captionBuf[MAX_PATH]`
(viewer2.cpp:51) is bounded (`lstrcpyn`) — truncates the caption, no overflow.

**Fix (D1)**: widen `name` to `SAL_MAX_PATH_UTF8` (the `strcpy` becomes safe;
downstream `SalGetFullName(name)`/`view->OpenFile(name,…)` and the heap
`CViewerWindow::FileName` store are already long-path capable). Widen
`captionBuf` to SAL for caption fidelity.

## R2: Signature B — clipboard Copy/Paste + drag-drop (dump 32916)

Symbolicated crash thread:
`HandleCtrlLetter` (Ctrl+V) → `CFilesWindow::ClipboardPaste` (fileswn9.cpp:262)
→ `dropTarget->DragEnter` → `CImpDropTarget::DragEnter` (shellib.cpp:721) →
inlined `CImpDropTarget::SetDirectory` (shellib.cpp) — the `/GS` cookie of the
DragEnter frame is corrupted.

**Root cause — the shell drop-target / data-object path buffers are
`MAX_PATH`/`2*MAX_PATH` (≤520), overrun by a Unicode long path (~540 UTF-8
bytes) or an ASCII path in the 260–520 range:**

| Site | Buffer | Kind |
|------|--------|------|
| `shellib.cpp:191,202` `strcpy(CurDir, path)` in `SetDirectory` | member `CurDir[2*MAX_PATH]` (`shellib.h:281`) | **CRASH** — paste/drag target path |
| `shellib.h:262` `OldDataObjectSrcFSPath[2*MAX_PATH]` (filled by `IsFakeDataObject`, shellib.cpp:706) | member | CRASH — source FS path |
| `shellib.h:60` `SrcPath[MAX_PATH]` (filled shellib.cpp:466-471,575-581, guarded `<MAX_PATH`) | member | ERROR/TRUNC — source path bounded but truncates |
| `shellib.cpp:1117` `dataObjectSrcFSPath[2*MAX_PATH]` | stack | CRASH-capable — data-object FS path |
| `shellib.cpp:2278` `mydir[2*MAX_PATH]` | stack | CRASH-capable |
| `shellib.cpp:2839` `buff[2*MAX_PATH]` | stack | CRASH-capable |
| `shellib.cpp:2388/2404` `path[MAX_PATH]`, `:2425` `display[MAX_PATH]` | stack | context-menu/display; check per-site |

Feature 012 already widened `GetShellFolder` `root` (shellib.cpp) and
`GetCurrentDir` `fullName` (shellsup.cpp) — those are correct but insufficient:
the drop target's own `CurDir` store and the data-object source-path buffers
were missed. `CCallStack::Push` (callstk.cpp:499) is `_vsnprintf_s(_TRUNCATE)`
bounded — the trace machinery is NOT a culprit (ruled out).

**Fix (D2)**: widen the shell drop-target / data-object / context-menu path
buffers that hold full panel/source paths to `SAL_MAX_PATH_UTF8` — members
`CurDir`, `OldDataObjectSrcFSPath`, `SrcPath` (and update their
`SalPathAddBackslash`/`strcpy`/guard sizes), and the stack buffers
`dataObjectSrcFSPath`, `mydir`, `buff`, plus the per-site context-menu/display
buffers that receive a path. Keep the `IsFakeDataObject`/parse guards
consistent with the new sizes. Members become larger; each `CImpDropTarget`/
data-object is a single allocation (no arrays), so the memory cost is bounded.
`ExecuteAssociation`'s `execName`/`name` (shellib.cpp:2476) that feed
`ShellExecute` stay bounded → external-verb command length remains safe
degradation.

## R3: Decisions

- **D1 Viewer thread** (`viewer2.cpp`): `name`, `captionBuf` → SAL.
- **D2 Shell machinery** (`shellib.h` / `shellib.cpp`): widen the drop-target /
  data-object / context-menu path buffers listed in R2 to SAL, keeping
  guards/append sizes consistent; ShellExecute command line stays a bounded
  safe-degradation.
- **D3 Verify**: Debug + Release build; re-parse a fresh crash dump if the
  user still crashes (tooling in the scratchpad); static sweep for any
  remaining unbounded path copy in shellib/viewer.

## R4: Closure (post-implementation)

- **Viewer (R1)**: `viewer2.cpp` `name`/`captionBuf` → SAL. F3 no longer
  overruns on a long file name.
- **Shell drop-target/data-object (R2)**: members `CurDir`,
  `OldDataObjectSrcFSPath`, `SrcPath` → SAL (+ `IsFakeDataObject` size args,
  `SrcPath` guard/append sizes); stack `dataObjectSrcFSPath`, `mydir`, browse
  `path` → SAL. `IsFakeDataObject` uses `lstrcpyn(…, bufSize)` — respects the
  widened size. `CImpDropTarget::SetDirectory` `strcpy(CurDir, path)` is now
  safe (SAL member) — the confirmed Ctrl+V crash frame.
- Shell-extension IPC (`SalShExtSharedMemView->TargetPath`, shellib.cpp
  :1238/:1316) stays MAX_PATH/2*MAX_PATH (fixed IPC contract with
  salextx64.dll) — `lstrcpyn`-bounded, truncates a long path for the context
  menu = safe degradation.
- Remaining `fileswn9.cpp`/`salshlib.cpp` clipboard buffers are `lstrcpyn`-
  bounded (truncate, no overflow). `ExecuteAssociation`/`ShellExecute` command
  buffers stay bounded (external verb length = safe degradation). One rare
  subst-drive `strcat` (fileswn9.cpp:1832, Copy-UNC) left as-is (subst targets
  are drive-mapping short).

Both dump-confirmed crash frames (viewer thread body; shell `SetDirectory`)
no longer contain an unbounded path copy.
