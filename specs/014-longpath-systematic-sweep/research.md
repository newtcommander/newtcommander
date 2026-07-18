# Research: Systematic Whole-Program Long-Path Hardening (Feature 014)

## R0 — Dump forensics: the pattern is confirmed, again

A fresh Release crash dump proves the point-fix loop has not converged.

- `salamand.exe.11292.dmp`, **2026-07-18 07:40**. The crashing binary
  (`salamand.exe`/`.pdb`) was built **07:34** — i.e. it already contained the
  feature-013 fixes. So the user rebuilt with 013 and F3 still crashed.
- Exception **`0xC0000409` (STATUS_STACK_BUFFER_OVERRUN)** — `/GS` cookie
  fast-fail: an unbounded copy overran a fixed stack buffer.
- Symbolicated crash stack (Release PDB, non-incremental → RVAs map):
  ```
  ThreadViewerMessageLoop        viewer2.cpp:158
  ThreadViewerMessageLoopEH      viewer2.cpp:144
  ThreadViewerMessageLoopBody    viewer2.cpp:121  (013 widened name/caption at :42/:54 — held)
  CViewerWindow::OpenFile        viewer2.cpp:679  <-- crash
  ```
- Root frame: `viewer2.cpp:676` `char fileName[MAX_PATH]; strcpy(fileName, file);`
  in `CViewerWindow::OpenFile`. Feature 013 widened the viewer-thread **entry**
  buffers (`name`/`captionBuf`), which let the long name flow one frame deeper
  into `OpenFile`, whose own `MAX_PATH` buffer then overran. **Textbook
  downstream-move**: each point fix relocates the overflow to the next fixed
  buffer on the same chain.

**Conclusion**: the defect is a program-wide *class* (a census found ~764
fixed-size `MAX_PATH`-class char buffers in the core, ~90+ on a direct
path-copy route across ~30 files), not a set of individual sites. It must be
eliminated by exhaustive audit, not by chasing dumps.

**Method (reusable)**: parse WER minidumps in `%LOCALAPPDATA%\CrashDumps`
(scratchpad `parse-dump3.ps1`), symbolicate RET RVAs against the Release PDB
(`resolve-rel.ps1` + VS `dbghelp.dll`). Debug `/ZI` incremental linking does
NOT symbolicate; Release does.

## R1 — Fixes applied this session (live-chain, high-confidence)

| # | file:line | buffer | verdict | fix |
|---|-----------|--------|---------|-----|
| 1 | viewer2.cpp:676 | `fileName[MAX_PATH]` in `CViewerWindow::OpenFile` | CRASH (dump-confirmed) | Eliminated the redundant intermediate; `OpenFile` now uses the `file` argument directly for `malloc(strlen(file)+1)` + `strcpy`. No fixed buffer. |
| 2 | fileswn9.cpp:60 | `curPath[2*MAX_PATH]` (=520) in `CFilesWindow::ParsePath` | BOUNDED-wrong | Widened to `SAL_MAX_PATH_UTF8`; 520 truncated the ~540-byte Unicode long path → "too long"/not-found popup. `GetGeneralPath(buf,size)` is size-bounded, so safe. |
| 3 | worker.cpp:6295 | `nameList[MAX_PATH+1]` in `DoDeleteFile` recycle-bin branch | CRASH | `memmove(nameList, name, strlen(name)+1)` had **no length guard** — a long file path overran it when deleting to the Recycle Bin. Added a guard: `strlen(name) >= MAX_PATH` → `err = ERROR_FILENAME_EXCED_RANGE` (bounded message); the recycle bin (SHFileOperation, ANSI) is MAX_PATH-bound anyway (external). |
| 4 | worker.cpp:6937 | `nameList[MAX_PATH+1]` in `DoDeleteDir` recycle-bin branch | CRASH | Same unbounded `memmove` for removing an empty subdir to the Recycle Bin. Added `strlen(name) < MAX_PATH` to the condition; a longer (empty) dir falls through to the long-path-capable `SalRemoveDirectory` (permanent — benign for an empty dir). |
| 5 | fileswn6.cpp:317 | `sourceDir[MAX_PATH+4]` in `CFilesWindow::MoveFiles` | CRASH | `memcpy(sourceDir, source, len)` overran when moving a directory whose source path is long. Widened to `SAL_MAX_PATH_UTF8+4` (MoveFiles is **non-recursive** → stack-safe). |
| 6 | fileswn6.cpp:344 | `targetDir[MAX_PATH]` in `CFilesWindow::MoveFiles` | CRASH | `strcpy(targetDir, target)` overran for a long move destination. Widened to `SAL_MAX_PATH_UTF8`. |
| 7 | fileswn6.cpp:1053 | `source[MAX_PATH]` in the drag/paste copy-move handler | BOUNDED-wrong | `lstrcpyn(...,MAX_PATH)` truncated a long source → wrong same-root detection → wrong copy strategy. Widened to `SAL_MAX_PATH_UTF8`. |
| 8 | fileswn6.cpp:1126 | `path[MAX_PATH]` (source refresh path) | BOUNDED-wrong | `lstrcpyn(...,MAX_PATH)` truncated the source dir handed to `SetWorkPath2` → wrong post-move refresh (a likely contributor to the "directory disappeared" symptom). Widened to `SAL_MAX_PATH_UTF8`. |

