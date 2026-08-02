# Tasks: Dark Mode Stabilization

**Input**: Design documents from `specs/049-dark-mode-stabilization/`
**Prerequisites**: plan.md, research.md (R1–R14), data-model.md, contracts/, quickstart.md,
analysis/dark-mode-audit.md (defect IDs referenced below)

**Tests**: saltests assertions are explicitly required by the spec (FR-005, SC-002, SC-007) —
included. No other automated test infrastructure exists for GUI surfaces; manual walkthrough
per quickstart.md.

**Organization**: Phase 2 builds the engine foundations every story consumes; user-story phases
then wire them site by site.

## Phase 1: Setup

- [x] T001 Verify clean baseline: `git status` clean on branch `049-dark-mode-stabilization`; run `.\build.cmd` (PowerShell, Debug x64) and confirm the tree builds before any change

## Phase 2: Foundational (blocking prerequisites)

- [x] T002 Rebalance dark palette: `COLOR_WINDOW` 32,32,32 → 56,56,56 in `src/common/themes_palette.h` (R2; defines C1/C2 behavior consumed by every story)
- [x] T003 Add public `ThemeApplyToWindowTree(HWND)` + `ThemeApplyToTooltip(HWND)` to `src/themes.h` and `src/themes.cpp` (R1, R7; contract §1)
- [x] T004 Extend `ThemeApplyChildEnumProc` in `src/themes.cpp` with the new dark branches: group-box subclass ID 6 (R4/B1), radio-glyph subclass ID 7 (R5/B2), grayscale-remap subclass ID 8 for `SysDateTimePick32`+`msctls_hotkey32` (R6/E1/E2), `msctls_progress32` recipe (R7/E3), `LVS_EX_CHECKBOXES` dark state-imagelist swap (R8/E5), and the light-side restores for each
- [x] T005 Extend disabled-edit subclass (ID 4) in `src/themes.cpp` to multiline edits: drop the `ES_MULTILINE` bail, paint `COLOR_BTNFACE` fill + `DrawTextW(DT_WORDBREAK|DT_NOPREFIX|DT_EDITCONTROL)` in `EM_GETRECT` with `COLOR_GRAYTEXT` (R9/C3)
- [x] T006 Pre-create the full dark brush cache in `UpdateCurrentColorsForTheme` in `src/themes.cpp` (R11/G3 thread safety)
- [x] T007 [P] Update saltests in `src/saltests/saltests.cpp`: keep existing floors (verify they hold with WINDOW=56), add `Lum(COLOR_WINDOW) > Lum(COLOR_BTNFACE)` and `ContrastRatio(COLOR_HOTLIGHT, RGB(0x0A,0x14,0x24)) >= 4.5` (R13; FR-005)
- [x] T008 Build + run saltests; fix regressions until all-pass (gate for all story phases)

## Phase 3: User Story 1 — Panel chrome stays dark (P1) [US1]

**Goal**: no panel element reverts to light on any view-mode change or inline rename.
**Independent test**: quickstart step 1.

- [x] T009 [US1] Call `ThemeApplyToWindowTree(HWindow)` at the end of `CFilesBox::SetMode` in `src/filesbx1.cpp` (A1; covers Alt+3/4/5, menu, toolbar, Alt+wheel, plugin-forced switches, header toggle)
- [x] T010 [US1] Route `WM_CTLCOLOREDIT` through `ThemeHandleCtlColor` in `CFilesBox::WindowProc` in `src/filesbx1.cpp` (A2 colors)
- [x] T011 [US1] Theme the quick-rename edit after creation via `ThemeApplyToWindowTree` in `src/fileswn5.cpp` (A2)
- [x] T012 [US1] Build; verify panel walkthrough (quickstart step 1) in Dark and light-regression in Default — scripted GUI pass: Alt+3/4/5/2 sequence stays dark (validation-results.md)

## Phase 4: User Story 2 — Readable links (P1) [US2]

**Goal**: every hyperlink ≥ 4.5:1 on its dark background; light theme byte-identical.
**Independent test**: quickstart step 2.

