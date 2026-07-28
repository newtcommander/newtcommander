# Implementation Plan: Fix Find Window Dark-Mode Rendering

**Branch**: `044-fix-find-dark-mode` | **Date**: 2026-07-28 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/044-fix-find-dark-mode/spec.md`

## Summary

Remove the six residual light-mode artifacts in the dark-theme Find
window (Ctrl+F): white etched separator lines, the white non-client edge
above the results list, the white border on the advanced-options box,
dark-on-dark text (list header labels, "Found Items", disabled
advanced-options text, disabled toolbar captions), the fully light
status bar, and the light progress bar shown during a search. All fixes
ride the feature-028 theme engine: four are **central** additions to
`ThemeApplyChildEnumProc`/`themes.cpp` (etched-static subclass,
status-bar subclass, Edit class → `DarkMode_CFD`, disabled-edit repaint)
and the rest are surgical Find-window edits (second idempotent
`ThemeApplyToDialog` pass after `WM_INITDIALOG`, `WM_NCPAINT` on
`CFindTBHeader`, header custom-draw text, status-bar owner-draw text
color, progress-bar colors, dark disabled-toolbar-text branch). Every
new paint path is keyed on `IsDarkThemeActive()` per message, so the
Default theme stays a bit-for-bit native passthrough and High
Contrast/live switching need no extra plumbing. Decisions R1–R10 in
[research.md](research.md).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; existing theme engine
`src/themes.h/.cpp` (feature 028); uxtheme + dwmapi + comctl32 already
linked — **no new dependencies**
**Storage**: None — no configuration change (theme mode DWORD from 028
is reused as-is)
**Testing**: saltests unit-test project (incl. existing WCAG-contrast
theme suite, `saltests.cpp:573-700`) + Debug/Release x64 build gates +
GUI walkthrough per quickstart.md against `temp/dark_find_window.png`
**Target Platform**: Windows 11+ (per constitution)
**Project Type**: Desktop application (existing monolith; core app only,
no plugin changes)
**Performance Goals**: No measurable paint-latency change; new work is
per-`WM_PAINT` branching in six small subclass procs, none on hot paths
**Constraints**: Default theme pixel-identical (subclasses defer to
`DefSubclassProc` when dark inactive); High Contrast wins via existing
`IsDarkThemeActive()` guard; no layout/metric/font changes; no dialog
template (`lang.rc`) changes; documented WinAPI only (no
`SetPreferredAppMode`/uxtheme ordinals)
**Scale/Scope**: 2 central files (`themes.cpp/.h`) + 3 Find/toolbar
files (`finddlg1.cpp`, `finddlg2.cpp`, `toolbar2.cpp`) + saltests
additions; ~6 root causes, ~10 touch sites, 0 new modules

## Constitution Check

*GATE: evaluated against constitution v2.0.0 — PASS (re-checked after
Phase 1 design — PASS).*

| Principle | Verdict | Notes |
|---|---|---|
| I. Build Reproducibility | PASS | No build-system, project, or dependency changes |
| II. Backward Compatibility | PASS | Purely visual defect fix inside the opt-in Dark theme; Default theme untouched (passthrough invariant); no config, ABI, or registry changes |
| III. Incremental Modernization | PASS | Surgical fixes at measured defect sites; no refactor of adjacent code; the status bar is repainted, not replaced (`CStatusWindow` swap explicitly rejected, R5) |
| IV. Windows Platform Commitment | PASS | Documented WinAPI only; undocumented dark-mode ordinals explicitly rejected (R7) |
| V. Plugin Architecture Preservation | PASS | No plugin API change; central-layer improvements flow to plugin dialogs through the existing exported `ThemeApplyToDialog` (`spl_gen.h:3469-3503`) with unchanged signatures |
| VI. UI Consistency | PASS | All restyling lives in the central theme engine as part of the versioned dark-mode decision (028) — the constitution's sanctioned path for app-wide visual change; no per-module hacks, no `ICC_STANDARD_CLASSES`, no manifest changes. The new Edit/status-bar subclasses are theme-engine infrastructure (extending 028's existing static-subclass precedent), not a module locally restyling standard controls |

## Project Structure

### Documentation (this feature)

```text
specs/044-fix-find-dark-mode/
├── plan.md              # This file
├── research.md          # Phase 0 — decisions R1–R10 + measured root-cause inventory
├── data-model.md        # Phase 1 — defect/surface inventory, palette mappings, state rules
├── quickstart.md        # Phase 1 — build/test gates + GUI walkthrough (SC-001..SC-004)
├── contracts/
│   └── theme-engine-additions.md  # Phase 1 — delta contract on the 028 theme engine
├── checklists/requirements.md     # spec-phase checklist (complete)
└── tasks.md             # Phase 2 (/speckit.tasks — not created by plan)
```

### Source Code (repository root)

```text
src/
├── themes.h                  # + decl of new subclass installers (if surfaced in API)
├── themes.cpp                # R1 etched-static subclass; R3 Edit → DarkMode_CFD;
│                             #   R4 disabled-edit dark repaint subclass;
│                             #   R5 msctls_statusbar32 branch + dark paint subclass
├── finddlg1.cpp              # R6 second ThemeApplyToDialog pass at end of WM_INITDIALOG;
│                             #   R5 SetTextColor in status-bar WM_DRAWITEM (:3823);
│                             #   R7 header NM_CUSTOMDRAW text in CFoundFilesListView;
│                             #   R9 dark progress-bar colors in SetTwoStatusParts (:1471)
├── finddlg2.cpp              # R2 CFindTBHeader WM_NCPAINT (ThemeDrawEdge);
│                             #   R4-adjacent: SetTextColor for "Found Items" DrawText (:568)
├── toolbar2.cpp              # R8 dark single-pass disabled text (replaces emboss when dark)
└── saltests/saltests.cpp     # contrast assertions for the new pairs
                              #   (GRAYTEXT/BTNFACE, BTNTEXT/BTNFACE on new surfaces)
```

**Structure Decision**: no new files unless a subclass needs a shared
declaration (then `themes.h` only); all changes are in-place edits to
the five listed sources plus tests. No resource (`lang.rc`) edits — all
fixes are runtime paint/theming behavior.

## Implementation Phases (execution order for /speckit.tasks)

1. **Central engine additions** (`themes.cpp`): etched-static subclass
   (R1), Edit → `DarkMode_CFD` (R3), disabled-edit repaint (R4),
   status-bar subclass (R5). After this phase the separators, edit
   border, and "No Advanced Options" text are fixed in every dialog.
2. **Find-window wiring** (`finddlg1.cpp`): re-apply
   `ThemeApplyToDialog` post-`WM_INITDIALOG` (R6 — activates the
   status-bar fix), status-bar owner-draw text color, progress-bar
   colors (R9).
3. **Find-window paint fixes** (`finddlg2.cpp`, `finddlg1.cpp`):
   `CFindTBHeader` NC edge (R2) + "Found Items" text color; results
   header custom-draw text (R7).
4. **Toolbar disabled text** (`toolbar2.cpp`): dark single-pass branch
   (R8).
5. **Tests + verification**: saltests contrast additions, Debug+Release
   builds, clang-format, quickstart walkthrough (all five window states)
   against the defect screenshot; light-mode side-by-side.

## Complexity Tracking

No constitution violations — table not required.
