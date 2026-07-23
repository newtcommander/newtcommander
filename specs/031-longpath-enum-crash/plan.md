# Implementation Plan: Directory-Listing Crash on Long Multi-Byte Names — Review & Regression Protection

**Branch**: `031-longpath-enum-crash` | **Date**: 2026-07-23 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/031-longpath-enum-crash/spec.md`

## Summary

Entering a directory that contains an entry whose UTF-8 name exceeds 260
bytes (while its character count is legal, ≤ 255) smashes the /GS stack
cookie in the panel paint path — dump-confirmed in
`CFilesWindow::DrawIcon` (`fileswn4.cpp`), where
`memmove(fileName, f->Name, f->NameLen)` writes up to 765+4 bytes into a
`char fileName[MAX_PATH + 4]` (264 B) buffer. Fix: widen every
name-component buffer in the paint/icon path to the established
`SAL_FIND_NAME_U8 + 4` bound, guard plugin-supplied over-long names with a
graceful simple-symbol fallback, bound the unbounded extension-lowercase
loops, and repair the Tiles text-buffer layout. Fence the class with
compile-time `static_assert`s at each site plus new `saltests` checks
(conversion invariants + on-disk enumeration of the user's exact repro
name). Full inventory and rationale in [research.md](research.md).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; internal shared libs `src/common/`
(`salunicode`, `salpath`, `salfileio`); no new external dependencies
**Storage**: N/A (no persisted data touched)
**Testing**: `saltests` (custom single-file CHECK framework,
`src\saltests\saltests.cpp`, Debug x64, built by `build.cmd`, run as
console exe with exit code = failed checks)
**Target Platform**: Windows 11+, x64 (Win32 kept building)
**Project Type**: Desktop application (two-panel file manager)
**Performance Goals**: listing/painting a directory with long multi-byte
names indistinguishable from ordinary directories (spec SC-001: < 1 s,
100/100 crash-free)
**Constraints**: no behavior change for names ≤ 260 UTF-8 bytes
(Constitution II); no heap allocation added to the paint hot path; fix
stays incremental — no refactor of adjacent paint code (Constitution III)
**Scale/Scope**: 8 defect sites in 5 core files (`fileswn4.cpp`,
`fileswnb.cpp`, `salamdr4.cpp`, `filesbx1.cpp`, `fileswn0.cpp`) + new tests
in `src\saltests\saltests.cpp`

## Constitution Check

*GATE: evaluated against `.specify/memory/constitution.md` v1.1.0.*

| Principle | Verdict | Notes |
|-----------|---------|-------|
| I. Build Reproducibility | PASS | No build-system change; `build.cmd` remains the single entry point. |
| II. Backward Compatibility | PASS | Names ≤ 260 UTF-8 bytes take byte-for-byte identical code paths; wider buffers + guards only extend the working domain. No plugin ABI change (`CFileData` layout untouched). |
| III. Incremental Modernization | PASS | Point fixes at audited sites; no drive-by refactoring; `static_assert` fences added only at touched sites. |
| IV. Windows Platform Commitment | PASS | Pure WinAPI, no new dependencies. |
| V. Plugin Architecture Preservation | PASS | Plugin-supplied `CFileData` with names beyond the disk bound now degrades gracefully (simple symbol) instead of corrupting the stack — host protection, no interface change. |
| VI. UI Consistency | PASS | No dialogs/controls touched; rendering of ≤ 260-byte names unchanged. |

**Post-design re-check** (after Phase 1): still PASS — design introduces no
new projects, dependencies, or interface changes.

## Project Structure

### Documentation (this feature)

```text
specs/031-longpath-enum-crash/
├── spec.md              # Feature specification
├── plan.md              # This file
├── research.md          # R0 dump forensics, R2 site inventory, R3 fix decisions
├── data-model.md        # Name/buffer invariants (CFileData, SAL_FIND_NAME_U8)
├── quickstart.md        # Build, test, and live-verification steps
├── checklists/
│   └── requirements.md  # Spec quality checklist (done)
└── tasks.md             # Phase 2 output (/speckit.tasks — NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/
├── fileswn4.cpp         # DrawIcon (sites 1,2,5), DrawIconThumbnailItem (site 3)
├── fileswnb.cpp         # WM_USER_REFRESHINDEX static-assoc icon path (site 4)
├── salamdr4.cpp         # InternalGetType global ext buffer (site 6)
├── filesbx1.cpp         # Tiles hit-test GetTileTexts buffer layout (site 7)
├── fileswn0.cpp         # Tiles draw GetTileTexts buffer layout (site 8, sync with 7)
├── common/
│   └── salfileio.h      # SAL_FIND_NAME_U8 (bound reused; no change expected)
└── saltests/
    └── saltests.cpp     # + TestLongComponentNames, TestFileIO extension
```

**Structure Decision**: existing single-solution layout; only the five core
files above change, plus tests. No `contracts/` directory: the feature has
no external interface — the internal invariant ("name-component buffers hold
`SAL_FIND_NAME_U8 + 4` bytes") is documented in `data-model.md` and enforced
by `static_assert` at each site.

## Phase 0 — Research (complete)

All unknowns resolved in [research.md](research.md): exact crash site and
call chain from the user's own dumps (R0/R1), program-wide defect-site
inventory with CRASH/CORRUPTION/SAFE classification (R2), fix strategy with
alternatives (R3), three-layer regression fence (R4), performance impact
(R5). No NEEDS CLARIFICATION items remain.

## Phase 1 — Design

- **data-model.md**: the `CFileData` name invariants (UTF-8 byte lengths,
  765-byte component worst case, `Ext` aliasing into `Name`), the buffer
  sizing rule, and the guard/fallback semantics.
- **quickstart.md**: exact build (`build.cmd`), test
  (`saltests.exe`), dump-analysis, and live-repro verification steps.
- Agent context updated via `.specify/scripts/bash/update-agent-context.sh claude`.

## Phase 2 — Task planning approach (executed by /speckit.tasks)

Tasks will be generated in dependency order:

1. **Foundational**: regression tests first (new `TestLongComponentNames` +
   `TestFileIO` repro-name extension) — they must fail/pass meaningfully
   around the fix (US3, P3 but written first as the fence).
2. **US1 (P1)**: fix sites 1–3 and 5 (`fileswn4.cpp`) + site 6
   (`salamdr4.cpp`) + site 4 (`fileswnb.cpp`) — the listing/paint crash.
3. **US2 (P2)**: fix Tiles pair (sites 7–8) and re-verify the SAFE list
   entries flagged for re-check (`fileswn3.cpp:631`, `drivelst.cpp:869`).
4. **Verification**: Debug + Release x64 builds, full `saltests`, live repro
   on `D:\Temp` (all view modes), clang-format, code review of the diff.

## Complexity Tracking

No constitution violations — table not needed.