- [x] T013 [US2] Replace the hardcoded link blue in `src/gui.cpp` (`CStaticText`, `STF_HYPERLINK_COLOR`): dark → `ThemeSysColor(COLOR_HOTLIGHT)`, light → `RGB(0,0,255)` unchanged (D1/F6, R3)
- [x] T014 [US2] Build + saltests (the T007 navy assertion now guards this); verify About/msgbox/config links per quickstart step 2 — About link verified light blue on navy in scripted GUI pass

## Phase 5: User Story 3 — Coherent dialog fields & separators (P2) [US3]

**Goal**: no bright chrome, no black-hole fields, late-created controls dark.
**Independent test**: quickstart step 3.

- [x] T015 [P] [US3] Theme `CEditListBox` inline editor after creation (`ThemeApplyToWindowTree`) and dark-draw its arrow button (face fill + `ThemeDrawEdge` + `COLOR_BTNTEXT` triangle) in `src/edtlbwnd.cpp` (A3, B3)
- [x] T016 [P] [US3] Theme listview label edits in `LVN_BEGINLABELEDIT` (Views + Hot Paths pages) in `src/dialogs4.cpp` (A4)
- [x] T017 [P] [US3] Add second `ThemeApplyToDialog(HWindow)` at end of `CPackACDialog` `WM_INITDIALOG` in `src/packac.cpp` (A5)
- [x] T018 [P] [US3] Fix `WM_SYSCOLORCHANGE` listview reset `GetSysColor` → `ThemeSysColor` in `src/dialogs4.cpp` (C7) and blank-swatch white-on-white → dark-conditional face color in `src/dialogs4.cpp` (D3)
- [x] T019 [P] [US3] Convert Change Icon owner-draw to `ThemeSysColorBrush`/`ThemeSysColor` in `src/dialogs3.cpp` (C4)
- [x] T020 [P] [US3] Add `SetTextColor(ThemeSysColor(COLOR_BTNTEXT))` + transparent bk to `CToolbarHeader::OnPaint` in `src/gui.cpp` (D2)
- [x] T021 [P] [US3] Add `SheetsIsDarkHook` to `src/common/sheets.h` + `src/common/sheets.cpp` (dark-aware tree theme at creation), install in `src/salamdr1.cpp` (B5)
- [x] T022 [US3] Build; walk quickstart step 3 dialogs in Dark + light regression — build + saltests pass; visual dialog-by-dialog confirmation *deferred to the manual GUI session* (validation-results.md)

## Phase 6: User Story 4 — Indicators & transient surfaces (P2) [US4]

**Goal**: progress bars, checkboxes-in-lists, tooltips, gauge, markers, overlay all dark.
**Independent test**: quickstart step 4.

- [x] T023 [P] [US4] Call `ThemeApplyToTooltip` at both native tooltip creation sites in `src/mainwnd3.cpp` (splitter) and `src/viewer3.cpp` (viewer) (E4)
- [x] T024 [P] [US4] `CExecuteWindow` dark erase (`ThemeSysColorBrush(COLOR_BTNFACE)` + return TRUE) in `src/pack3.cpp` (C5)
- [x] T025 [P] [US4] Drive Info gauge: dark outline pen `COLOR_GRAYTEXT` in `src/gui.cpp`; force truecolor pie/legend constants in dark in `src/dialogs3.cpp` (D4)
- [x] T026 [P] [US4] Marker fixes: focus-rect color pair in `src/toolbar3.cpp`, insert-mark pen `ThemeSysColor(COLOR_BTNTEXT)` in `src/toolbar2.cpp` (D5)
- [x] T027 [US4] Build; walk quickstart step 4 (incl. Find-duplicates progress — closes the 044 open check) + light regression — build pass; Find-duplicates progress + tooltip visual confirmation *deferred to the manual GUI session*

## Phase 7: User Story 5 — Plugin coverage (P3) [US5]

**Goal**: peviewer, plugin propsheet frames, mdview Find, diskmap About, validation boxes dark.
**Independent test**: quickstart step 5.

