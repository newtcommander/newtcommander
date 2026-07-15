# Tasks: Correct Display of Unicode File Names in Dialogs and Text Fields

**Input**: Design documents from `/specs/005-fix-unicode-display/`
**Prerequisites**: plan.md, spec.md, research.md (audit inventory A/B/C/D/E), data-model.md, contracts/ui-text-contract.md, quickstart.md

**Tests**: No automated UI test infrastructure exists (see plan.md Technical Context); no test tasks generated. Every phase ends with a manual verification checkpoint mapped to the quickstart matrix.

**Organization**: Tasks grouped by user story. Inventory IDs (A1–A9, B1–B17, C1–C3, D1–D3, E1–E10) and decisions (D1–D8) refer to [research.md](research.md).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 (P1 rename dialog), US2 (P2 all surfaces render correctly), US3 (P3 input fidelity)

## Path Conventions

Single native-app codebase: all paths relative to repository root `E:\Projects\salamander\`.

---

## Phase 1: Setup

**Purpose**: Baseline build + reproducible defect state

- [ ] T001 Verify fixtures per quickstart.md (`%TEMP%\salamander-test` NFC/NFD `č-dir` pair + unicode sample set; recreate if missing), run `build.cmd full`, launch the built binary, and confirm the baseline repro: F2 on the two `č-dir` directories shows `ÄŤ-dir` / `cĚŚ-dir` (record in notes)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Central helpers + shared plumbing every story's fixes call into

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [ ] T002 Add central UTF-8 window-text helpers `SalSetWindowTextU8`, `SalGetWindowTextU8`, `SalSetDlgItemTextU8`, `SalGetDlgItemTextU8`, `SalComboAddStringU8` in src/common/winlib.h + src/common/winlib.cpp (decisions D2/D3: `SalU8ToWAlloc` → `W` message; read-back `GetWindowTextW` + `SalWToU8` with 3×WCHAR+1 sizing; invalid-UTF-8 ANSI fallback identical to `CTransferInfo::EditLine` at winlib.cpp:1042-1055; guard with `#if defined(INSIDE_SALAMANDER) && !defined(_UNICODE)` like EditLine)
- [ ] T003 [P] Fix `LoadComboFromStdHistoryValues` in src/salamdr6.cpp:383-391 (inventory A7): convert each history entry via `SalU8ToWAlloc` and add with `SendMessageW(combo, CB_ADDSTRING, …)`; ANSI fallback for invalid UTF-8
- [ ] T004 [P] Fix `CTruncatedString::TruncateText` in src/salamdr4.cpp:157+ (inventory A8, decision D5): convert `Text` to UTF-16 once, measure with `GetTextExtentExPointW`, truncate at WCHAR boundaries without splitting surrogate pairs, re-encode to valid UTF-8 (or store a parallel wide result); both `forMessageBox` and dialog paths
- [ ] T005 [P] Drop invalid-UTF-8 history entries at registry load in src/mainwnd2.cpp (`LoadHistory` counterpart of `SaveHistory` at mainwnd2.cpp:2054-2077; decision D6): validate each loaded string with the existing UTF-8 validity check (`SalU8ToW` failure) and skip the entry
- [ ] T006 Build checkpoint: `build.cmd` compiles clean; app starts; no visible behavior change yet (helpers unused outside T003–T005)

**Checkpoint**: Foundation ready — user story implementation can begin

---

## Phase 3: User Story 1 - Rename dialog shows and preserves the true name (Priority: P1) 🎯 MVP

**Goal**: F2 shows the exact stored name for both composition forms; unchanged confirm is a byte-exact no-op; edited names apply with full fidelity. (Also transparently fixes F5/F6/F7, which share `CCopyMoveDialog` — verified under US2/US3.)

**Independent Test**: quickstart.md rows #1–#6 on the live `č-dir` fixtures; code-point comparison before/after no-op confirm.

### Implementation for User Story 1

- [ ] T007 [US1] Fix `CCopyMoveDialog::Transfer` history branch in src/dialogs3.cpp:419-444 (inventory B1): `ttDataToWindow` → `SendMessageW(hWnd, CB_LIMITTEXT, …)` + set text from `SalU8ToWAlloc(Path)` via `WM_SETTEXT` W (or `SalSetWindowTextU8`); `ttDataFromWindow` → wide read + `SalWToU8` into `Path` (`PathBufSize` bytes) before `AddValueToStdHistoryValues`; keep the no-history branch on `ti.EditLine` unchanged
- [ ] T008 [US1] Fix name-validation loop in `CFilesWindow::RenameFileInternal` src/fileswn5.cpp:2113-2116 (inventory C1, decision D4): iterate as `unsigned char`; reject only ASCII control bytes (1–31) and the reserved set `\ / : < > | "`; bytes ≥ 0x80 always pass
- [ ] T009 [US1] Verify SC-001/SC-003 per quickstart.md rows #1–#6: F2 shows `č-dir` for both NFC and NFD directory; unchanged OK = byte-identical name (PowerShell code-point check) with no error; edit to `řž-dir` applies exactly; rename to `Тест-测试` succeeds; history dropdown shows past Unicode entries unmangled; record results

