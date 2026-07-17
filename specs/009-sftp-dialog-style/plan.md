# Implementation Plan: Consistent SFTP Plugin Dialog Appearance

**Branch**: `009-sftp-dialog-style` | **Date**: 2026-07-17 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/009-sftp-dialog-style/spec.md`

## Summary

Bring the SFTP plugin's dialog windows into visual conformance with the rest of Open
Salamander. The confirmed acceptance-defining difference is the focused text-input
decoration: on Windows 11 an SFTP edit shows a modern accent underline on focus, while the
core and the FTP plugin show the classic themed border. An initial change (dropping
`ICC_STANDARD_CLASSES` from the SFTP plugin init and giving all SFTP dialogs
`DS_SHELLFONT`) is on this branch, but the user reports the divergence persists on a clean
build, and a repo-wide search confirms no in-process module now registers
`ICC_STANDARD_CLASSES` — so that hypothesis is not confirmed as the true cause. The plan
therefore leads with an empirical root-cause spike (research.md), then removes the confirmed
divergence (not a per-plugin theming hack), and preserves consistency for future extensions
via the already-added constitution principle VI (documentation + code review, no automated
check).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (Visual Studio 2022)
**Primary Dependencies**: Pure WinAPI; comctl32 v6 and UxTheme visual styles supplied by the
host `salamand.exe` manifest; dialog templates live in the SFTP language module
(`english.slg`). (libssh2/WinCNG are SFTP runtime deps, irrelevant to styling.)
**Storage**: N/A (presentation-only change)
**Testing**: Manual side-by-side visual verification on a clean build; build verification via
`build.cmd`. No automated visual/consistency check (per spec clarification 2026-07-17).
**Target Platform**: Windows 11 (and Windows 10), pure WinAPI
**Project Type**: Desktop application — a plugin within a two-panel WinAPI file manager
**Performance Goals**: N/A (visual consistency only)
**Constraints**: No new external dependencies; no per-plugin manifest or theming/subclassing
hacks; must not regress the appearance of other dialogs; appearance must be deterministic
within a session (order-independent).
**Scale/Scope**: 8 SFTP dialog templates; 1 plugin-init flag; 1 constitution principle
(already added). Root-cause trigger is **to be empirically confirmed** in research.md (the
earlier `ICC_STANDARD_CLASSES` hypothesis is unverified after the user's clean-build report).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Assessment |
|-----------|------------|
| I. Build Reproducibility | PASS — source-only change built by `build.cmd`; no manual steps. |
| II. Backward Compatibility | PASS — SFTP functionality unchanged; the change only aligns appearance toward the rest of the app (a consistency fix), and must not alter other dialogs. |
| III. Incremental Modernization | PASS — small, SFTP-scoped, independently reviewable and revertible; no refactor of adjacent code. |
| IV. Windows Platform Commitment | PASS — pure WinAPI, targets Windows 11. |
| V. Plugin Architecture Preservation | PASS — SFTP plugin preserved; no interface changes. |
| VI. UI Consistency | PASS — this feature directly implements the principle; the fix removes a per-plugin styling divergence rather than adding one. |

No violations. Gate passes. (Re-checked after Phase 1 — still passes; the design adds no new
projects, dependencies, or per-plugin theming overrides.)

## Project Structure

### Documentation (this feature)

```text
specs/009-sftp-dialog-style/
├── plan.md              # This file (/speckit.plan output)
├── spec.md              # Feature spec (+ Clarifications)
├── research.md          # Phase 0: root-cause hypotheses, spike, decisions
├── data-model.md        # Phase 1: dialog inventory + house-style attributes
├── quickstart.md        # Phase 1: build & manual verification steps
├── contracts/
│   └── ui-house-style.md # Phase 1: observable UI contract (C1–C5)
└── tasks.md             # Phase 2 (/speckit.tasks — not created here)
```

### Source Code (repository root)

This feature modifies existing source; it introduces no new projects. Affected paths:

```text
src/plugins/sftp/
├── sftp.cpp             # plugin init (InitCommonControlsEx flags)
└── lang/lang.rc2        # 8 dialog templates (STYLE flags, DS_SHELLFONT)

.specify/memory/
└── constitution.md      # principle VI (UI Consistency) — durability record
```

Any additional change is contingent on the research spike (e.g. aligning the SFTP dialog
creation path or DPI/theme conditions with FTP/core if the spike shows those are the
trigger). Such change stays within `src/plugins/sftp/` or the shared plugin framework only
if it removes a divergence without side effects on other plugins.

**Structure Decision**: Existing repository layout; no new structure. Changes are confined
to the SFTP plugin and the constitution.

## Complexity Tracking

No constitution violations — table not applicable.
