---
description: "Task list for 043-fix-ui-text-encoding"
---

# Tasks: Fix UI Text Encoding in Language Selection and Rename Captions

**Input**: Design documents from `/specs/043-fix-ui-text-encoding/`

**Tests**: Included — FR-016 requires an explicit automated test per repaired surface.

## Format: `[ID] [P?] [Story] Description`

---

## Phase 1: Setup

- [X] T001 Confirm the fixture set and all 9 language modules are present; `build.cmd full` to regenerate `lang\*.slg`
- [X] T002 Produce a clean baseline Debug x64 build and record it succeeds

---

## Phase 2: Foundational (Blocking)

- [X] T003 Revert the feature 042 regression in `src/dialogs5.cpp`: the plugin-remove message must use `LoadStr` again, annotated so it is not re-converted (its substituted value is a local copy of the ANSI `p->Name`)
- [X] T004 Sweep every site feature 042 converted for the same misclassification — a substituted variable that is a local copy of plugin metadata — and record the result
- [X] T005 Widen `tools/check_encoding.py` (FR-011) with rules that describe the defect rather than two examples: a UTF-8 value reaching a legacy list-view call, a legacy window-text call, a legacy status-bar call, or a composed caption whose template is `LoadStr`
- [X] T006 Prove the widened guard detects all three reported defects on the **pre-fix** tree (SC-007) — stash the fixes, run, record, restore
- [X] T007 Add `SalStatusSetTextU8()` to `src/common/winlib.h` / `winlib.cpp` alongside the existing `Sal*U8` family

**Checkpoint**: guard describes the class and is proven to detect it.

---

## Phase 3: US1 - Language names are readable (P1)

- [X] T008 [US1] `src/dialogs2.cpp` — language list: use `SalListViewSetItemTextU8` for the language name
- [X] T009 [US1] Verify the language list in all 9 shipped languages; every name exact, no mojibake
- [X] T010 [P] [US1] Verify the dialog's other controls (labels, note field, path column) are unchanged

---

## Phase 4: US2 - The configured language reads correctly (P1)

- [X] T011 [US2] `src/dialogs4.cpp` — configuration language field: use `SalSetDlgItemTextU8`
- [X] T012 [US2] Verify the field in all 9 shipped languages
- [X] T013 [P] [US2] Verify the rest of the configuration page is unchanged

---

## Phase 5: US3 - Composed captions (F2 / F5 / F6 and family) (P1)

- [X] T014 [US3] `src/fileswn5.cpp:2383` — F2 Quick Rename caption: `LoadStrU8` for template and inner strings
- [X] T015 [US3] `src/fileswn8.cpp:474` — F5 Copy / F6 Move / F8 Delete caption: `LoadStrU8` + `ExpandPluralFilesDirs(..., TRUE)`
- [X] T016 [US3] `src/fileswna.cpp:92` — F5/F6/Delete on a plugin file system: same, keeping `RemoveAmpersands` byte-safe
- [X] T017 [P] [US3] `src/fileswn7.cpp:469` — copy out of / delete from an archive
- [X] T018 [P] [US3] `src/fileswn7.cpp:1382` — Pack files (Alt+F5)
- [X] T019 [P] [US3] `src/fileswn7.cpp:1580` — add-to-existing-archive confirmation
- [X] T020 [P] [US3] `src/fileswn7.cpp:1724` — Unpack archive (Alt+F6)
- [X] T021 [P] [US3] `src/fileswn5.cpp:406` — NTFS compress/encrypt confirmation
- [X] T022 [P] [US3] `src/finddlg2.cpp:1949` — Find log → Ignore
- [X] T023 [US3] `src/plugins3.cpp:547,550` — plugin subject label: give it a wide path (`SalSetWindowTextU8`); it has none today and is broken even in English
- [X] T024 [US3] Verify the F2 caption against `Тест-Ελλάδα-测试 +ěš` in all 9 languages; caption name identical to the edit field
- [X] T025 [US3] Verify the F5 and F6 captions in all 9 languages
- [X] T026 [P] [US3] Verify caption truncation on a very long non-ASCII name does not split a character