**Checkpoint**: The reported bug is fixed and independently verified — MVP deliverable

---

## Phase 4: User Story 2 - Every name-bearing text surface renders Unicode correctly (Priority: P2)

**Goal**: Close every remaining DEFECTIVE row of the audit inventory (core dialogs, framework chrome, message boxes, plugin shared layer, plugin dialogs) and resolve every UNCERTAIN row.

**Independent Test**: quickstart.md rows #7–#17 and #19 with the sample-name set (NFC, NFD, Greek, Japanese, emoji).

### Core dialogs (per-file tasks; all use T002 helpers, D3 read-back rule)

- [ ] T010 [P] [US2] Fix remaining src/dialogs3.cpp sites: `CCopyMoveMoreDialog::Transfer` 608-628 (B2), `CEditNewFileDialog` WM_GETTEXT 555 (B3), `CChangeDirDlg::Transfer` 1185-1192 (B4), convert/filter masks 113/117/316/320 + resolve 77 (B8), plugin-FS quick-rename read-back 1258 (B12), network-path static 1804 (B13), ANSI subject statics 1972/2148 (B14), conversion-tables list items 2866-2870 (B17-part)
- [ ] T011 [P] [US2] Fix Make File List dialog in src/dialogs.cpp: history combo + WM_SETTEXT/WM_GETTEXT 1873/1878/1883, validation read-backs 1900/1919/2005/2008 (B5)
- [ ] T012 [P] [US2] Fix src/dialogs2.cpp sites: Select/Deselect mask combo 538/563/565/569 (B6), user-menu compare-arguments path 1229/1233 (B9-part), resolve HelpDir statics 837/840 (B17-part)
- [ ] T013 [P] [US2] Fix src/dialogs4.cpp sites: user-menu config command/arguments/init-dir 2148-2153 + 2178-2182 (B9), view-template list item 1022 + in-place edit 1508-1518, hot-path list name 2768 + in-place edit 3060-3072 (B15-part)
- [ ] T014 [P] [US2] Fix src/dialogs5.cpp sites: external viewer/editor + pack command/init-dir 2056-2084, 2443-2470 (B10); resolve archive-name/edit sites 1422, 2917, 2966 (B17-part)
- [ ] T015 [P] [US2] Fix pack/associations config pages in src/dialogsp.cpp: command/path WM_SETTEXT/WM_GETTEXT 157-232, 606-661, 949-1048, viewer/editor CB_ADDSTRING 1117-1150 (B11)
- [ ] T016 [P] [US2] Fix Find dialog in src/finddlg1.cpp: Named/Look-in/Grep read-backs 1685/1699/1749-1753 (B7); resolve 2758 (drives string), 3551, and status-bar 2988 (B17-part)
- [ ] T017 [P] [US2] Fix src/dialogs6.cpp list views: shared-directories items 506-510, connections name+path 1294/1298 (B15-part); resolve icon-overlay list 2527-2530 (B17-part)
- [ ] T018 [P] [US2] Fix packers/associations custom-draw text in src/packac.cpp:186 (B15-part): supply item text via `LVM_*` W path (`NMLVDISPINFOW`) or convert before assignment
- [ ] T019 [P] [US2] Fix `CMessageBox` in src/msgbox.cpp (A1): caption `SetWindowTextW` 384, text `SetDlgItemTextW` 387, ANSI `DrawText` measurement/paint 588/598/613 → wide, button labels 809/837; all via `SalU8ToW*` with ANSI fallback
- [ ] T020 [P] [US2] Resolve remaining UNCERTAIN core sites (B17): src/salamdr3.cpp 3150/3635/3802/3804, src/salamdr2.cpp 223/225, src/codetbl.cpp 668, src/shellsup.cpp 2028/2091, file-properties dialog name control (locate in src/dialogs2.cpp), plugin list src/plugins2.cpp 1055-1062 — classify each (fix if it can carry a name; record verdict in research.md inventory)

### Core chrome (framework classes)

