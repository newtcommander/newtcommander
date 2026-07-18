# Implementation Plan: Long-Path File Operations

**Branch**: `012-longpath-file-operations` | **Date**: 2026-07-18 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/012-longpath-file-operations/spec.md`

## Summary

After features 004/010/011 made the panel path and navigation long-path
capable, *operating* on items inside a long path still crashes or refuses,
because each operation composes a full item path into a fixed
`MAX_PATH`-class buffer. The four-part audit ([research.md](research.md))
found the defects in five clusters: (R1) the copy/move/delete **script
builder** `fileswn6.cpp` — the copy crash; (R2) internal **view/edit** compose
buffers — the F3 error; (R3) the shared **shell binding** `GetShellFolder`
plus the drag `GetCurrentDir` — the clipboard/Properties/drag crash; (R4) the
**single-file build gate** `BuildName` and create-dir bound; (R5) **panel
persistence and change-notification** truncation — the "directory
disappeared" symptom. The engine's file I/O itself is safe (dynamic paths),
so there is no actual file-data loss. Fix strategy (research R6): widen the
compose/error buffers to `SAL_MAX_PATH_UTF8` and convert unbounded
`sprintf`/`strcpy`/`memmove`/`memcpy` to bounded forms; raise `BuildName`'s
gate; make config restore and change-notify long-path capable; and safely
degrade the genuine external-API limits (shell verbs, `IShellLink`, external
editor, Recycle Bin) to a bounded error. Verification per the autonomous /
headless constraints.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; existing long-path infra (`SAL_MAX_PATH_UTF8`, `SalPathAppend`, `Sal*` file wrappers, `CSalPathBuf`); no new dependencies
**Storage**: Windows Registry panel-path values (REG_SZ) — restore made long-path capable (D6); no schema change
**Testing**: `build.cmd` Debug + `build.cmd full release`; `-a` startup navigation; static exhaustion of the audited sinks (interactive keystroke automation unavailable in the headless env, feature 011 R5); crash-dump parser tooling on standby
**Target Platform**: Windows 11+ (x64)
**Project Type**: Desktop application — in-place fixes
**Performance Goals**: None affected (buffer sizing + bounding only)
**Constraints**: worker-thread frames have 1MB stacks (SAL stack arrays ok, but prefer heap/shared where several coexist); genuine external-API limits must degrade, not be faked; no behavior change for sub-260 paths; `DefaultDir`-style plugin-facing structures keep MAX_PATH
**Scale/Scope**: ~30 audited sites across ~8 files (fileswn6, fileswn5, fileswnb, shellib, shellsup, salamdr1, mainwnd2, worker.h/dialogs.cpp + viewer.cpp)

## Constitution Check

| # | Principle | Verdict |
|---|-----------|---------|
| I | Build Reproducibility | PASS — code-only changes |
| II | Backward Compatibility | PASS — sub-260 behavior identical; long-path ops now work or degrade cleanly instead of crashing |
| III | Incremental Modernization | PASS — per-site buffer fixes, no adjacent refactoring |
| IV | Windows Platform Commitment | PASS — pure WinAPI |
| V | Plugin Architecture Preservation | PASS — no plugin interface change; plugin-facing MAX_PATH structs preserved |
| VI | UI Consistency | PASS — no visual change; bounded error messages reuse existing strings |

**Post-design re-check**: PASS (no new projects/deps/visual changes).

## Project Structure

### Documentation (this feature)

```text
specs/012-longpath-file-operations/
├── plan.md              # This file
├── research.md          # Four-part audit R1–R6 (operation-audit table = FR-008 deliverable)
├── spec.md              # Requirements + clarifications
├── checklists/requirements.md
└── tasks.md             # Phase 2 output
```

### Source Code (repository root)

```text
src/
├── fileswn6.cpp     # R1: script builder (copy/move/delete/size) error+compose buffers — THE copy crash
├── salamdr1.cpp     # R1/R4: BuildName MAX_PATH gate (single-file copy/delete)
├── shellib.cpp      # R3: GetShellFolder strcpy sink (Properties/New/clipboard/drag/menu)
├── shellsup.cpp     # R3: GetCurrentDir memcpy-before-guard (drag target)
├── fileswn5.cpp     # R2/R4: ViewFile/EditFile compose, external cmdLine, CreateDir bound
├── viewer.cpp       # R2: viewer name intake buffer
├── fileswnb.cpp     # R2: viewer next/prev enum channel
├── consts.h         # R2: viewer enum struct field sizes (FileName/LastFileName)
├── worker.h         # R5: COperations::WorkPath1/2 change-notify buffers
├── dialogs.cpp      # R5: workPath1/2 change-notify copies
├── mainwnd.h/mainwnd3.cpp # R5: CChangeNotifData::Path + dispatch
├── mainwnd2.cpp     # R5: panel-path config restore truncation
└── fileswn7.cpp     # R5: AcceptChangeOnPathNotification GetPath truncation
```

**Structure Decision**: In-place fixes only; no new files. The research.md
audit table is the FR-008 operation-audit deliverable; no data-model/contracts
phase (no entities/interfaces).

## Complexity Tracking

> No violations — intentionally empty.