---

## Phase 6: US4 - Sweep the class (P2)

- [X] T027 [US4] `src/dialogs.cpp:1682,1683` — overwrite-confirmation source/target attributes (size, date, time)
- [X] T028 [P] [US4] `src/finddlg1.cpp:2072,2327,3242` — Find window caption
- [X] T029 [P] [US4] `src/finddlg1.cpp:3001` and `src/finddlg2.cpp:285` — Find status bar, via the new `SalStatusSetTextU8`
- [X] T030 [P] [US4] `src/finddlg2.cpp:720,721,723` — Find options fields
- [X] T031 [P] [US4] `src/dialogs3.cpp:2013,2015,2022,2024` — pack dialog path combo, via `SalComboAddStringU8`
- [X] T032 [P] [US4] `src/dialogs2.cpp` occupied-space fields and `src/dialogs3.cpp` volume-information fields — the `NumberToStr` / `PrintDiskSize` locale-separator cluster
- [X] T033 [P] [US4] `src/dialogs.cpp:2093`, `src/dialogs6.cpp:2054,2388` — date and reparse-point/drive fields
- [X] T034 [US4] `src/fileswn9.cpp:1665,1710,1716` — drag image: add a wide measuring/drawing path with the ANSI call as fallback
- [X] T035 [US4] Write `specs/043-fix-ui-text-encoding/inventory.md` recording every site in the class with its verdict, axis and language class
- [X] T036 [US4] Re-run the widened guard; every remaining finding must be annotated with a reason

---

## Phase 7: US5 - Nothing that worked before is broken (P1)

- [X] T037 [US5] Re-verify feature 042: the reported Find search, exact names, Path column, sorting, type-to-search
- [X] T038 [US5] Re-verify feature 042: the duplicate-name notice in all 9 languages
- [X] T039 [US5] Re-verify feature 041: the panel information line for the reported `.mkv`, and the selection summary
- [X] T040 [P] [US5] Re-verify the panel file lists, long paths and the Find Path column against the fixture set
- [X] T041 [US5] Walk every dialog touched by this feature in all 9 languages and confirm no control that was correct before is wrong after

---

## Phase 8: Polish

- [X] T042 Add per-surface automated tests to `src/saltests/saltests.cpp` (FR-016): composed-caption encoding, locale-name encoding, number-with-separator encoding, truncation safety
- [X] T043 Demonstrate the widened guard failing for each of the three reported defects, separately (FR-012, SC-006)
- [X] T044 [P] Plugin review: confirm `src/plugins/`, `plugins.h`, `spl_gen.h`, `spl_vers.h` all have empty diffs
- [X] T045 [P] Confirm `LoadStr`, `SalMessageBox` and `CMessageBox` are unmodified (FR-014)
- [X] T046 Write `specs/043-fix-ui-text-encoding/validation-results.md` including everything NOT verified and why
- [X] T047 Run `clang-format` over every changed file
- [X] T048 Clean Debug and Release builds; `saltests` zero failures

---

## Dependencies

- Phase 2 blocks everything: the guard must be widened and proven *before* the fixes, or it cannot be shown to detect them.
- US1, US2 and US3 are mutually independent (different files, different routes).
- US5 depends on US1–US4 being complete.
- T035 (inventory) depends on T027–T034.

## Task Summary

| Phase | Tasks | Count |
|---|---|---|
| 1 Setup | T001–T002 | 2 |
| 2 Foundational | T003–T007 | 5 |
| 3 US1 (P1) | T008–T010 | 3 |
| 4 US2 (P1) | T011–T013 | 3 |
| 5 US3 (P1) | T014–T026 | 13 |
| 6 US4 (P2) | T027–T036 | 10 |
| 7 US5 (P1) | T037–T041 | 5 |
| 8 Polish | T042–T048 | 7 |
| **Total** | | **48** |