- [ ] T021 [P] [US2] Convert `CStatusWindow` (panel directory + info line) in src/stswnd.cpp + src/stswnd.h (A2): wide measurement (`GetTextExtentExPointW` 188/274), wide paint (`ExtTextOutW` 1004-1104), keep `Text` UTF-8 but compute hot-track segment offsets on the wide form (or maintain byte↔wchar map); `GetHotText`/clipboard copy 610/1802 stays UTF-8
- [ ] T022 [P] [US2] Convert `CStaticText` in src/gui.cpp + src/gui.h (A3): store/refresh a parallel wide string on `SetText` 548, measure in `PrepareForPaint` 616 and paint 1132/1153 wide; progress-dialog Source/Target/Operation (fed from src/dialogs.cpp:473-486) then renders correctly with no caller changes
- [ ] T023 [P] [US2] Convert custom menu text drawing in src/menu1.cpp (measure 331-345), src/menu3.cpp (draw 935-1001), src/menubar.cpp (258), and `FillMenuHandle` `InsertMenuItemW` in src/menu2.cpp:1019-1020 (A4) — covers directory-history, drive (Alt+F1/F2), hot-paths, user-menu, plugin menus
- [ ] T024 [P] [US2] Convert toolbar text drawing in src/toolbar2.cpp:299/633/639, src/toolbar3.cpp:498, src/toolbar4.cpp:1421 (A5) — hot-path bar names, drive labels
- [ ] T025 [P] [US2] Convert tooltip drawing in src/tooltip.cpp:323/640 and verify/convert `TTN_GETDISPINFO` sites src/mainwnd3.cpp:5077, src/viewer3.cpp:561 (A6)
- [ ] T026 [P] [US2] Convert command-line combo (`CEditWindow`) in src/editwnd.cpp: execute read-back 375, set-text 561, history fill `CB_ADDSTRING` 1711, LastText tracking 1332/1986/1997, Ctrl+Backspace word ops 219 (A9)
- [ ] T027 [P] [US2] Fix signed-char cache-name validation loops in src/zip.cpp:2458-2460 (`ViewFileInPluginViewer`, C2) and src/zip.cpp:3190-3192 (`MoveFileToCache`/`GetFileFromCache`, C3) per decision D4 — unblocks viewing Unicode-named archive entries

### Plugin shared layer

- [ ] T028 [US2] Fix plugin-shared transfer library src/plugins/shared/winliblt.cpp + winliblt.h (D1): `CTransferInfo::EditLine(char*)` WM_SETTEXT 1063 / WM_GETTEXT 1071 → `SplU8ToWAlloc`+`SendMessageW` / wide read+`SplWToU8` (include splunicode.h honoring the 004 include-order caveat: after windows-dependent headers, before C headers that `#define BOOL`); add shared `SetDlgItemTextU8`/`GetDlgItemTextU8` helpers to winliblt.h/.cpp centralizing the zip/splitcbn pattern
- [ ] T029 [P] [US2] Fix `HistoryComboBox` in src/plugins/shared/lukas/utildlg.cpp:40-98 (D2): narrow `ti.EditLine` 52 (fixed via T028), `CB_ADDSTRING` 88 and `WM_SETTEXT` 95 → wide with `SplU8ToW*`
- [ ] T030 [P] [US2] Document the UTF-8 dialog-text contract in src/plugins/shared/spl_gen.h (D3): comments on `AddValueToStdHistoryValues`/`LoadComboFromStdHistoryValues` (~2782-2790) and general dialog guidance per contracts/ui-text-contract.md

### Plugin-specific sites

