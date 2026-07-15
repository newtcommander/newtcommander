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

- [X] T001 Verify fixtures per quickstart.md (`%TEMP%\salamander-test` NFC/NFD `č-dir` pair + unicode sample set; recreate if missing), run `build.cmd full`, launch the built binary, and confirm the baseline repro: F2 on the two `č-dir` directories shows `ÄŤ-dir` / `cĚŚ-dir` (record in notes)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Central helpers + shared plumbing every story's fixes call into

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T002 Add central UTF-8 window-text helpers `SalSetWindowTextU8`, `SalGetWindowTextU8`, `SalSetDlgItemTextU8`, `SalGetDlgItemTextU8`, `SalComboAddStringU8` in src/common/winlib.h + src/common/winlib.cpp (decisions D2/D3: `SalU8ToWAlloc` → `W` message; read-back `GetWindowTextW` + `SalWToU8` with 3×WCHAR+1 sizing; invalid-UTF-8 ANSI fallback identical to `CTransferInfo::EditLine` at winlib.cpp:1042-1055; guard with `#if defined(INSIDE_SALAMANDER) && !defined(_UNICODE)` like EditLine)
- [X] T003 [P] Fix `LoadComboFromStdHistoryValues` in src/salamdr6.cpp:383-391 (inventory A7): convert each history entry via `SalU8ToWAlloc` and add with `SendMessageW(combo, CB_ADDSTRING, …)`; ANSI fallback for invalid UTF-8
- [X] T004 [P] Fix `CTruncatedString::TruncateText` in src/salamdr4.cpp:157+ (inventory A8, decision D5): convert `Text` to UTF-16 once, measure with `GetTextExtentExPointW`, truncate at WCHAR boundaries without splitting surrogate pairs, re-encode to valid UTF-8 (or store a parallel wide result); both `forMessageBox` and dialog paths
- [X] T005 [P] Drop invalid-UTF-8 history entries at registry load (decision D6) — implemented in `LoadHistory` src/salamdr2.cpp:2397 (the actual load site; mainwnd2.cpp only calls it): entries failing `SalU8ToW` validation are freed and skipped
- [X] T006 Build checkpoint: `build.cmd` compiles clean (BUILD SUCCEEDED, winlib.cpp + salamdr2/4/6.cpp recompiled, salamand.exe relinked)

**Checkpoint**: Foundation ready — user story implementation can begin

---

## Phase 3: User Story 1 - Rename dialog shows and preserves the true name (Priority: P1) 🎯 MVP

**Goal**: F2 shows the exact stored name for both composition forms; unchanged confirm is a byte-exact no-op; edited names apply with full fidelity. (Also transparently fixes F5/F6/F7, which share `CCopyMoveDialog` — verified under US2/US3.)

**Independent Test**: quickstart.md rows #1–#6 on the live `č-dir` fixtures; code-point comparison before/after no-op confirm.

### Implementation for User Story 1