Confirmed **already safe** (feature 012 heap-backed): `BuildScriptMain2` copy
buffers `sourcePath`/`lastSourcePath`/`targetPath`/`targetName`/`mapNameBuf`
are `CPasteBufs` heap allocations of `SAL_MAX_PATH_UTF8`; `BuildScriptDir`'s
recursive `finalName` is `CFinalNameBuf` heap (recursion-safe). `sourcePath`
in `BuildScriptMain` is `SAL_MAX_PATH_UTF8+10` (feature 011). `root`/`fsName`
buffers hold only a volume root (COMPONENT — left `MAX_PATH`).

Verified NOT reverted (013 held): `CImpDropTarget::CurDir`/`SrcPath`/
`OldDataObjectSrcFSPath`, `mydir`, `root` are `SAL_MAX_PATH_UTF8`;
`CFilesWindow::DropPath` (the Ctrl+V paste buffer, `strcpy(DropPath, GetPath())`)
is `SAL_MAX_PATH_UTF8`. So the paste chain
(`ClipboardPaste`→`DropPath`→`CImpDropTarget`→`SetDirectory`/`CurDir`) is
crash-clean at the layers audited so far.

Viewer chain classified crash-clean after fix #1:
`SetViewerCaption` `caption[MAX_PATH+300]` is BOUNDED (`lstrcpyn(..,MAX_PATH)`;
title-bar-only cosmetic truncation, heap `FileName` used for I/O is intact);
`WM_DROPFILES` `path[MAX_PATH]` and `CM_OPENFILE` `file[MAX_PATH]` are
EXTERNAL (DragQueryFile / GetOpenFileName cap at MAX_PATH — truncate, no
overflow); `viewer.h CurrentDir[MAX_PATH]` is the Open-dialog seed, already
guarded (degrades to empty for long paths).

## R2 — Recursion hotspots (heap, not stack) — audit pending

The copy/move/delete/calc-size engine (worker.cpp, fileswn6/8.cpp) walks
directory trees recursively. Any large path buffer placed on a recursive frame
must be **heap** (feature 012 already did this for `BuildScriptDir`/paste
buffers via RAII). The remaining recursive routes are part of the pending
exhaustive audit (R4).

## R3 — Documented external / ABI safe-degradation set (leave `MAX_PATH`, keep bounded)

Consistent with features 012/013: `ShellExecute`/`ShellExecuteEx` command
line; `IShellLink` (.lnk) target resolution; `SHFileOperation` (recycle bin);
the shell "New" menu; the `salextx64.dll` shared-memory IPC `TargetPath`
(fixed contract). These are bound by an external API's own 260 limit; they
must show a clear bounded message, never crash — verified their copies are
`lstrcpyn`/bounded.

## R4 — Exhaustive per-subsystem audit — STATUS: interrupted, to resume

The planned 8 parallel per-subsystem audit passes (A–H, brief in
`audit/_BRIEF.md`) covering all ~30 core files were **interrupted by an API
session limit** (resets 11:20am Europe/Prague) before writing their
inventories. They are to be re-run to produce the complete FR-008 buffer
inventory across: viewer (A), copy/move/delete/pack engine + workers (B),
file-op UI (C), navigation/change-notify (D), core path/error primitives (E),
main window/persistence/history (F), shell/clipboard/drag (G), dialogs/find/
drive/plugin handoff (H). Until then, the live-chain crashes confirmed by the
07:40 dump (F3) and the direct paste/navigation chains have been addressed
(R1); the remaining lower-frequency operations await the full sweep.
