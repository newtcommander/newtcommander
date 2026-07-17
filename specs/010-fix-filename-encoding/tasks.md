# Tasks: Complete Revision of File Name and Path Display Encoding

**Input**: Design documents from `/specs/010-fix-filename-encoding/`
**Prerequisites**: plan.md, spec.md, research.md (R1–R9), data-model.md, contracts/display-conversion-contract.md, quickstart.md

**Tests**: No automated test tasks — per the 2026-07-17 clarification, verification is the documented manual-walkthrough protocol (quickstart.md), recorded in `surface-inventory.md`. Build verification via `build.cmd` after each phase.

**Organization**: Tasks are grouped by user story. Every code change must satisfy the rules in `contracts/display-conversion-contract.md` (C1–C7): convert UTF-8 → UTF-16 with the `Sal*U8`/`SalU8ToW*` helpers, keep the invalid-UTF-8 ANSI fallback, measure/index in WCHAR units, convert on change not on paint, no visual restyling.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 (chrome P1), US2 (packer lists P2), US3 (audit P3)

## Phase 1: Setup

**Purpose**: Baseline, test data, and the audit inventory skeleton

- [X] T001 Create `specs/010-fix-filename-encoding/surface-inventory.md` seeded from research.md R7: one row per candidate surface (id, area, location file:line, string source, api, verdict=candidate, resolution=—), grouped by area (chrome, dialogs, menus, toolbars, tooltips, misc-deferred, plugin-ftp, plugin-sftp, plugin-regedt, plugins-spot-check), plus the "already safe" reference list
- [X] T002 [P] Create the sample-name test tree per quickstart.md (`Můj disk\AI`, composed+decomposed `č-dir` pair, `Тест-Ελλάδα-测试`, `emoji-🙂-dir`, `plain-ascii`) under the local temp test root
- [X] T003 [P] Verify clean baseline build: `build.cmd` (Debug x64) passes on branch `010-fix-filename-encoding` before any change

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Shared helper needed by US2 and US3 listbox sites

**⚠️ CRITICAL**: T004 blocks T014 (config pages listbox); other stories do not depend on it

- [X] T004 Add `SalListBoxAddStringU8(HWND listbox, const char* u8Text)` to `src/common/winlib.h` and `src/common/winlib.cpp`, exactly mirroring `SalComboAddStringU8` (`winlib.cpp:1146-1158`): `SalU8ToWAlloc` → `SendMessageW(LB_ADDSTRING)` → `free`, ANSI `LB_ADDSTRING` fallback on invalid UTF-8; same `INSIDE_SALAMANDER && !_UNICODE` guard

**Checkpoint**: Helpers complete — user stories can proceed (US1 immediately, in parallel with this phase if desired)

---

## Phase 3: User Story 1 - Main-window chrome renders Unicode paths correctly (Priority: P1) 🎯 MVP

**Goal**: Directory Line, bottom Info Line, and their tooltips render any Unicode path exactly (correct glyphs, correct ellipsis, no dropped components); hot-track click/drag/copy operate on the true path. Fixes the reported `G:\Můj disk\AI` → `G:\MĹŻj disk` defect (005 deferred item A2 + A6 tooltip).

**Independent Test**: quickstart.md "P1 walkthrough" — navigate into the diacritics test path, compare Directory Line/Info Line/tooltip with the panel rendering, shrink the window to force ellipsis, click/drag path segments (SC-001).

### Implementation for User Story 1

