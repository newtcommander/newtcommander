# Implementation Plan: Dark Theme for Plugin Windows and Dialogs

**Branch**: `036-plugin-dark-theme` | **Date**: 2026-07-25 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/036-plugin-dark-theme/spec.md`

## Summary

Feature 028 built a complete theme engine (`src/themes.cpp`) but wired it
only into the core: its dark dialog layer hooks the core's two central
dialog procs (`src/common/winlib.cpp`, `src/common/sheets.cpp`), which
plugin DLLs do not compile. This feature extends the reach in three moves:
(1) **export the theme engine to plugins** — six new virtual methods
appended to `CSalamanderGeneralAbstract` (`spl_gen.h`), delegating to the
existing `themes.cpp` (single palette, no duplication), with the plugin
interface version bumped 104 → 105; (2) **hook the shared plugin WinLib**
(`src/plugins/shared/winliblt.cpp`) exactly the way 028 hooked the core
winlib — one central `WM_INITDIALOG`/`WM_CTLCOLOR*` touchpoint themes every
`CDialog`/`CPropSheetPage` in every plugin that compiles winliblt (ftp's 55
dialogs, pictview's 17, regedt, renamer, dbviewer, filecomp, checksum,
7zip, peviewer, uniso, undelete… at the cost of one `SetupWinLibTheme()`
call per plugin); (3) **sweep the remainder** — raw `DialogBoxParam`
dialogs (sftp, zip, uncab, diskmap) get the same two touchpoints per dialog
proc, and top-level plugin windows (mdview, pictview, dbviewer, filecomp,
sftp log, diskmap…) get the dark title bar plus chrome/content colors from
the new API (text/document content dark per clarification; images never
recolored). Theme adoption is at window creation only — reopen after a
switch is sufficient (clarified), so no live-repaint machinery is added.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; existing theme engine `src/themes.cpp` (028): `IsDarkThemeActive`, `ThemeSysColor(Brush)`, `ThemeApplyToDialog`, `ThemeApplyToTopLevel`, `ThemeHandleCtlColor`, DWM immersive dark title bar, `SetWindowTheme` dark variants
**Storage**: None new — `Configuration.ThemeMode` (`Theme Mode` DWORD, 028) is the single source of truth; plugins never persist theme state
**Testing**: Debug x64 `build.cmd` (all enabled plugins relink); runtime verification of representative plugin surfaces (SFTP dialogs, ftp connect, zip pack dialog, mdview/pictview/dbviewer windows) in Dark and Default; per-plugin code-level audit checklist
**Target Platform**: Windows 11+, x64
**Project Type**: Desktop application + 19 plugin DLLs (per `plugins.cfg`; the spec's "18" is the stale CLAUDE.md count — the authoritative set is whatever `plugins.cfg` enables)
**Performance Goals**: No measurable dialog-open slowdown (theming work is one child-enumeration per dialog creation, same as core dialogs since 028)
**Constraints**: Plugin ABI compatibility — methods appended at the END of `CSalamanderGeneralAbstract` only, `LAST_VERSION_OF_SALAMANDER` 104 → 105, old plugins keep loading; no `ICC_STANDARD_CLASSES`, no per-plugin manifests, no process-wide visual side effects (constitution VI); Default theme = exact passthrough (zero visual change)
**Scale/Scope**: ~150 plugin dialog templates across 19 enabled plugins (ftp 55, zip 26, pictview 17, regedt 9, renamer 9, sftp 9, dbviewer 6, undelete 6, filecomp 5, 7zip 4, uncab 4, checksum 3, peviewer 1, uniso 1) + ~8 top-level plugin window classes; the winliblt hook covers the ~12 winliblt-based plugins centrally

## Constitution Check

*GATE: evaluated pre-Phase 0, re-checked post-Phase 1 — PASS.*

| Principle | Verdict | Notes |
|-----------|---------|-------|
| I. Build Reproducibility | PASS | Code-only change; `build.cmd` builds core + all enabled plugins as today. |
| II. Backward Compatibility | PASS | Interface extended by appending virtuals at the vtable end + version bump (the documented, established pattern — `spl_vers.h` history). Old third-party plugins load and run unchanged (they never call the new slots); they simply stay light (spec FR-006). Default theme is a strict passthrough. |
| III. Incremental Modernization | PASS | Mechanism first, then SFTP, then per-plugin slices — each plugin's change is small, reviewable, revertible. |
| IV. Windows Platform Commitment | PASS | Same APIs 028 already uses (DWM attribute, `SetWindowTheme`); no new dependencies. |
| V. Plugin Architecture Preservation | PASS | The new interface methods are documented in `spl_gen.h` + `contracts/plugin-theme-api.md` BEFORE plugin changes; ABI append preserves binary compat. |
| VI. UI Consistency | PASS | This feature exists to enforce it. winliblt/plugin changes never call `InitCommonControlsEx(ICC_STANDARD_CLASSES)` or add manifests; all theming is per-window (`SetWindowTheme`/colors), no process-wide behavior change. |

## Project Structure

### Documentation (this feature)

```text
specs/036-plugin-dark-theme/
├── plan.md              # This file
├── research.md          # Phase 0 — mechanism decisions R1-R6
├── data-model.md        # Theme-state flow, per-plugin surface inventory
├── quickstart.md        # Build + runtime verification walkthrough
├── contracts/
│   ├── plugin-theme-api.md   # The 6 new CSalamanderGeneralAbstract methods
│   └── winliblt-theming.md   # SetupWinLibTheme contract for plugin authors
└── tasks.md             # Phase 2 (/speckit.tasks)
```

### Source Code (repository root)

```text
src/
├── plugins.h                      # CSalamanderGeneral: +6 method decls
├── plugins2.cpp (or sibling)      # impl delegating to themes.cpp
├── themes.h / themes.cpp          # unchanged public API (single palette owner)
└── plugins/
    ├── shared/
    │   ├── spl_gen.h              # +6 appended virtuals, documented
    │   ├── spl_vers.h             # LAST_VERSION_OF_SALAMANDER 104 -> 105 (+ history row)
    │   └── winliblt.h/.cpp        # SetupWinLibTheme() + central-proc hooks
    ├── sftp/dialogs.cpp, logs.cpp                # raw dlgprocs: 2 touchpoints each; log window chrome
    ├── zip/dialogs*.cpp                          # raw dlgprocs (26 templates)
    ├── uncab/dialogs.cpp                         # raw dlgprocs (4)
    ├── diskmap/…                                 # top-level window chrome
    ├── mdview/viewer.cpp                         # viewer window: dark title bar + dark document content
    ├── pictview/…                                # winliblt dialogs central; viewer chrome dark, image canvas untouched
    ├── dbviewer/…                                # winliblt + dark table content
    ├── filecomp/…                                # winliblt + dark compare panes
    └── <each enabled plugin entry>.cpp           # SetupWinLibTheme(SalamanderGeneral) call
```

**Structure Decision**: All new behavior lives in the existing theme engine
(core) and the shared plugin WinLib; per-plugin edits are deliberately
minimal (entry-point call, raw-dialog touchpoints, window-chrome color
swaps) so the sweep across 19 plugins stays reviewable per plugin.

## Complexity Tracking

No constitution violations — table intentionally empty.