- [ ] T031 [P] [US2] Fix ftp plugin: bookmark listbox `LB_INSERTSTRING`/`LB_ADDSTRING` src/plugins/ftp/dialogs1.cpp:894/1199/1231 and narrow `SetDlgItemText` name/host/path fields across src/plugins/ftp/dialogs*.cpp (E1) using T028 helpers
- [ ] T032 [P] [US2] Fix regedt plugin file-path fields src/plugins/regedt/dialogs.cpp:950-983, 1222, 1234 (E5) — switch to `EditLineW`/U8 helpers matching its existing key-name pattern
- [ ] T033 [P] [US2] Fix wmobile plugin: narrow `SetDlgItemText` name sites in src/plugins/wmobile/dialogs.cpp (7×) and signed-char loop src/plugins/wmobile/fs2.cpp:524 (E6, D4)
- [ ] T034 [P] [US2] Fix pictview plugin: signed-char loop src/plugins/pictview/render1.cpp:2807 (E4, D4); path fields dialogs.cpp:1128/1835/1889 come via T028 — spot-verify
- [ ] T035 [P] [US2] Fix demoplug SDK sample: src/plugins/demoplug/dialogs.cpp:250 and signed-char loop src/plugins/demoplug/fs2.cpp:933 (E8) — sample doubles as SDK documentation of the pattern
- [ ] T036 [P] [US2] Spot-verify plugins served by the shared fixes — renamer, undelete, filecomp, dbviewer, nethood (E2/E3/E7 via T028/T029): drive one Unicode-path dialog each; fix any residual direct ANSI site found
- [ ] T037 [US2] UNCERTAIN plugin sweep (E9): uniso, checksum, folders, uncab, unarj, unrar, tar, pak, unlha, unchm, unfat, unole, unmime — grep each for narrow `SetDlgItemText`/`GetDlgItemText`/`WM_SETTEXT`/`CB_`/`LB_` on name variables; fix hits with T028 helpers; record verdict per plugin in research.md inventory
- [ ] T038 [US2] Full-solution checkpoint: `build.cmd full` (all 90 projects, plugins rebuilt against fixed winliblt/lukas); verify quickstart.md rows #7–#17 and #19; record results

**Checkpoint**: All display surfaces render the sample-name set correctly

---

## Phase 5: User Story 3 - Names entered by the user are applied with full fidelity (Priority: P3)

**Goal**: Every editable name field round-trips typed/pasted Unicode (including outside-ACP characters) byte-exactly to the file system.

**Independent Test**: quickstart.md rows #4, #5, #8, #18 + clipboard paste; code-point verification of results.

### Implementation for User Story 3

- [ ] T039 [US3] Switch browse helpers feeding name fields to W common dialogs (B16, decision D7): `GetOpenFileNameA`/`GetSaveFileNameA` → `…W` structs/calls with `SalWToU8`/`SalU8ToW` at the boundary in src/dialogs.cpp:1843 and src/execute.cpp:1705/1712/2130/2157
- [ ] T040 [US3] Input-fidelity verification pass: F2 rename to multi-script name `Тест-测试-🙂` (quickstart #5), F7 create `テスト-dir` (#8 — exercises `CCopyMoveDialog` via src/fileswn5.cpp:1963), clipboard-paste a Unicode path into Change Directory and the command line (#9/#18), browse-dialog pick of a Unicode-named file (T039); verify each result byte-exactly via code-point dump; record results

**Checkpoint**: All stories independently verified

---

## Phase 6: Polish & Cross-Cutting Concerns

- [ ] T041 Audit closure (FR-005/SC-002): update research.md inventory — every A/B/C/D/E row gets Resolution (task/commit ref) + ✓/verdict; no DEFECTIVE or UNCERTAIN row left unresolved
- [ ] T042 [P] ASCII regression pass (SC-004): quickstart row #20 across rename/copy/move/create/delete + confirm reference surfaces of research.md §F are untouched (git diff scope check)
- [ ] T043 Format touched files with clang-format (repo config), then Release build check: `build.cmd full release` compiles clean (LTO/WPO)
- [ ] T044 Write specs/005-fix-unicode-display/validation-results.md summarizing the quickstart matrix results, audit closure, and any follow-ups (004 pattern)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: none — start immediately
- **Foundational (Phase 2)**: after Setup; **blocks all stories** (T002 helpers are used by nearly every fix; T003/T004 are shared display plumbing)
- **US1 (Phase 3)**: after Phase 2 — MVP, no dependency on other stories
- **US2 (Phase 4)**: after Phase 2; independent of US1 (different sites), but run after US1 to keep the repro fixture flow; T028 blocks T029/T031/T034/T036/T037 (shared helpers first); T038 last in phase
- **US3 (Phase 5)**: after Phase 2; T040 verification is meaningful only after US1+US2 read-back fixes are in
- **Polish (Phase 6)**: after all stories

### Parallel Opportunities

- Phase 2: T003, T004, T005 in parallel (different files) after/alongside T002
- Phase 4 core: T010–T027 all parallel (strictly per-file tasks)
- Phase 4 plugins: T029–T036 parallel after T028
- Phase 6: T042 parallel with T041

## Implementation Strategy

**MVP first**: Phases 1–3 fix and verify the reported bug (SC-001) in three code tasks — deliverable on its own. Then US2 closes the audit tail in two waves (core → plugins) with a full-solution checkpoint, and US3 finishes input fidelity. Stop-and-validate points: T006, T009 (MVP), T038, T040, T044. Commit after each task or logical per-file group.