- [X] T005 [US1] Add a cached wide-text model to `CStatusWindow` in `src/stswnd.h` + `src/stswnd.cpp`: new `WCHAR* TextW` (+ WCHAR length) filled once in `SetText` (`stswnd.cpp:119-165`) and `SetSubTexts` via `SalU8ToWAlloc`; NULL ⇒ invalid UTF-8 ⇒ legacy ANSI route stays fully intact (contract C2, C4); keep UTF-8 `Text` as source of truth for producers/consumers (data-model.md §4)
- [X] T006 [US1] Rebase `CStatusWindow::BuildHotTrackItems` (`src/stswnd.cpp:167-302`) on the wide cache: `GetTextExtentExPointW`, `AlpDX` per WCHAR, `HotTrackItems[].Offset/Chars` and root/`'\\'` segment splitting in WCHAR units (contract C3); `blBottom` measurement branch (`:265-298`) and `SubTexts` offsets included
- [X] T007 [US1] Rewrite the text drawing in `CStatusWindow::Paint` (`src/stswnd.cpp:739-1188`) to `ExtTextOutW` on `TextW` — all sites (`:1004,1008,1015,1017,1019-1020,1029,1033,1045,1053,1082,1086-1087,1095,1099-1104`) — with the ellipsis/truncation math (`:881-954`) in WCHAR units so trailing components are never dropped; surrogate pairs never split (mirror `CTruncatedString`, `src/salamdr4.cpp:157+`); info-line `DrawText` at `:2225` → `DrawTextW`; ANSI fallback branch preserved when `TextW == NULL`
- [X] T008 [US1] Convert the hot-track consumers to round-trip the true path (contract C5): `FindHotTrackItem` (`src/stswnd.cpp:619-638`), `GetHotText` (`:605-617`, `SalWToU8` back to UTF-8), and the click/drag handlers copying `Text + Offset` (`:1802, 2082, 2098`) — navigation, clipboard, and drag-drop must carry exact UTF-8 paths
- [X] T009 [US1] Verify the Info Line producers still deliver correct sub-text highlighting: check `CFilesWindow::ItemFocused`/`ExpandInfoLineItems` call sites (`src/fileswn2.cpp:1139-1174`, `src/fileswn3.cpp:1397`, `src/fileswnb.cpp:970,987`) pass byte-based `SubTexts` offsets and adapt the `SetSubTexts` boundary to convert them to WCHAR offsets inside `CStatusWindow` (producers stay UTF-8/byte-based — no API change outside the class)
- [X] T010 [P] [US1] Convert `CToolTip` to wide rendering in `src/tooltip.cpp`: measurement `DrawText(DT_CALCRECT)` at `:323` and paint at `:640` → `SalU8ToWAlloc` + `DrawTextW` with ANSI fallback — covers the Directory Line tooltip (`WM_USER_TTGETTEXT` case 4, `src/stswnd.cpp:1686-1700`), panel long-name tooltip, and throbber/security tooltips (`stswnd.cpp:1720-1730`)
- [ ] T011 [US1] Run the quickstart P1 walkthrough with the sample-name tree (exact path, ellipsis at narrow widths, tooltip, segment click/drag, Info Line with emoji names, ASCII regression) and record verdicts + resolutions for the A2/A6 rows in `specs/010-fix-filename-encoding/surface-inventory.md` (SC-001)

**Checkpoint**: The two permanently visible chrome lines are correct for the full sample set — MVP deliverable

---

## Phase 4: User Story 2 - Packer lists and other stored display strings render correctly (Priority: P2)

**Goal**: Alt+F5/Alt+F6 packer combos, the Pack/Unpack/External-Archivers configuration pages, and the save→restart→reload round trip all show correct text; configs from pre-UTF-8 versions rebuild packer sections from defaults (clarified reset policy).

**Independent Test**: quickstart.md "P2 walkthrough" — fresh config with Czech `.slg`: every Alt+F5/Alt+F6 combo entry readable; custom packer `Můj balíčkovač` round-trips across restart; legacy-version config triggers the defaults rebuild (SC-002).

### Implementation for User Story 2

