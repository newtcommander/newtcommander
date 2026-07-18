# Implementation Plan: Long-Path Viewer & Shell Crash Fix

**Branch**: `013-longpath-shell-viewer-crash` | **Date**: 2026-07-18 | **Spec**: [spec.md](spec.md)

## Summary

Two dump-confirmed `STATUS_STACK_BUFFER_OVERRUN` crashes remain after feature
012 ([research.md](research.md)): (A) the F3 viewer thread copies the now-long
file name into a `MAX_PATH` buffer (`viewer2.cpp:50`); (B) the shell
drop-target/data-object machinery holds panel/source paths in
`MAX_PATH`/`2*MAX_PATH` buffers overrun by Unicode long paths (~540 B) —
`CImpDropTarget::SetDirectory` `CurDir` and siblings. Fix = widen those
specific buffers to `SAL_MAX_PATH_UTF8`, keeping guard/append sizes
consistent; genuine external-verb command length stays a bounded degradation.

## Technical Context

**Language/Version**: C++20, MSVC v143
**Primary Dependencies**: Pure WinAPI; `SAL_MAX_PATH_UTF8`, `Sal*` helpers; no new deps
**Storage**: n/a
**Testing**: `build.cmd` Debug + `build.cmd full release`; crash-dump forensic re-check; static sweep of shellib/viewer path buffers
**Target Platform**: Windows 11+ x64
**Project Type**: Desktop app — in-place fixes
**Constraints**: shell drop-target/data-object members become larger (single allocations, bounded cost); ShellExecute command length stays external-limit degradation; no behavior change for sub-260 paths
**Scale/Scope**: 2 files (`viewer2.cpp`, `shellib.cpp` + `shellib.h`), ~10 buffer sites

## Constitution Check

| # | Principle | Verdict |
|---|-----------|---------|
| I | Build Reproducibility | PASS — code-only |
| II | Backward Compatibility | PASS — sub-260 identical; long-path view/clipboard now work vs crash |
| III | Incremental Modernization | PASS — per-buffer fixes |
| IV | Windows Platform Commitment | PASS — pure WinAPI |
| V | Plugin Architecture Preservation | PASS — no plugin interface change |
| VI | UI Consistency | PASS — no visual change |

**Post-design re-check**: PASS.

## Project Structure

```text
specs/013-longpath-shell-viewer-crash/
├── plan.md, research.md, spec.md, checklists/requirements.md, tasks.md

src/
├── viewer2.cpp   # R1: viewer thread name/caption buffers
├── shellib.h     # R2: CImpDropTarget/data-object path members
└── shellib.cpp   # R2: SetDirectory + data-object/context-menu path buffers
```

**Structure Decision**: In-place buffer widening; research.md is the FR-005
audit deliverable; no data-model/contracts phase.

## Complexity Tracking

> No violations.