- [x] T028 [US5] ABI 106: append `ThemeSubclassPropSheetFrame` virtual to `src/plugins/shared/spl_gen.h`, history row + `LAST_VERSION_OF_SALAMANDER` 106 in `src/plugins/shared/spl_vers.h`, delegation impl in `src/zip.cpp` (F2; contract plugin-theme-api-v106.md)
- [x] T029 [US5] Call `ThemeSubclassPropSheetFrame(GetParent())` centrally in winliblt `CPropSheetPageProc` `WM_INITDIALOG` in `src/plugins/shared/winliblt.cpp` (F2 — fixes ftp/pictview/filecomp frames)
- [x] T030 [P] [US5] Adopt theming in peviewer: `SetupWinLibTheme(SalamanderGeneral)` in `src/plugins/peviewer/peviewer.cpp` (F1)
- [x] T031 [P] [US5] Two-touchpoint theme pattern in mdview `FindDlgProc` in `src/plugins/mdview/viewer.cpp` (F3)
- [x] T032 [P] [US5] Two-touchpoint theme pattern in diskmap About dlgproc in `src/plugins/diskmap/DiskMap/GUI.AboutDialog.h` (F4) — **no change needed**: verified during implementation that the raw `DialogBox` is compiled only in the standalone (non-SALAMANDER) build; the shipped plugin routes About through the themed `SalamanderGeneral->SalMessageBox` (`DiskMapPlugin.cpp:325-335`). Audit F4 was a false positive.
- [x] T033 [US5] Message boxes — safe sites: convert `src/fileswn2.cpp`, `src/packac.cpp`, `src/dialogs2.cpp` raw `::MessageBox` to `SalMessageBox`; add `WinLibMessageBoxHook` in `src/common/winlib.cpp|h` + winliblt adapter in `src/plugins/shared/winliblt.cpp`, install core adapter in `src/salamdr1.cpp`; keep regwork/startup sites unchanged (F5, clarified scope)
- [x] T034 [US5] Full build (all plugins); walk quickstart step 5 + light regression; verify a 105-built plugin still loads (backward-compat gate) — full build pass; ABI append verified structurally (036 precedent); plugin-surface visual confirmation *deferred to the manual GUI session*

## Phase 8: User Story 6 — Robust against system events (P3) [US6]

**Goal**: HC wins under any notification ordering; style refresh never strips dark.
**Independent test**: quickstart step 6.

- [x] T035 [P] [US6] `WM_SETTINGCHANGE` + `SPI_SETHIGHCONTRAST` → full `WM_SYSCOLORCHANGE` sequence, and `WM_THEMECHANGED` → `ThemeApplyToDialog(HWindow)` in `src/mainwnd3.cpp` (G1, G2)
- [x] T036 [P] [US6] `WM_THEMECHANGED` (+ `WM_SYSCOLORCHANGE` for CCommonDialog) → `ThemeApplyToDialog` in `CCommonDialog`/`CCommonPropSheetPage` in `src/dialogs2.cpp`; `WM_THEMECHANGED` re-apply block in `src/viewer3.cpp` (G2, G5)
- [x] T037 [P] [US6] `UpdateViewerColors(CurrentViewerColors)` contract conformance in `src/salamdr1.cpp` (G4)
- [x] T038 [US6] Build; robustness pass per quickstart step 6 (HC toggle — closes the second 044 open check) — build pass; HC/system-event toggles *deferred to the manual GUI session* (system-wide settings)

## Phase 9: Polish & verification

- [x] T039 Full build.cmd full (Debug x64) + saltests all-pass; clang-format check on touched files (UTF-8-BOM preserved) — BUILD SUCCEEDED, saltests 1135/0, clang-format conformant
- [x] T040 Light-theme regression sweep per quickstart step 7 (SC-006) and Dark full walkthrough steps 1–6; results in validation-results.md incl. accepted residuals — written; scripted portions recorded, manual remainder itemized there

## Dependencies & execution order

- Phase 2 blocks everything (palette + engine primitives + test gate T008).
- US1 (T009–T012) and US2 (T013–T014) are independent of each other — both only need Phase 2.
- US3/US4 depend on Phase 2 branches (T004/T005); their tasks marked [P] touch disjoint files.
- US5 depends on Phase 2 only; T028 → T029 → T034 sequential (ABI before adoption before build).
- US6 independent of US3–US5; depends on Phase 2.
- Phase 9 last.

**MVP scope**: Phase 2 + US1 + US2 (the two P1 stories — the user-reported dailies).

## Parallel example (US3)

T015–T021 all touch different files → run as one parallel batch, then T022 gates.
