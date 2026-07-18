# Tasks: Long-Path Viewer & Shell Crash Fix

**Input**: Design documents from `/specs/013-longpath-shell-viewer-crash/`
**Prerequisites**: plan.md, spec.md, research.md (R1/R2 dump forensics)

**Tests**: Build (Debug+Release) + crash-dump forensic re-check + static sweep
(headless env — no interactive keystroke automation).

## Phase 1: Setup
- [X] T001 Confirm the long-path test tree (incl. the Unicode chain ~540 B) and a file to view/copy exist under `%LOCALAPPDATA%\Temp\salamander-test\010\long-paths`
- [X] T002 Verify baseline Debug x64 build passes on branch `013-longpath-shell-viewer-crash`

## Phase 2: User Story 1 — F3 viewer never crashes (P1) 🎯
- [X] T003 [US1] Fix `ThreadViewerMessageLoopBody` in `src/viewer2.cpp:49-51`: `char name[MAX_PATH]; strcpy(name, data->Name);` and `char captionBuf[MAX_PATH]; lstrcpyn(…, MAX_PATH)` → `SAL_MAX_PATH_UTF8` (the unbounded `strcpy` of the now-long name is the F3 crash; downstream `SalGetFullName`/`OpenFile`/heap `FileName` are already long-path capable)

## Phase 3: User Story 2 & 3 — Clipboard/drag/shell never crash (P1/P2)
- [X] T004 [US2] Widen the `CImpDropTarget` path members in `src/shellib.h`: `CurDir[2*MAX_PATH]` (:281), `OldDataObjectSrcFSPath[2*MAX_PATH]` (:262), `SrcPath[MAX_PATH]` (:60) → `SAL_MAX_PATH_UTF8`
- [X] T005 [US2] Update `CImpDropTarget::SetDirectory` in `src/shellib.cpp:191,202` (`strcpy(CurDir, path)` — now safe with the widened member) and any `CurDir`/`SrcPath`/`OldDataObjectSrcFSPath` size args (`SalPathAddBackslash`, `IsFakeDataObject(…, 2*MAX_PATH)` at :706, the `strlen<MAX_PATH` guards at :466-471/:575-581) → SAL, consistently
- [X] T006 [P] [US2] Widen the shell stack path buffers in `src/shellib.cpp` that hold a full path: `dataObjectSrcFSPath[2*MAX_PATH]` (:1117), `mydir[2*MAX_PATH]` (:2278), `buff[2*MAX_PATH]` (:2839), and the context-menu/display buffers `path[MAX_PATH]` (:2388,:2404), `display[MAX_PATH]` (:2425) where they receive a panel/source path → SAL (verify each per site; leave short-name-only buffers)
- [X] T007 [US3] Verify `ExecuteAssociation`/`ShellExecute` command buffers (`shellib.cpp:2476` `name[MAX_PATH]`, `execName`) stay bounded — external verb command length remains safe degradation (no crash)

## Phase 4: Audit closure + verification
- [X] T008 Static closure sweep: re-grep `src/shellib.cpp`, `src/shellib.h`, `src/viewer2.cpp` for unbounded `strcpy`/`memcpy`/`sprintf` of a path (GetPath/dir/path/data->Name) into a fixed buffer; confirm zero remaining CRASH sites; append a Closure section to research.md
- [X] T009 Build Debug x64 + Release x64; clang-format touched files; rebuild clean
- [X] T010 Verification: `-a` navigation regression survives; if a fresh crash dump appears from the user, re-parse+symbolicate to confirm the fixed frames; document headless-env limits
- [X] T011 Commit to branch, fast-forward `main`, update memory notes

## Dependencies
- T002 → all. T003 (viewer) and T004–T007 (shell) independent. T008–T011 last.

## Strategy
Single pass, one commit (both dump-confirmed crash clusters), then build +
dump-forensic verify. MVP = T003 (F3) + T004/T005 (clipboard/drag CurDir).