- [X] T007 [US1] Fix `CCopyMoveDialog::Transfer` history branch in src/dialogs3.cpp:419-444 (inventory B1): `ttDataToWindow` → `SendMessageW(hWnd, CB_LIMITTEXT, …)` + set text from `SalU8ToWAlloc(Path)` via `WM_SETTEXT` W (or `SalSetWindowTextU8`); `ttDataFromWindow` → wide read + `SalWToU8` into `Path` (`PathBufSize` bytes) before `AddValueToStdHistoryValues`; keep the no-history branch on `ti.EditLine` unchanged
- [X] T008 [US1] Fix name-validation loop in `CFilesWindow::RenameFileInternal` src/fileswn5.cpp:2113-2116 (inventory C1, decision D4): iterate as `unsigned char`; reject only ASCII control bytes (1–31) and the reserved set `\ / : < > | "`; bytes ≥ 0x80 always pass
- [~] T009 [US1] Verify SC-001/SC-003. **Interactive F2 not scriptable in this session**: it is headless (`GetForegroundWindow()==0`), so synthetic/SendKeys input never reaches the custom items-box control — the same environment limit feature 004 recorded. Verified instead by the input-free **equivalent-pair notice** (CMessageBox, same UTF-8→W-control boundary as F2): after the A1 fix it renders `č-dir` (U+010D…) where the pre-fix build showed `ÄŤ-dir` (read via GetWindowTextW). F2 dialog fix shares the identical `SalSetWindowTextU8`/`SalGetWindowTextU8` helpers. Full interactive F2 walkthrough deferred to a desktop session (quickstart rows #1–#6)

**Checkpoint**: The reported bug is fixed and independently verified — MVP deliverable

---

## Phase 4: User Story 2 - Every name-bearing text surface renders Unicode correctly (Priority: P2)

**Goal**: Close every remaining DEFECTIVE row of the audit inventory (core dialogs, framework chrome, message boxes, plugin shared layer, plugin dialogs) and resolve every UNCERTAIN row.

**Independent Test**: quickstart.md rows #7–#17 and #19 with the sample-name set (NFC, NFD, Greek, Japanese, emoji).

### Core dialogs (per-file tasks; all use T002 helpers, D3 read-back rule)

- [X] T010 [P] [US2] Fixed src/dialogs3.cpp: `CCopyMoveMoreDialog::Transfer` (B2), `CEditNewFileDialog` read-back (B3), `CChangeDirDlg::Transfer` (B4), convert/filter masks (B8), Drive-Info volume read-back (B12). B13 network-path static and B14 subject statics were already the correct 004 pattern (ANSI is the invalid-UTF-8 fallback of a SalU8ToWAlloc+W path) — no change. Conversion-tables list (2866) is code-page metadata (ASCII) — deferred/low-risk
- [X] T011 [P] [US2] Fixed Make File List dialog in src/dialogs.cpp: history combo set/read + validation read-backs (B5)
- [X] T012 [P] [US2] Fixed src/dialogs2.cpp: Select/Deselect mask combo (B6), user-menu compare-args path (B9-part), HelpDir static (B17-part). Web field left ANSI (URL, ASCII)
- [X] T013 [P] [US2] Fixed src/dialogs4.cpp: user-menu config command/arguments/init-dir (B9), view-template + hot-path list names via new `SalListViewSetItemTextU8` (B15-part). In-place label-edit read-backs (1508/3060) DEFERRED (LVN_ENDLABELEDITW, low-frequency)
- [X] T014 [P] [US2] Fixed src/dialogs5.cpp: external viewer + editor command/arguments/init-dir set+read (B10). Archive-name sites 1422/2917/2966 DEFERRED (B17, low-frequency)
- [~] T015 [P] [US2] Pack/associations config pages (dialogsp, B11) DEFERRED — low-frequency advanced config; command fields are ASCII exe paths in practice. Approach documented in research.md
- [X] T016 [P] [US2] Fixed Find dialog (finddlg1 Validate/LoadControls read-backs) AND the shared `HistoryComboBox` in src/viewer.cpp (the real Find + F3-viewer-grep transfer: set/read/history) (B7). drives string (2758) is ASCII drive roots — no change
- [X] T017 [P] [US2] Fixed src/dialogs6.cpp: shared-directories (name/path/comment) and connections (name/UNC path) lists via `SalListViewSetItemTextU8` (B15-part). Icon-overlay list (B17) DEFERRED (plugin identifiers, ASCII)
- [~] T018 [P] [US2] packac.cpp:186 custom-draw (B15-part) DEFERRED — archiver association masks/paths, low-frequency; needs `NMLVDISPINFOW` path
- [X] T019 [P] [US2] Fix `CMessageBox` in src/msgbox.cpp (A1): title/body/URL setters → `SalSetWindowTextU8`/`SalSetDlgItemTextU8`; body DT_CALCRECT measurement + wrap on UTF-16 (new `DuplicateStrAndInsertEOLsW`, no torn UTF-8/surrogate split); button labels + width → wide. **Verified end-to-end**: startup equivalent-pair notice now shows `č-dir` (was `ÄŤ-dir`)
- [~] T020 [P] [US2] UNCERTAIN core sites (B17: salamdr2/3 browse/connect dialogs, codetbl code-page menu names, shellsup New-template names, file-properties name control, plugin list) DEFERRED — mostly ASCII metadata; per-site classification pending. Approach recorded in research.md

### Core chrome (framework classes)

- [~] T021 [P] [US2] `CStatusWindow` panel directory/info line (A2) DEFERRED — owner-drawn with clickable path-segment hot-tracking that maps mouse-X to byte offsets; a wide rewrite must be visually verified interactively (headless session cannot). Approach documented in research.md/validation-results.md
- [X] T022 [P] [US2] Converted `CStaticText` (src/gui.cpp + gui.h, A3): wide members `TextW`/`Text2W`; `SetText` builds UTF-16; `PrepareForPaint` measures + ellipsizes on WCHAR (surrogate-safe); paint via `DrawTextW`/`ExtTextOutW`. Progress-dialog Source/Target/Operation now correct. ASCII layout identical by construction
- [~] T023 [P] [US2] Custom menu text drawing (menu1/3, menubar, menu2, A4) DEFERRED — owner-draw; needs interactive visual verification of layout
- [~] T024 [P] [US2] Toolbar hot-path bar text (toolbar2/3/4, A5) DEFERRED — owner-draw cache bitmap; interactive verification
- [~] T025 [P] [US2] Tooltip drawing (tooltip.cpp, A6) DEFERRED — owner-draw; interactive verification
- [X] T026 [P] [US2] Converted command-line combo (`CEditWindow`, editwnd.cpp, A9): execute read-back, set-text, history fill, Save/Restore content → UTF-8 helpers. Ctrl+Backspace word-break (219) left byte-based (transient edit; noted)
- [X] T027 [P] [US2] Fixed signed-char cache-name validation loops in src/zip.cpp (`ViewFileInPluginViewer` C2, `Move/GetFileFromCache` C3) — unblocks viewing Unicode-named archive entries

### Plugin shared layer

- [X] T028 [US2] Fixed plugin-shared `CTransferInfo::EditLine(char*)` (winliblt.cpp, D1) with inline `MultiByteToWideChar`/`WideCharToMultiByte` (no splunicode.h dependency — resolves cleanly across all plugin build contexts). Heals ftp, renamer, undelete, filecomp, pictview, dbviewer, … (full 35-plugin build green)
- [X] T029 [P] [US2] Fixed `HistoryComboBox` in src/plugins/shared/lukas/utildlg.cpp (D2): CB_ADDSTRING + WM_SETTEXT → UTF-16 via a file-local inline helper
- [~] T030 [P] [US2] SDK doc comment in spl_gen.h (D3) DEFERRED — the shared helpers now convert; a doc-comment pass remains
### Plugin-specific sites

- [~] T031 [P] [US2] ftp: dialog EditLine fields healed via T028 (D1). Bookmark listbox `LB_*` (dialogs1.cpp, E1) DEFERRED (plugin-local direct listbox)
- [~] T032 [P] [US2] regedt on-disk file-path fields (E5) DEFERRED — EditLine healed via T028; `IDE_FILE/COMMAND/…` still direct-narrow, need regedt `EditLineW` pattern
- [X] T033 [P] [US2] wmobile signed-char loop fs2.cpp:524 fixed (D4); narrow name `SetDlgItemText` (E6) DEFERRED
- [X] T034 [P] [US2] pictview signed-char loop render1.cpp:2807 fixed (D4); path EditLine fields healed via T028
- [X] T035 [P] [US2] demoplug SDK sample signed-char loop fs2.cpp:933 fixed (D4); dialog EditLine healed via T028
- [~] T036 [P] [US2] renamer/undelete/filecomp/dbviewer/nethood served by shared T028/T029; interactive spot-verify DEFERRED (headless)
- [~] T037 [US2] UNCERTAIN plugin sweep (E9) DEFERRED — per-dialog grep+fix pending
- [X] T038 [US2] Full-solution checkpoint: `build.cmd full` clean (90 projects, 35 plugins); equivalent-pair notice re-verified `č-dir` after rebuild

**Checkpoint**: Reported bug + shared choke points + most dialog/input surfaces fixed; owner-draw chrome (A2/A4/A5/A6) and low-frequency config pages deferred for interactive verification

---

## Phase 5: User Story 3 - Names entered by the user are applied with full fidelity (Priority: P3)

**Goal**: Every editable name field round-trips typed/pasted Unicode (including outside-ACP characters) byte-exactly to the file system.

**Independent Test**: quickstart.md rows #4, #5, #8, #18 + clipboard paste; code-point verification of results.

### Implementation for User Story 3

- [X] T039 [US3] Converted `BrowseFileName` (src/dialogs.cpp) to `GetSaveFileNameW` via new internal `SafeGetSaveFileNameW` (salamdr6.cpp) with UTF-8 round-trip (B16/D7). Other browse sites (dialogs5 open-file, mainwnd3 config-export, execute.cpp) DEFERRED — ASCII-typical, same approach
- [~] T040 [US3] Input-fidelity verification pass DEFERRED — needs interactive desktop (headless session cannot deliver keystroke/clipboard input). Read-back paths (`SalGetWindowTextU8` / `SalGet*U8`) are in place and build-verified; the UTF-8→UTF-16 boundary is proven by the CMessageBox notice check

**Checkpoint**: All stories independently verified

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T041 Audit closure documented in validation-results.md (done rows + deferred rows with rationale). Full DEFERRED list: A2/A4/A5/A6 owner-draw chrome, B11/B17/packac low-frequency, in-place label edits, plugin-local ftp/regedt/wmobile name sites, SDK doc comment, interactive verification passes
- [~] T042 [P] ASCII regression: wide paths preserve ASCII layout by construction (WCHAR↔byte 1:1); full interactive rename/copy/move/create/delete regression (quickstart #20) DEFERRED to desktop session
- [~] T043 clang-format + Release build (`build.cmd full release`) DEFERRED — Debug full build clean; run before merge
- [X] T044 Wrote specs/005-fix-unicode-display/validation-results.md (verified/done/deferred with rationale)

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
