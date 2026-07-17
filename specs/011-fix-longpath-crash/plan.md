# Implementation Plan: Fix Application Crash When Entering a Long-Path Directory

**Branch**: `011-fix-longpath-crash` | **Date**: 2026-07-18 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/011-fix-longpath-crash/spec.md`

## Summary

Entering a >260-char directory succeeds since feature 010, which exposes
post-navigation consumers still holding the current path in fixed
`MAX_PATH`-class buffers. The audit ([research.md](research.md) R2) found
**7 crash-capable sites** (one global corruptor firing on every path change —
`UpdateDefaultDir` — plus rename, recycle-bin delete, file-times, copy
script, listing-error report, unpack prefill) and 2 bounded-truncation sites.
Fix strategy per research R3: bound the global `DefaultDir` writes with a
drive-root fallback (rows must stay `MAX_PATH` for plugin consumers), widen
the operation-local buffers to `SAL_MAX_PATH_UTF8` with `sizeof`-based
guards, harden path-bearing `sprintf`s, and keep safe degradations
(startup-restore truncation) documented. Verification is fully automated
(scripted drive of the built app + static re-sweep), per the autonomous-run
mandate in the spec Clarifications.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; existing long-path infra from 004/010 (`SAL_MAX_PATH_UTF8`, `SalPathAppend`, `Sal*` file wrappers); no new dependencies
**Storage**: n/a (registry DefaultDirs section unchanged — rows stay ≤ MAX_PATH by design decision D1)
**Testing**: `build.cmd` Debug x64; scripted keystroke drive (`drive-enter-nav.ps1` + new action script) asserting process survival; static grep sweep
**Target Platform**: Windows 11+ (x64)
**Project Type**: Desktop application — in-place fixes in existing files
**Performance Goals**: None affected (buffer sizing only)
**Constraints**: `DefaultDir` rows MUST remain `MAX_PATH` (plugin ABI/consumers); stack arrays of `SAL_MAX_PATH_UTF8` only one per frame on UI thread (004 precedent); no behavior change for sub-260 paths
**Scale/Scope**: 7 crash fixes + 1 fidelity widening + 1 hardening across 7 files

## Constitution Check

| # | Principle | Verdict |
|---|-----------|---------|
| I | Build Reproducibility | PASS — code-only changes |
| II | Backward Compatibility | PASS — sub-260 behavior identical; long-path recycle-bin delete now reports the standard too-long-name error instead of corrupting memory |
| III | Incremental Modernization | PASS — per-site minimal fixes, no adjacent refactoring |
| IV | Windows Platform Commitment | PASS — pure WinAPI |
| V | Plugin Architecture Preservation | PASS — DefaultDir row size deliberately preserved for plugin consumers |
| VI | UI Consistency | PASS — no visual changes |

**Post-design re-check**: PASS (no new projects/deps/visual changes).

## Project Structure

### Documentation (this feature)

```text
specs/011-fix-longpath-crash/
├── plan.md              # This file
├── research.md          # Repro results + full audit table R1–R3
├── spec.md              # Requirements
├── checklists/requirements.md
└── tasks.md             # Phase 2 output
```

### Source Code (repository root)

```text
src/
├── mainwnd1.cpp    # UpdateDefaultDir (bounded + root fallback), EditWindowSetDirectory (widen)
├── fileswn5.cpp    # RenameFileInternal buffers, file-times buffer
├── fileswn6.cpp    # BuildScriptMain sourcePath widening
├── fileswn7.cpp    # Unpack prefill bounded copy
├── fileswn8.cpp    # DeleteThroughRecycleBin guard
├── fileswn3.cpp    # Listing-error _snprintf_s
└── bugreprt.cpp    # Bug-report sprintf hardening
```

**Structure Decision**: In-place fixes only; no new files. Data-model and
contracts phases are not applicable (no data entities, no interfaces) — the
audit table in research.md serves as the FR-006 deliverable.

## Complexity Tracking

> No violations — intentionally empty.