- [X] T012 [P] [US2] Fix `CPackDialog::Transfer` in `src/dialogs3.cpp`: packer-title combo fill at `:1845` and the `IDE_PATH` combo entries at `:1875,1878` → `SalComboAddStringU8` (contract C1; the same DialogProc already converts the subject label at `:1965-1972` — use it as the in-file reference)
- [X] T013 [US2] Fix `CUnpackDialog::Transfer` in `src/dialogs3.cpp`: packer-title combo fill at `:2091` plus its path-combo entries → `SalComboAddStringU8` (same file as T012 — do after T012)
- [X] T014 [US2] Fix the packer-name surfaces in `src/dialogsp.cpp` (005 item B11): custom packers/unpackers pages at `:238,377,666,781` (the `EDTLBN_GETDISPINFO` edit-list-box route must deliver wide text or convert at fill — follow the pattern chosen for the edit-list-box control), External Archivers listbox at `:891` → `SalListBoxAddStringU8` (from T004), and title read at `:1116,1117-1150` viewer/editor command combos per contract C1
- [X] T015 [P] [US2] Implement the legacy-config reset gate in `src/mainwnd2.cpp` (research R4, data-model §5): first verify (git history of `THIS_CONFIG_VERSION`, `mainwnd2.cpp:143`) which version value corresponds to the feature-004 UTF-8 baseline; bump `THIS_CONFIG_VERSION` 104 → 105; in the packer-section load orchestration (`:2880-2977` — Custom Packers, Custom Unpackers, Predefined Archivers, Archive Associations) skip `Load()` and rebuild via `DeleteAllPackers()` + `AddDefault()` when `Configuration.ConfigVersion` < the verified UTF-8 baseline (precedent: plugin gate `ConfigVersion >= 6` at `:2876`); healthy post-004 configs MUST NOT be reset
- [ ] T016 [US2] Run the quickstart P2 walkthrough (fresh Czech-`.slg` config, Alt+F5/Alt+F6, config pages, `Můj balíčkovač` save→restart→reload, legacy-version reset case) and record verdicts + resolutions for the pack-family and B11 rows in `specs/010-fix-filename-encoding/surface-inventory.md` (SC-002, FR-004)

**Checkpoint**: All packer/archiver display strings correct end to end, including upgrade behavior

---

## Phase 5: User Story 3 - Exhaustive audit closes every remaining surface, including plugins (Priority: P3)

**Goal**: Every remaining display surface in the core app and the 18 enabled plugins is verified with the sample-name set; every defective site fixed; the audit inventory is complete with a verdict per row ("prověř vše").

**Independent Test**: quickstart.md "P3 walkthrough" — walk `surface-inventory.md` area by area; 100 % of rows have a verdict, all defective rows fixed and re-verified (SC-003, SC-004).

### Implementation for User Story 3

- [X] T017 [P] [US3] Convert owner-draw menu text (005 item A4) in `src/menu3.cpp` (~9 draw sites), `src/menu1.cpp`, `src/menu2.cpp`, `src/menubar.cpp`: dir-history, drive menus (volume labels), hot paths, user menu — wide draw + wide measurement per contracts C1–C4
- [X] T018 [P] [US3] Convert toolbar text (005 item A5) in `src/toolbar2.cpp:299,633,639`, `src/toolbar3.cpp:498`, `src/toolbar4.cpp:1421` (hot-path bar labels, drive labels) — wide draw + measurement; note drive-label *acquisition* stays ANSI (research R6 follow-up, not this feature)
- [X] T019 [P] [US3] Replace the byte-based `PathCompactPath` in the command-line combo custom draw at `src/editwnd.cpp:1567` with the `PathCompactPathW` pattern from `src/finddlg1.cpp:4050-4058` (wide compact + ANSI fallback)
- [X] T020 [US3] Resolve the misc deferred/B17 sites with a per-site verdict (fix or justify `correct`/`out-of-scope`): `src/salamdr2.cpp:223,225`; `src/salamdr3.cpp:3150,3635,3802`; `src/codetbl.cpp:668`; `src/shellsup.cpp:2028,2091` (New-file templates); `src/dialogs6.cpp:2527`; `src/dialogs3.cpp:2866`; `src/dialogs5.cpp:1422,2917,2966` (archive-name statics); `src/dialogs4.cpp:1508,3060` (in-place ListView label-edit read-backs — `LVN_ENDLABELEDITW` route); `src/packac.cpp:186` (custom-draw — `NMLVDISPINFOW` route)
- [X] T021 [P] [US3] Fix the ftp plugin's own name/path UI in `src/plugins/ftp/`: bookmark listbox `LB_INSERTSTRING` at `dialogs1.cpp:1231` (005 item E1) and remote-path text in the log/operation windows — use `SplU8ToW*` helpers from `src/plugins/shared/splunicode.h` with ANSI fallback
- [X] T022 [P] [US3] Verify and fix the sftp plugin's own surfaces in `src/plugins/sftp/` (log window `logs.cpp`, connect dialogs) with the same `SplU8ToW*` pattern
- [X] T023 [P] [US3] Fix the regedt plugin's on-disk file-path edit fields (005 item E5) in `src/plugins/regedt/dialogs.cpp` with the shared plugin helpers
- [X] T024 [US3] Spot-check the remaining enabled plugins (zip, 7zip, tar, uncab, uniso, checksum, pictview, renamer, undelete, folders, portables, peviewer, dbviewer, diskmap, filecomp) against the sample-name set — their listings render via the already-fixed core panel; record one `verified-correct` (or defect + fix) row per plugin in `specs/010-fix-filename-encoding/surface-inventory.md`
- [ ] T025 [US3] Run the full quickstart P3 walkthrough over the complete inventory (menus, toolbars, tooltips, Find window, archive browsing with Unicode entry names, plugin surfaces); fix anything newly discovered under the same contract; ensure every row has verdict + resolution (SC-003, SC-004)

