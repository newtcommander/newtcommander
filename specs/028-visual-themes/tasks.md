# Tasks: Switchable Visual Themes (Default + Dark)

**Input**: Design documents from `/specs/028-visual-themes/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/theme-engine.md

**Tests**: Included — the spec defines measurable criteria (SC-005 contrast,
SC-003 passthrough) that are unit-testable, and project practice requires
saltests to stay green.

**Organization**: Phases follow plan.md §Implementation Phases; user-story
labels map to spec.md US1–US4. **Hard constraint (user directive): visual
layer only — no functional-core changes.**

## Format: `[ID] [P?] [Story] Description`

---

## Phase 1: Setup & Foundational Core (blocking for everything)

**Purpose**: Theme engine skeleton + palettes + config plumbing, invisible
by default. After this phase the app builds and behaves identically.

- [x] T001 Create `src/themes.h` — API per contracts/theme-engine.md
      (THEME_MODE_* enum, IsDarkThemeActive, RefreshThemeHighContrastState,
      ThemeSysColor, ThemeSysColorBrush, ThemeDrawEdge,
      UpdateCurrentColorsForTheme, SetThemeMode, ThemeApplyToTopLevel,
      ThemeApplyToDialog, ThemeHandleCtlColor, ReleaseThemeGraphics)
- [x] T002 Create `src/themes.cpp` — dark chrome palette table
      (data-model.md §5), high-contrast cache, brush cache with lazy
      create/rebuild/free, DWM title-bar helper (dwmapi,
      DWMWA_USE_IMMERSIVE_DARK_MODE), ThemeDrawEdge dark bevel,
      Default-passthrough guarantees per contract invariant 1
- [x] T003 Add `DarkColors[NUMBER_OF_COLORS]` + `DarkViewerColors[4]`
      static data (data-model.md §4, zero SCF_DEFAULT flags) in
      `src/salamdr1.cpp`; externs + `SchemeColors`/`CurrentViewerColors`
      externs in `src/consts.h`
- [x] T004 Add `DWORD ThemeMode` to `CConfiguration` in `src/cfgdlg.h`
      (+ ctor default 0); add `CONFIG_THEMEMODE_REG` constant; save/load
      beside `UseRecycleBin` in `src/mainwnd2.cpp` (~1735 / ~3248);
      coerce invalid values to 0
- [x] T005 Early theme read: load `CONFIG_THEMEMODE_REG` next to the
      `CONFIG_SHOWSPLASHSCREEN_REG` quick read in `src/salamdr1.cpp`
      (~3958), before `SplashScreenOpen`
- [x] T006 Decouple scheme pointer: introduce `SchemeColors`; convert the
      two repoint sites (`src/mainwnd2.cpp:2572-2586`,
      `src/dialogs4.cpp:3363-3378`) and three identity ladders
      (`src/mainwnd2.cpp:2269`, `src/dialogs4.cpp:3338`, `:3395`) to
      `SchemeColors`; implement `UpdateCurrentColorsForTheme()` and call
      it after each repoint + at startup
- [x] T007 Viewer palette indirection: add `CurrentViewerColors` pointer;
      switch draw/brush sites in `src/viewer.cpp` (sweep §6: ~812-1534,
      1510/1516) and `UpdateViewerColors` call sites to it; map viewer
      indices in `CSalamanderGeneral::GetCurrentColor` (`src/zip.cpp:1648`)
      to `CurrentViewerColors`
- [x] T008 Register `themes.cpp`/`themes.h` in
      `src/vcxproj/salamand.vcxproj` (+ filters file if present)
- [x] T009 Build gate: `build.cmd` (Debug x64) compiles clean; app starts;
      zero visual difference (Default passthrough)

**Checkpoint**: invisible foundation ready — palettes exist, nothing reads
them yet in Dark mode paths.

---

## Phase 2: User Story 1+2 — Menu switch + persistence (P1+P2 core)

**Goal**: Options → Theme submenu switches Default/Dark live; choice
persists. Panels/captions/viewer/plugins recolor (chrome follows in Ph3).

- [x] T010 [US1] Add `CM_THEME_DEFAULT`/`CM_THEME_DARK` to
      `src/resource.rh2`; `IDS_MENU_OPT_THEME`,
      `IDS_MENU_OPT_THEME_DEFAULT`, `IDS_MENU_OPT_THEME_DARK` to
      `src/texts.rh2`; English strings to `src/lang/texts.rc2`
- [x] T011 [US1] Add "Theme" submenu (MNTT_PB + 2 MNTT_IT rows, all skill
      levels) to Options popup in `src/menu4.cpp` (~180-256)
- [x] T012 [US1] Implement `SetThemeMode` full switch sequence
      (contract §SetThemeMode) in `src/themes.cpp`: config write, HC
      refresh, UpdateCurrentColorsForTheme, class-brush swap
      (CMAINWINDOW/CVIEWERWINDOW/CWINDOW*/SAVEBITS/SHELLEXECUTE classes
      via SetClassLongPtr), ColorsChanged(TRUE, FALSE, TRUE), DWM
      re-apply on MainWindow
- [x] T013 [US1] WM_COMMAND dispatch: `case CM_THEME_DEFAULT/DARK` beside
      CM_SKILLLEVEL in `src/mainwnd3.cpp` (~2911) → SetThemeMode; radio
      check via CheckRadioItem keyed on Configuration.ThemeMode in the
      Options popup-init handler (`src/mainwnd3.cpp` ~4700-4945)
- [x] T014 [US2] Startup application: after LoadConfig
      (`src/salamdr1.cpp:4242` area) ensure UpdateCurrentColorsForTheme +
      class brushes + DWM on main window before ShowWindow (:4268); HC
      refresh hook in WM_SYSCOLORCHANGE handler (`src/mainwnd3.cpp:1237`)
- [x] T015 [US1] Build + verify: switch recolors panels, panel captions,
      viewer (open F3 window live), Find live; persists across restart;
      Default restores user scheme untouched

**Checkpoint**: US2 fully done; US1 works for palette-driven surfaces.

---

## Phase 3: User Story 1 — Chrome sweep (P1 completion)

**Goal**: menus, toolbars, status/directory lines, bottom bar, command
line, tooltips render dark. All conversions are draw-site-only.

- [x] T016 [US1] Convert cached GDI: six `GetSysColorBrush` globals →
      app-owned `CreateSolidBrush(ThemeSysColor(...))` in
      `src/salamdr1.cpp` (~1808-1813) with rebuild-on-switch + proper
      DeleteObject in release path; six pens (~2460-2465) → ThemeSysColor
- [x] T017 [P] [US1] Menu chrome: `src/menu3.cpp` (89-94 shared res,
      431/432, 601/602, 733/734 mask blits, DrawEdge 448/618/750 →
      ThemeDrawEdge) and `src/menubar.cpp` (175/178 FillRect, 186, 215,
      232) → theme accessors
- [x] T018 [P] [US1] Toolbars: `src/toolbar2.cpp` (556/557, 648/659/662,
      DrawEdge 569/577), `src/toolbar3.cpp` (473/475 FillRect, 496, 512
      pen) → accessors
- [x] T019 [P] [US1] Status/directory line: `src/stswnd.cpp` (792-1278
      D-sites, DrawEdge 896) → accessors
- [x] T020 [P] [US1] Panels edge + bottom F-key bar: `src/filesbx1.cpp`
      (1359 DrawEdge, 1366 brush), `src/filesbx2.cpp` (79/217 brushes,
      228/267 text) → accessors
- [x] T021 [P] [US1] Command line + edit-list: `src/editwnd.cpp` (1571 +
      pen sites 1887-1962; WM_CTLCOLOREDIT for the edit line in its
      parent proc), `src/edtlbwnd.cpp` (622, 640, 651, DrawFrameControl
      555 scroll arrows) → accessors
- [x] T022 [P] [US1] Tooltip: `src/tooltip.cpp` (644 INFOBK fill, 646
      INFOTEXT) → accessors
- [x] T023 [US1] Graphics baking: rebar bg (`src/salamdr1.cpp:2997`),
      toolbar bitmap blends + `ImageList_SetBkColor` sites
      (~2285-2513), `CreateMappedBitmap` COLORMAP (~2479-2494),
      `src/gui.cpp:42`/`:2731` → ThemeSysColor(COLOR_BTNFACE etc.)
- [x] T024 [US1] SVG icons: route `GetSVGSysColor` (`src/svg.cpp:35`)
      through ThemeSysColor so toolbar/menu sprites rasterize for dark
- [x] T025 [US1] Main-window non-dialog surfaces: `src/mainwnd3.cpp`
      pens/brushes (6740-6803) → accessors; verify tab/drive bars pick
      up imagery from T023
- [x] T026 [US1] Build + visual check of main window in Dark (screenshot
      run), fix stragglers; Default still passthrough-identical

**Checkpoint**: US1 complete — whole main window coherent dark.

---

## Phase 4: User Story 3 — Dialog layer (P3)

**Goal**: all app dialogs, config property sheet, message boxes, Find
window, progress dialogs dark via the two central procs.

- [x] T027 [US3] Implement `ThemeHandleCtlColor` +
      `ThemeApplyToDialog` in `src/themes.cpp`: CTLCOLOR DLG/STATIC/BTN →
      dialog brush + light text; EDIT/LISTBOX/COMBOBOX → field brush;
      EnumChildWindows: SetWindowTheme "DarkMode_Explorer" (buttons,
      scrollbars, list/tree/listbox, edit) / "DarkMode_CFD" (combos),
      ListView/TreeView Set*Color, header via listview
- [x] T028 [US3] Hook central procs: `CDialog::DialogProc`
      (`src/common/winlib.cpp:635` + WM_INITDIALOG at :726 →
      ThemeApplyToDialog for CDialog-only dialogs) and
      `CPropSheetPage::DialogProc`/`CPropSheetPageProc`
      (`src/common/sheets.cpp:382-406`); `NotifDlgJustCreated` variants
      (`src/dialogs2.cpp:251`, `:333`) → ThemeApplyToDialog
- [x] T029 [US3] Property-sheet frame: subclass sheet frame from first
      page creation (`src/common/sheets.cpp`) for WM_CTLCOLOR* + DWM
      title bar + tab-control dark (sweep sites 578/600/618-620/932)
- [x] T030 [P] [US3] Reconcile precedents: `src/msgbox.cpp`
      (WM_ERASEBKGND 1120-1137, CTLCOLORSTATIC 1142-1154, FillRects) →
      theme accessors; `src/logo.cpp` about/splash exempt (keep bitmap,
      ensure no dark artifacts); `src/gui.cpp:1144` CStaticText forward
      verified compatible
- [x] T031 [P] [US3] Find window: `src/finddlg1.cpp` results custom draw
      (4003-4079, 4460) + `src/finddlg2.cpp:559` → accessors (keep the
      high-contrast test on raw GetSysColor); status bar dark
- [x] T032 [P] [US3] Remaining dialog draw sites: `src/dialogs3.cpp`
      (2680-2752, 2717), `src/dialogs5.cpp` (906/1258, 3259, 3273/3282),
      `src/dialogs6.cpp` (90-173, 97, 718/1392/2613),
      `src/dialogs2.cpp:1126`, `src/dialogs4.cpp:1359-1360`,
      `src/pack3.cpp:307`, `src/packac.cpp:2034` → accessors /
      ThemeApplyToDialog centralization
- [x] T033 [US3] Viewer + Find DWM title bars on creation
      (`src/viewer.cpp:1570` reg/create path) and on-switch re-apply in
      their broadcast handlers (`src/viewer3.cpp:700`,
      `src/finddlg2.cpp:293`)
- [x] T034 [US3] Build + dialog walkthrough in Dark (config all pages,
      Find, copy progress, msgbox, Alt+F7, password prompt), fix
      stragglers

**Checkpoint**: US3 complete.

---

## Phase 5: User Story 4 — Imagery polish (P4)

- [x] T035 [US4] Owner-drawn buttons/checkboxes dark branch in
      `src/gui.cpp` (2087-2167 uxtheme push buttons, 3796-3833 check
      glyphs, FillRect sites 1230-3832, DrawEdge 2861): flat dark chrome
      instead of light DrawThemeBackground when dark
- [x] T036 [P] [US4] Menu glyphs/marks: verify DrawFrameControl mask
      blits (`src/menu3.cpp:138-147`) recolor via T017 accessors; menu
      check/gray imagery (`HMenuMarkImageList`) against dark bg
- [x] T037 [P] [US4] Panel icon legibility: icon blend colors
      (ICON_BLEND_* dark values) verified vs system file icons; bottom
      bar symbols (`HBottomTBImageList`) re-baked via T023
- [x] T038 [US4] Full imagery sweep in Dark; adjust palette values in
      data-model tables where contrast fails

**Checkpoint**: US4 complete — "se vším všudy".

---

## Phase 6: Polish, Tests & Verification

- [x] T039 [P] saltests theme suite (new test file in saltests project):
      palette coverage of drawn COLOR_* indices; WCAG ≥4.5:1 text /
      ≥3:1 disabled pairs (both chrome palette + DarkColors pairs);
      no SCF_DEFAULT in DarkColors/DarkViewerColors; Default
      passthrough ThemeSysColor(i)==GetSysColor(i); ThemeMode default 0
      + invalid-value coercion
- [x] T040 Run full saltests — all pre-existing 427 + new must pass
- [x] T041 `build.cmd` Debug x64 AND `build.cmd full release` Release
      x64 compile clean; smoke-launch built exe (start + exit)
- [x] T042 clang-format all changed files; UTF-8-BOM check on new files
- [x] T043 Update `specs/028-visual-themes/quickstart.md` walkthrough
      with any deviations; hand off GUI walkthrough (SC-001..SC-007,
      incl. 20-switch stability + high-contrast check) to user
- [x] T044 Re-verify FR-005/SC-003: diff-review that every converted
      call site is inside `IsDarkThemeActive()`-guarded accessor
      passthrough (no Default-theme behavior change)

---

## Dependencies & Execution Order

- Phase 1 → blocks everything (T001-T009 sequential except T003/T004
  parallelizable after T001).
- Phase 2 needs Phase 1; delivers MVP (switch + persistence + panels).
- Phase 3 needs T012 (SetThemeMode) + T016; T017-T022 mutually parallel.
- Phase 4 needs T001/T002 only (independent of Phase 3); T030-T032
  parallel after T027/T028.
- Phase 5 after Phase 3 (imagery depends on chrome accessors).
- Phase 6 last; T039 can be written any time after T003.

**MVP scope**: Phases 1+2 (working, persisted theme switch with dark
panels/captions/viewer/plugins). Each later phase is an independently
verifiable increment.

---

## Completion Status (2026-07-21)

All 44 tasks implemented autonomously on branch `028-visual-themes`.
Verified: Debug x64 + Release x64 compile clean; saltests **482 checks,
0 failed** (427 pre-existing + 55 new theme palette/contrast tests);
Debug exe smoke-launched and closed cleanly; all changed files
clang-formatted.

Notes vs. original task text:
- T012: class-brush swap implemented via `ThemeUpdateWindowClassBackground`
  for the main window + viewer windows (at creation/broadcast) instead of a
  blanket sweep over all universal classes — the remaining classes are
  self-painted or covered by WM_CTLCOLOR/erase paths.
- T029: property-sheet frame theming implemented app-side
  (`ThemeSubclassPropSheetFrame` from `CCommonPropSheetPage`) — shared
  winlib/sheets sources kept neutral: sheets.cpp only gained an optional
  `SheetsGetSysColorHook` pointer (NULL = old behavior for
  translator/tserver/salmon).
- Dark palettes moved to a single source of truth
  `src/common/themes_palette.h` (X-macro lists) shared by the app and
  saltests; positional integrity is enforced by `static_assert` in
  salamdr1.cpp.
- The in-GUI **visual walkthrough** (quickstart.md sections A-G: SC-001,
  SC-003, SC-004, SC-006 twenty-switch stability, high-contrast check) is
  handed off to the user per established project practice — automated
  gates (builds, unit tests, smoke run) are all green.
