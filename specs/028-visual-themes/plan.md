# Implementation Plan: Switchable Visual Themes (Default + Dark)

**Branch**: `028-visual-themes` | **Date**: 2026-07-21 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/028-visual-themes/spec.md`

## Summary

Add an application-wide theme axis (Default / Dark) switched from a new
Options → Theme submenu, persisted in the registry, applied live. The
Default theme is a mechanical passthrough (pixel-identical to today).
Dark is delivered through: (1) a new built-in dark panel/viewer palette
plugged into the existing `CurrentColors` scheme system — covering
panels, captions, thumbnails, viewer, and all color-consuming plugins
for free; (2) a new theme engine (`src/themes.h/.cpp`) providing
`ThemeSysColor`/`ThemeSysColorBrush`/`ThemeDrawEdge` accessors that
replace draw-time `GetSysColor` in the owner-drawn chrome (menus,
toolbars, status lines, bottom bar, command line, tooltips); (3) a
centralized dark dialog layer hooked into the two dialog proc funnels
(`CDialog::CDialogProc`, `CPropSheetPage::CPropSheetPageProc`) that all
~107 dialogs flow through — WM_CTLCOLOR* + per-control `SetWindowTheme`
+ DWM dark title bars. **Strictly visual: no functional-core changes**
(user directive) — the only state added is one config DWORD; the switch
reuses the existing, battle-tested `ColorsChanged` rebuild pipeline.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; existing in-repo WinLib
(`src/common/winlib`), uxtheme (already linked), **dwmapi (new link
dependency, documented `DWMWA_USE_IMMERSIVE_DARK_MODE`)**; no external
libraries
**Storage**: Windows Registry — one new DWORD `Theme Mode` under the
existing `SALAMANDER_CONFIG_REG` key
**Testing**: saltests unit-test project (427 tests green today) + build
gates (Debug/Release x64) + user GUI walkthrough (quickstart.md)
**Target Platform**: Windows 11+ (per constitution)
**Project Type**: Desktop application (existing monolith + plugins)
**Performance Goals**: theme switch < 2 s (SC-001); no startup or panel
drawing regression (SC-007) — switch reuses the same rebuild path as
WM_SYSCOLORCHANGE, so cost is identical to an OS color change
**Constraints**: Default theme pixel-identical (SC-003); no
functional-core changes; no layout/metric/font changes; plugin ABI
unchanged; high contrast wins (FR-013)
**Scale/Scope**: ~20 chrome files converted to theme accessors, 2
central dialog procs hooked, 1 new module, 1 menu submenu, 1 config
value; 107 dialog templates covered centrally (no per-dialog edits)

## Constitution Check

*GATE: evaluated against constitution v1.1.0 — PASS (re-checked after
Phase 1 design — PASS).*

| Principle | Verdict | Notes |
|---|---|---|
| I. Build Reproducibility | PASS | No build-system changes; dwmapi.lib is a standard SDK lib added via `#pragma comment(lib)` |
| II. Backward Compatibility | PASS | Default theme unchanged & default-on; Dark is opt-in; plugin ABI untouched (`GetCurrentColor` semantics extended, signature identical); registry value absent ⇒ old behavior |
| III. Incremental Modernization | PASS | Additive module + mechanical accessor conversion; no refactoring of adjacent code; changes reviewable file-by-file |
| IV. Windows Platform Commitment | PASS | Pure WinAPI (DWM/uxtheme are Windows APIs); Win11+ only |
| V. Plugin Architecture Preservation | PASS | Plugins inherit theme through the existing color API + `PLUGINEVENT_COLORSCHANGED`; interfaces documented in contracts/theme-engine.md before modification |
| VI. UI Consistency | PASS (this feature is the intended case) | Constitution explicitly requires dark mode to be "a deliberate, versioned decision applied across the whole application" — exactly this feature. No per-module restyling; no `ICC_STANDARD_CLASSES`; standard themed controls kept |

## Project Structure

### Documentation (this feature)

```text
specs/028-visual-themes/
├── plan.md              # This file
├── research.md          # Phase 0 — decisions D1–D11
├── data-model.md        # Phase 1 — palettes, config field, caches, menu surface
├── quickstart.md        # Phase 1 — build/test gates + GUI walkthrough
├── contracts/
│   └── theme-engine.md  # Phase 1 — internal API contract + hook obligations
├── analysis/
│   └── visual-architecture-survey.md  # spec-phase survey
└── tasks.md             # Phase 2 (/speckit.tasks — not created by plan)
```