**Checkpoint**: Audit inventory complete — no surface without a verdict, no defective surface unfixed

---

## Phase 6: Polish & Cross-Cutting Concerns

- [ ] T026 ASCII regression pass (SC-005, FR-008): quickstart "Regression pass" in `plain-ascii` (browse, Alt+F5, copy/move, rename) + confirm reference surfaces untouched (`src/fileswn4.cpp` panel drawing, `src/mainwnd1.cpp:1899-1904` title, 005-fixed dialogs)
- [X] T027 Format and build verification: clang-format on all touched files (repo `normalize.ps1`/config), then clean `build.cmd rebuild` (Debug x64) and `build.cmd full` smoke run of the built `salamand.exe`
- [X] T028 Finalize `specs/010-fix-filename-encoding/surface-inventory.md` summary header (totals per verdict, walkthrough dates, sample set, build verified against) and note the drive-bar volume-label ANSI-acquisition limitation as a documented follow-up (research R6/R9)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: none — start immediately
- **Foundational (Phase 2)**: after Setup; T004 blocks only T014
- **US1 (Phase 3)**: after Setup — independent of T004; T005 → T006 → T007 → T008/T009 sequential (same files); T010 parallel to all of them
- **US2 (Phase 4)**: after Setup; T014 needs T004; T012 → T013 sequential (same file); T015 independent
- **US3 (Phase 5)**: after Setup; T017–T023 mutually parallel; T020 after US2 (touches `dialogs3.cpp:2866`); T024–T025 last
- **Polish (Phase 6)**: after all desired stories

### User Story Dependencies

- **US1 (P1)**: no dependency on other stories — MVP
- **US2 (P2)**: independent of US1 (different files)
- **US3 (P3)**: independent of US1/US2 except T020's one `dialogs3.cpp` site; the final walkthrough T025 assumes US1/US2 fixes are in to report a complete inventory

### Parallel Opportunities

```text
Phase 1:  T002 ∥ T003 (after T001)
Phase 3:  {T005→T006→T007→T008→T009} ∥ T010
Phase 4:  {T012→T013} ∥ T014 (after T004) ∥ T015
Phase 5:  T017 ∥ T018 ∥ T019 ∥ T021 ∥ T022 ∥ T023, then T020 → T024 → T025
Stories:  US1 ∥ US2 possible (disjoint files) if staffed
```

---

## Implementation Strategy

**MVP first (US1 only)**: T001–T003 → T005–T011. Delivers the reported, permanently visible Directory Line defect fixed and independently verified — a shippable increment.

**Incremental delivery**: add US2 (packer lists + upgrade reset) → verify → add US3 (audit closure) → verify → polish. Each checkpoint leaves the app strictly better with zero regressions (ANSI fallback preserves all pure-ASCII behavior bit-for-bit — contract C7).

**Traceability**: FR-001/002 → T005–T011; FR-003 → T012–T014; FR-004 → T014–T016; FR-005 → T001, T011, T016, T020, T024, T025, T028; FR-006 → contract C1/C2 in every code task; FR-007 → T008; FR-008 → T026. SC-001 → T011; SC-002 → T016; SC-003/004 → T025/T028; SC-005 → T026.
