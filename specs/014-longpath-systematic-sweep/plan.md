# Implementation Plan: Systematic Whole-Program Long-Path Hardening

**Branch**: `014-longpath-systematic-sweep` | **Date**: 2026-07-18 | **Spec**: [spec.md](spec.md)

## Summary

Features 011–013 fixed long-path crash sites one dump at a time, but the
failures kept reappearing at the next fixed-size buffer downstream. A fresh
Release crash dump (2026-07-18 07:40, built 07:34 = the 013 binary) proves it:
F3 no longer crashes at the entry buffer that 013 widened, it now crashes one
frame deeper — `CViewerWindow::OpenFile` at `viewer2.cpp:676`
(`char fileName[MAX_PATH]; strcpy(fileName, file)`). A census found **~764
fixed-size `MAX_PATH`-class char buffers** in the core, ~90+ directly on a
path-copy route.

This feature stops the whack-a-mole: it **enumerates every fixed-size path
buffer across the whole application, classifies each, and fixes or safely
bounds all of them in one coordinated pass**, verified across the entire
operation surface rather than discovered by the next crash. The audit
(`research.md`, produced by 8 parallel per-subsystem passes) is the FR-008
deliverable and the regression baseline. Fixes apply the discipline proven in
011–013: widen a single stack/member buffer to `SAL_MAX_PATH_UTF8`, heap-back
buffers on recursive routes, eliminate redundant intermediates, and keep
external-API-bound and component-name buffers bounded (safe degradation).

## Technical Context

**Language/Version**: C++20 (`/std:c++latest`), MSVC v143 (VS2022), ANSI build (no `UNICODE`)
**Primary Dependencies**: Pure WinAPI; existing helpers `SAL_MAX_PATH_UTF8`, `Sal*` path/unicode/winlib wrappers; **no new external dependencies**
**Storage**: Registry/config for persisted panel paths & history (must round-trip long paths without truncation)
**Testing**: `build.cmd` (Debug x64) + `build.cmd full release` (Release x64), both clean; crash-dump forensic re-check against the Release PDB; static exhaustion sweep of the buffer inventory; command-line (`-a <path>`) navigation across the long-path test tree
**Target Platform**: Windows 11+ x64
**Project Type**: Desktop app — in-place buffer/lifetime fixes across the core
**Constraints**:
- Buffers on **recursive** routes (directory-tree walkers) must be heap, never a large stack array (98 KB × depth would overflow the 1 MB thread stack).
- **External-API** buffers (`ShellExecute`, `IShellLink`, `SHFileOperation`, shell "New", the `salextx64.dll` shared-memory IPC) stay `MAX_PATH` but must be **bounded** (no unbounded copy) → documented safe degradation, not a crash.
- **Plugin-ABI** buffers that are part of the fixed core↔`.spl` contract must not change size; verify they are bounded.
- **Component-name** buffers (single name ≤255) are left as-is.
- Zero behavior change for sub-260 paths.
**Scale/Scope**: ~30 core source files; the CRASH-verdict subset of the ~764 buffers (final count from the audit). No data-model/contract changes.

## Constitution Check

| # | Principle | Verdict |
|---|-----------|---------|
| I | Build Reproducibility | PASS — code-only; no toolchain/dep change |
| II | Backward Compatibility | PASS — sub-260 behavior identical; long paths now work/degrade instead of crashing; plugin ABI buffers preserved |
| III | Incremental Modernization | PASS — mechanical per-buffer fixes reusing established helpers; no rewrite |
| IV | Windows Platform Commitment | PASS — pure WinAPI, extended-length path model |
| V | Plugin Architecture Preservation | PASS — fixed-ABI `MAX_PATH` interface buffers explicitly preserved & bounded |
| VI | UI Consistency | PASS — no visual change; only removes crashes / spurious "too long" popups |

**Post-design re-check**: to be re-confirmed after `research.md` consolidation (no new violations expected — every fix is a size/lifetime change of an existing buffer).

## Project Structure

```text
specs/014-longpath-systematic-sweep/
├── spec.md
├── plan.md              # this file
├── research.md          # consolidated buffer audit (FR-008 deliverable) + dump forensics
├── quickstart.md        # operation × path-length verification matrix
├── audit/               # per-subsystem raw inventories (A..H) from parallel audit
│   ├── _BRIEF.md
│   └── A.md … H.md
├── tasks.md
└── checklists/requirements.md

src/  (fix targets — final set from research.md; known clusters:)
├── viewer.cpp, viewer2.cpp, viewer3.cpp           # A: viewer (F3) — OpenFile confirmed
├── fileswn6/8.cpp, worker.cpp, salamdr1.cpp, ...   # B: copy/move/delete/pack engine (recursion → heap)
├── fileswn0/3/4/5/9/a.cpp                          # C: file-op UI (view/edit/rename/attrs/size)
├── fileswn1/2/7.cpp                                # D: navigation / change-notify / directory line
├── salamdr2/3/4/5/6/7.cpp                          # E: core path & error primitives
├── mainwnd1..5.cpp, mainwnd.h                      # F: main window / persistence / history / title
├── shellib.cpp/.h, shellsup.cpp, shiconov.cpp      # G: shell / clipboard / drag-and-drop
└── dialogs*.cpp, finddlg*.cpp, drivelst.cpp, plugins*.cpp  # H: dialogs / find / drive bar / plugin handoff
```

**Structure Decision**: In-place buffer/lifetime fixes. `research.md` is the
audit deliverable; no `data-model.md`/`contracts/` (no new entities or
interfaces — the plugin ABI is explicitly held constant).

## Phase 0 — Research (the audit)

Eight parallel per-subsystem audit passes (A–H, brief in `audit/_BRIEF.md`)
enumerate every fixed-size path buffer, classify it
(CRASH / BOUNDED / COMPONENT / EXTERNAL / FIXED), and specify the concrete
fix. Consolidated into `research.md` with:
- **R0**: dump forensics (the 07:40 crash → `viewer2.cpp:676`) confirming the
  downstream-move pattern and the method.
- **R1**: the full CRASH-site inventory (the fix work-list), grouped by
  subsystem, each with file:line and fix.
- **R2**: recursion hotspots requiring heap (not stack) fixes.
- **R3**: the documented EXTERNAL/ABI safe-degradation set (buffers left
  `MAX_PATH`, verified bounded).

## Phase 1 — Design

No data model or contracts. `quickstart.md` defines the verification matrix
(each operation × {ASCII long, Unicode long, sub-260 regression}) and the
static-exhaustion check that proves no CRASH site remains.

## Complexity Tracking

> No constitution violations. The only notable complexity is breadth (many
> files); it is managed by the audit inventory (deterministic work-list) and
> the per-buffer classification discipline that prevents over-widening
> (recursion → heap, external/ABI → bounded, component → leave).
