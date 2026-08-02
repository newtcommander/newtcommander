# Implementation Plan: Dark Mode Stabilization

**Branch**: `049-dark-mode-stabilization` | **Date**: 2026-08-02 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/049-dark-mode-stabilization/spec.md`

## Summary

Stabilize the Dark theme by closing the 32 defects inventoried in
[`analysis/dark-mode-audit.md`](analysis/dark-mode-audit.md): promote the theme engine's
child-sweep into a public re-applicable helper and call it on every window-recreation path
(panel view switch, inline editors, late-created controls); rebalance the dark palette so input
surfaces sit lighter than the dialog face; replace the hardcoded hyperlink blue with the
palette's validated `COLOR_HOTLIGHT`; add engine branches/subclasses for the control classes
still rendering light (group boxes, radio glyphs, date/time pickers, hotkey, progress bars,
tooltips, listview checkboxes); close the audited plugin holes (peviewer adoption, propsheet
frame export as ABI 106, mdview Find, diskmap About); and harden the lifecycle paths
(High Contrast via `WM_SETTINGCHANGE`, `WM_THEMECHANGED` re-apply, brush-cache thread safety).
Native Win32 menus are deferred to a follow-up feature per clarification.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI (user32, comctl32 v6, uxtheme, dwmapi); internal theme
engine `src/themes.cpp|h` + shared palette `src/common/themes_palette.h`; no new external deps
**Storage**: Windows Registry (`Theme Mode` DWORD, unchanged)
**Testing**: `saltests` unit suite (palette/contrast assertions, 1133 checks); manual Dark-theme
GUI walkthrough per `quickstart.md`
**Target Platform**: Windows 11+ (x64 Debug/Release)
**Project Type**: Desktop application (main app + 18 enabled plugins)
**Performance Goals**: View-mode switch re-theming adds no perceptible latency (< 1 ms per
switch — one `EnumChildWindows` over ≤ 5 children); no flicker on subclassed paints
**Constraints**: Documented OS APIs only; Default theme byte-identical passthrough; High
Contrast wins; plugin ABI append-only (105 → 106); no functional side effects from paint
subclasses; UTF-8-BOM + clang-format
**Scale/Scope**: ~25 code sites across ~20 files in `src/` + 4 plugin sites + engine additions
+ saltests updates; 110 dialog templates in validation scope

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Assessment |
|---|---|
| I. Build Reproducibility | PASS — no build-system changes; all fixes are source-only |
| II. Backward Compatibility | PASS — plugin ABI extended append-only (105 → 106, one virtual appended after `ThemeHandleCtlColor`, `spl_vers.h` history row); plugins built for ≤ 105 keep loading; user-facing change is Dark-theme-only rendering (Default theme provably untouched, FR-020); registry format unchanged |
| III. Incremental Modernization | PASS — point fixes at audited sites; no refactors of adjacent code; new engine code follows existing subclass patterns |
| IV. Windows Platform Commitment | PASS — pure WinAPI; documented APIs only (`SetWindowTheme`, `DWMWA_USE_IMMERSIVE_DARK_MODE`, GDI custom draw); undocumented dark-mode ordinals remain rejected |
| V. Plugin Architecture Preservation | PASS — plugin interface documented before modification (contract in `contracts/`); ABI bump follows the 036 precedent exactly |
| VI. UI Consistency | PASS — this feature IS the deliberate, versioned application-wide visual decision the principle requires; all behavior lives in the central theme engine, never per-plugin styling side effects; no `ICC_STANDARD_CLASSES`, no manifests |

**Post-design re-check (after Phase 1)**: PASS — no new violations introduced by the design;
Complexity Tracking not needed.

## Project Structure

### Documentation (this feature)

```text
specs/049-dark-mode-stabilization/
├── spec.md                          # Feature specification (clarified)
├── plan.md                          # This file
├── research.md                      # Phase 0: design decisions D1–D14
├── data-model.md                    # Phase 1: palette & theming-state model
├── quickstart.md                    # Phase 1: build + validation walkthrough
├── contracts/
│   ├── theme-engine-additions.md    # Engine API delta (public helper, new branches, subclasses)
│   └── plugin-theme-api-v106.md     # ABI 106: ThemeSubclassPropSheetFrame export
├── analysis/                        # Six-track audit (input to this plan)
└── tasks.md                         # Phase 2 (/speckit.tasks — not created by plan)
```

### Source Code (repository root)

```text
src/
├── themes.cpp / themes.h            # Engine: ThemeApplyToWindowTree, new enum-proc branches,
│                                    #   group-box/radio/DTP-hotkey subclasses, brush pre-warm
├── common/themes_palette.h          # COLOR_WINDOW 32→56 rebalance (single source of truth)
├── common/sheets.cpp / sheets.h     # SheetsIsDarkHook (dark-aware tree theme)
├── common/winlib.cpp                # (verify only) validation MessageBox routing decision
├── filesbx1.cpp                     # A1: re-theme after ShowHideChilds; WM_CTLCOLOREDIT
├── fileswn5.cpp                     # A2: theme quick-rename edit at creation
├── edtlbwnd.cpp                     # A3: theme inline editor; B3: dark arrow button
├── dialogs4.cpp                     # A4 label edits, C7 GetSysColor, D3 swatches, E5 checkboxes
├── dialogs2.cpp                     # G2/G5: WM_THEMECHANGED/WM_SYSCOLORCHANGE re-apply; E5
├── dialogs3.cpp                     # C4 Change Icon owner-draw; D4 Drive Info colors
├── dialogs6.cpp                     # E5 checkboxes (Icon Overlays)
├── packac.cpp                       # A5: second ThemeApplyToDialog; E5 checkboxes; F5 msgbox
├── pack3.cpp                        # C5: CExecuteWindow dark erase
├── gui.cpp                          # D1 link color; D2 header SetTextColor
├── toolbar2.cpp / toolbar3.cpp      # D5: insert-mark pen, focus-rect colors
├── mainwnd3.cpp                     # G1: WM_SETTINGCHANGE HC refresh; G2: WM_THEMECHANGED
├── salamdr1.cpp                     # G4: UpdateViewerColors(CurrentViewerColors); hooks install
├── fileswn2.cpp / dialogs2.cpp      # F5: SalMessageBox conversions (safe sites)
├── saltests/saltests.cpp            # Palette/contrast assertion updates + new checks
└── plugins/
    ├── shared/spl_gen.h, spl_vers.h # ABI 106 append
    ├── shared/winliblt.cpp|h        # Central propsheet-frame subclass call
    ├── peviewer/peviewer.cpp        # F1: SetupWinLibTheme adoption
    ├── mdview/viewer.cpp            # F3: Find dialog theme touchpoints
    └── diskmap/DiskMap/GUI.AboutDialog.h  # F4: About dialog theme touchpoints
src/zip.cpp                          # CSalamanderGeneral::ThemeSubclassPropSheetFrame impl
```

**Structure Decision**: Single existing desktop-app tree; all changes land in the files above.
No new projects, no build-system changes.

## Complexity Tracking

No constitution violations — table not needed.