### Source Code (repository root)

```text
src/
├── themes.h                  # NEW — theme engine API (contract)
├── themes.cpp                # NEW — palettes, brush cache, DWM/dialog/ctlcolor hooks
├── consts.h                  # + DarkColors/DarkViewerColors/SchemeColors externs
├── salamdr1.cpp              # DarkColors data; brush/pen rebuild via ThemeSysColor;
│                             #   early ThemeMode read; class-brush swap; rebar bg
├── cfgdlg.h                  # + Configuration.ThemeMode
├── mainwnd2.cpp              # save/load ThemeMode; SchemeColors decoupling (load)
├── mainwnd3.cpp              # CM_THEME_* dispatch; popup check state; WM_SYSCOLORCHANGE HC refresh
├── menu4.cpp                 # Options → Theme submenu template rows
├── resource.rh2 / texts.rh2  # CM_THEME_*, IDS_MENU_OPT_THEME* IDs
├── lang/texts.rc2            # English strings
├── dialogs2.cpp              # NotifDlgJustCreated → ThemeApplyToDialog (both variants)
├── dialogs3-6.cpp            # draw-site conversions (waits, listview bk, owner-draw)
├── dialogs4.cpp              # SchemeColors decoupling (Colors page)
├── menu3.cpp, menubar.cpp    # menu chrome → ThemeSysColor/ThemeDrawEdge
├── toolbar2.cpp, toolbar3.cpp# toolbar chrome → accessors
├── stswnd.cpp                # status/dir-line body → accessors
├── filesbx1.cpp, filesbx2.cpp# panel edge + bottom bar → accessors
├── editwnd.cpp               # command line colors
├── tooltip.cpp, msgbox.cpp   # tooltip/msgbox → accessors + central-layer reconcile
├── finddlg1.cpp, finddlg2.cpp# find results custom draw → accessors
├── gui.cpp                   # owner-drawn buttons/checkboxes dark branch
├── svg.cpp                   # GetSVGSysColor → ThemeSysColor
├── viewer.cpp                # CurrentViewerColors; window class brush; DWM
├── zip.cpp                   # GetCurrentColor viewer indices → CurrentViewerColors
├── logo.cpp                  # splash/about exempt-handling (keep bitmap look)
├── pack3.cpp, packac.cpp     # listview bk / text conversions
└── common/
    ├── winlib.cpp            # CDialogProc: ThemeHandleCtlColor + DWM on WM_INITDIALOG
    └── sheets.cpp            # CPropSheetPageProc ditto + sheet-frame subclass + tab text

src/vcxproj/salamand.vcxproj  # + themes.cpp/h
saltests: new theme unit tests (palette coverage, contrast, passthrough)
```

**Structure Decision**: single new module + surgical edits in existing
files listed above; no new projects, no directory restructuring.

## Implementation Phases (execution order for /speckit.tasks)

1. **Core engine + palettes** (no visible change yet): themes.h/.cpp,
   DarkColors/DarkViewerColors data, ThemeMode config field +
   persistence + early read, SchemeColors decoupling,
   `UpdateCurrentColorsForTheme`, vcxproj registration.
2. **Menu + switch**: CM_THEME_* IDs/strings/template rows, dispatch →
   `SetThemeMode`, radio check state. After this phase panels/captions/
   viewer/plugins already switch (US1 partial, US2 complete).
3. **Chrome sweep**: accessor conversion of the D-tagged draw sites +
   GDI cache conversion + class-brush swap + rebar/imagelist/toolbar
   baking + SVG routing (US1 complete).
4. **Dialog layer**: ThemeHandleCtlColor in both procs,
   ThemeApplyToDialog from NotifDlgJustCreated, DWM title bars,
   sheet-frame subclass, msgbox/logo/gui reconcile, listview/treeview
   centralization (US3 complete).
5. **Imagery polish**: owner-drawn button/checkbox dark branches,
   glyph mask colors, icon-blend verification (US4 complete).
6. **Tests + verification**: saltests theme suite, Debug+Release
   builds, smoke run, clang-format, quickstart walkthrough handoff.

## Complexity Tracking

No constitution violations — table not required.
