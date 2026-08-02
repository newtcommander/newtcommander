# Implementation Plan: Hot Path Display Names and Custom Icons

**Branch**: `047-hot-path-names-icons` | **Date**: 2026-08-02 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/047-hot-path-names-icons/spec.md`

## Summary

Make the hot path name an explicit, optional attribute (filled → shown on every
surface, empty → the directory path is shown, as today) and add a per-entry icon
chosen from a predefined gallery: the current default bookmark icon (shell32
resource 319, kept unchanged) plus 9 shipped color variants of a bookmark motif.
Technically this means: (1) changing the "assigned slot" marker from *Name
non-empty* to *Path non-empty* and routing all label lookups through a new
`GetDisplayName()` fallback helper; (2) adding an `IconIndex` to `CHotPathItem`
persisted as a per-slot `Icon` DWORD registry value; (3) extending the Hot Paths
config page with a Name edit box and an owner-drawn icon combo; (4) generating
the variant `.ico` assets through the existing `tools/brand/` pipeline and
loading them beside `HFavoritIcon`. The registry keeps writing the *effective
label* into the legacy `Name` value, so old and new builds interoperate and
existing configurations upgrade with zero visible change.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI (comctl32 ListView/ComboBox, custom
`CToolBar`/`CHotPathsBar`, `CMenuPopup`, `CDrivesList`); shell32.dll icon 319
(existing default); developer-side Python 3.13 `tools/brand/gen_icons.py` for
asset generation (never invoked by the build)
**Storage**: Windows Registry — `HKCU\Software\Tandem Commander\0.1\Hot Paths\<1..30>`
with values `Name` (REG_SZ), `Path` (REG_SZ), `Visible` (REG_DWORD) + new `Icon`
(REG_DWORD, optional, default 0)
**Testing**: `build.cmd` (Debug x64) + manual validation per `quickstart.md`
(project has no automated UI test infrastructure); registry round-trip checks
via `reg query`
**Target Platform**: Windows 11+ (light and dark theme, 100–200 % DPI)
**Project Type**: Desktop application (monolithic WinAPI exe + plugins; this
feature touches only the core exe)
**Performance Goals**: Imperceptible — 30 slots max; +9 `LoadImage` calls at
startup and on DPI/color change; menu/toolbar build stays O(30)
**Constraints**: No new external dependencies; icon artwork must be original
(GPLv2-compatible — Microsoft shell32 artwork must not be copied or recolored
for shipping); no config migration from other products; `src/lang/lang.rc`
changes ripple into 20 `.slt` translation modules (feature 038 tooling)
**Scale/Scope**: 30 hot path slots; 10-item icon gallery; 7 display surfaces
(Hot Path Bar, Change Drive menu ×2 panels, Go menu, drop-down menu,
directory-line assign submenu, jump list, settings list); ~12 source files
touched

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| # | Principle | Assessment |
|---|-----------|------------|
| I | Build Reproducibility | **PASS** — variant `.ico` files are generated developer-side by `tools/brand/gen_icons.py` and committed, exactly like the existing brand assets (feature 035); the build only compiles committed resources; no manual steps added. |
| II | Backward Compatibility | **PASS** — registry change is additive (`Icon` DWORD ignored by older builds); the `Name` value keeps its legacy on-disk semantics (always written as the effective label), so configs round-trip both directions; upgraded configs render identically (FR-010/SC-004); label behavior changes only when the user opts in by filling the Name field. No Open Salamander/Newt Commander keys touched. |
| III | Incremental Modernization | **PASS** — changes confined to hot-path code paths; adjacent code untouched; new helpers added to `CHotPathItems` rather than rewriting it. |
| IV | Windows Platform Commitment | **PASS** — pure WinAPI; no new dependencies; Windows 11+ only. |
| V | Plugin Architecture Preservation | **PASS** — hot paths are core-shell functionality (panels, main menus, drive menu); not viable as a plugin; plugin API untouched. |
| VI | UI Consistency | **PASS** — `IDD_CFGPAGE_HOTPATH` stays a `DIALOGEX` with `DS_SHELLFONT` and standard themed controls; the icon picker is an owner-drawn `COMBOBOX` (`CBS_OWNERDRAWFIXED`), which is functional owner-draw (icon swatches cannot be expressed otherwise), not restyling of a standard edit control; no process-wide visual behavior is altered. |

**Post-design re-check (after Phase 1)**: PASS — no violations introduced by the
design artifacts; Complexity Tracking left empty.

## Project Structure

### Documentation (this feature)

```text
specs/047-hot-path-names-icons/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   ├── registry-format.md   # Hot Paths registry schema + compat rules
│   ├── display-rules.md     # Label/icon resolution contract per surface
│   └── icon-set.md          # Gallery contract (indices, assets, extensibility)
└── tasks.md             # Phase 2 output (/speckit.tasks — NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
src/
├── mainwnd.h            # CHotPathItem/CHotPathItems: IconIndex, GetDisplayName(),
│                        #   GetIconIndex(), assigned-slot semantics
├── mainwnd1.cpp         # Save/Load (new Icon value, Name-as-label write, name==path
│                        #   load rule), FillHotPathsMenu, GetUnassignedHotPathIndex,
│                        #   direct quick-assign write, bar rebuild on color change
├── mainwnd2.cpp         # (registry value name constant only, if placed here)
├── fileswn1.cpp         # SetUnescapedHotPath / SetUnescapedHotPathToEmptyPos:
│                        #   stop auto-filling Name with the path
├── toolbar.h            # CHotPathsBar (no interface change expected)
├── toolbar7.cpp         # CreateButtons: skip on empty Path, label via
│                        #   GetDisplayName, per-item icon; tooltip stays Path
├── drivelst.cpp         # BuildData: drop non-empty-Name condition, label via
│                        #   GetDisplayName, per-item HIcon from gallery
├── jumplist.cpp         # Title via GetDisplayName
├── dialogs4.cpp         # CCfgPageHotPath: Name edit box, icon combo (owner-drawn),
│                        #   ListView icon imagelist, validation, delete resets icon
├── cfgdlg.h             # CCfgPageHotPath members for new controls
├── salamdr1.cpp         # Load/destroy HHotPathIcons[] beside HFavoritIcon
├── consts.h             # extern HHotPathIcons[], gallery size constant
├── salamand.rh          # IDI_HOTPATH_* resource IDs
├── salamand.rc2         # ICON statements for res\hotpath*.ico
├── res/
│   └── hotpath1.ico … hotpath9.ico   # committed generated color variants
└── lang/
    ├── lang.rh          # IDC_HOTPATH_NAME, IDC_HOTPATH_ICON, new string IDs
    └── lang.rc          # IDD_CFGPAGE_HOTPATH layout + new strings

tools/brand/
├── hotpath-master.png   # new master asset (bookmark motif, original artwork)
├── gen_icons.py         # extended: tint table → src/res/hotpath*.ico
└── README.md            # regeneration instructions

translations/<language>/salamand.slt   # regenerated/merged for the new
                                       # dialog controls + strings (tooling of
                                       # feature 038; languages per languages.cfg)
```

**Structure Decision**: Single monolithic desktop app — all changes live in the
core `salamand` project (no plugin, no new project files). Asset generation
stays developer-side in `tools/brand/` per the feature-035 convention.

## Complexity Tracking

> No Constitution Check violations — table intentionally empty.
