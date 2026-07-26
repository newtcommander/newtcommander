# Tasks: Fix About Dialog Copyright Notice

**Input**: Design documents from `/specs/040-fix-about-copyright/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/copyright-display.md, quickstart.md

**Tests**: No automated test tasks. The repository has no unit-test harness for
the main application, and the specification did not request one. Verification is
performed by scripted assertions over files and by visual checks of the two
affected surfaces — both are listed as explicit tasks with pass criteria.

**Organization**: Tasks are grouped by user story. User Story 1 (correct
attribution everywhere) is a complete, shippable fix on its own — it works even
before any translation archive is touched, because the runtime override wins
regardless of what the language module contains. User Story 2 makes the fix
structural so it cannot regress.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2)

## Path Conventions

Repository root is `E:\Projects\newtcommander`. Product sources live under
`src/`, translation archives under `translations/`. Scratch output goes to the
session scratchpad, never into the repository.

---

## Phase 1: Setup (Baseline Capture)

**Purpose**: Record the "before" state so the invariants (FR-012, INV-5) can be
proven rather than assumed.

- [X] T001 Record the current `LegalCopyright` literal from `src/versinfo.rh2` and the byte size, line count and SHA-256 of every `translations/*/salamand.slt` into a baseline file in the scratchpad directory

**Checkpoint**: Baseline captured — every later invariant check has something to compare against.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The two display constants both user stories read. Nothing else can
be implemented until these exist.

**⚠️ CRITICAL**: T002 blocks all of Phase 3.

- [X] T002 Restructure the copyright defines in `src/versinfo.rh2`: replace `VERSINFO_COPYRIGHT1` / `VERSINFO_COPYRIGHT2` with `VERSINFO_COPYRIGHT_OPENSAL` = `"Copyright © 1997-2026 Open Salamander Authors"` and `VERSINFO_COPYRIGHT_NEWT` = `"Copyright © 2026 Newt Commander Authors"`, both self-contained; leave `VERSINFO_COPYRIGHT` byte-identical; update the maintenance comment to name all three defines (FR-006, FR-012)
- [X] T003 Verify in `src/versinfo.rh2` that `VERSINFO_COPYRIGHT` is unchanged from the T001 baseline, and grep the whole tree to confirm no reference to `VERSINFO_COPYRIGHT1` or `VERSINFO_COPYRIGHT2` survives outside `src/logo.cpp` (which Phase 3 updates)

**Checkpoint**: Both display constants exist and the metadata string is provably untouched.

---

## Phase 3: User Story 1 - Correct attribution in every language (Priority: P1) 🎯 MVP

**Goal**: The About dialog and the splash screen show the correct English notice
in the correct order, in every language, regardless of what the language module
says.

**Independent Test**: Build, launch under Czech (the reported case), open
**Help → About Newt Commander**. The first line reads `Copyright © 2026 Newt
Commander Authors`, the second `Copyright © 1997-2026 Open Salamander Authors`.
This holds with the translation archives still carrying their corrupted text —
which is exactly what proves the runtime override works.

### Implementation for User Story 1

- [X] T004 [US1] In `CAboutDialog::DialogProc`'s `WM_INITDIALOG` in `src/logo.cpp`, set `IDC_STATIC_1` to `VERSINFO_COPYRIGHT_NEWT` and `IDC_STATIC_2` to `VERSINFO_COPYRIGHT_OPENSAL` via `SetDlgItemText`, placed beside the existing `IDC_ABOUT_WWW` override, with a comment naming the feature and why the text bypasses the language module (FR-002, FR-003, FR-004, FR-005)
- [X] T005 [US1] In `CSplashScreen::PrepareBitmap` in `src/logo.cpp`, swap the two `PaintText` calls so `CopyrightR` receives `VERSINFO_COPYRIGHT_NEWT` and `Copyright2R` receives `VERSINFO_COPYRIGHT_OPENSAL`, and update the surrounding comment to state the order rule (FR-011)
- [X] T006 [US1] Verify `src/logo.cpp` still compiles clean and matches `clang-format` (run the repository's formatting check on the changed file only)

### Verification for User Story 1

- [X] T007 [US1] Run `build.cmd full` from the repository root and confirm it completes with no errors and produces `newtcommander.exe` plus the 8 enabled `.slg` modules under `lang\`
- [X] T008 [US1] Launch the built `newtcommander.exe` in English, open **Help → About Newt Commander**, and confirm the two lines and their order match contract C-1 and C-2 exactly (FR-001, FR-002, FR-003)
- [X] T009 [US1] Repeat T008 under Czech — the originally reported failure — and confirm the displayed bytes are identical to English: no `Autorská práva`, no missing `Open Salamander`, no `2023` (FR-004, SC-002, SC-003)
- [X] T010 [US1] Repeat T008 under the remaining enabled languages (German, French, Dutch, Hungarian, Romanian, Slovak, Spanish) and confirm all are byte-identical to English (FR-004, SC-001)
- [X] T011 [US1] Confirm neither line is clipped, wrapped or overlapped in any language checked, in both the light and the dark theme (FR-007, SC-005)
- [X] T012 [US1] Restart with the splash screen enabled and confirm it lists the two holders in the same order as the About dialog (FR-011, SC-007)

**Checkpoint**: The reported bug is fixed and verified in every shipped language. This is a complete, shippable increment.

---

## Phase 4: User Story 2 - Translations can no longer break the notice (Priority: P2)

**Goal**: Remove the notice from the translation pipeline entirely, so no future
translation round — human or machine — can produce a divergent copyright line.

**Independent Test**: Put arbitrary text into the Czech archive's two copyright
rows, rebuild Czech, run under Czech: the About dialog is unaffected. Then
confirm that a merge round has nothing to translate for those rows because the
English source is empty.

### Implementation for User Story 2

- [X] T013 [US2] In `src/lang/lang.rc`, blank the captions of the two `LTEXT` controls in `IDD_ABOUT` (`IDC_STATIC_1` at `10,97,196,8` and `IDC_STATIC_2` at `10,108,196,8`), keeping both controls, their IDs and their geometry, and add a comment stating the text is supplied at runtime from `versinfo.rh2` (FR-008, FR-009)
- [X] T014 [US2] Blank the text field of rows `1150,10,97,196,8,1,...` and `1151,10,108,196,8,1,...` in all 11 `translations/*/salamand.slt` archives with a byte-level script, keeping the state flag at `1`, the row count identical, and the UTF-8 BOM and CRLF endings intact (FR-009, FR-009a, FR-010, INV-5)

### Verification for User Story 2

- [X] T015 [US2] Assert every edited archive still starts with a UTF-8 BOM, uses CRLF exclusively, and has the exact line count recorded in the T001 baseline (INV-5)
- [X] T016 [US2] Assert that grepping all 11 archives for a non-empty About-dialog copyright row returns zero hits, down from 22 (FR-009a, SC-004a)
- [X] T017 [US2] Re-run `build.cmd full` and confirm all 8 enabled language modules still build, then re-confirm the About dialog text under English and Czech is unchanged from Phase 3 (regression guard for the positional import)
- [X] T018 [US2] Negative test: inject arbitrary text into the two copyright rows of `translations/czech/salamand.slt`, rebuild, confirm the About dialog under Czech still shows the correct English notice, then restore the blanked rows (SC-004)

**Checkpoint**: The notice is structurally untranslatable. Both user stories are complete.

---

## Phase 5: Polish & Cross-Cutting Concerns

- [X] T019 [P] Verify the built `newtcommander.exe` reports `LegalCopyright` exactly as recorded in the T001 baseline, using the file's version resource (FR-012, INV-1)
- [X] T020 [P] Verify `IDD_ABOUT` in `src/lang/lang.rc` still declares twelve controls with unchanged IDs, geometry and dialog size `299x184`, and retains `DIALOGEX` / `DS_SETFONT | DS_FIXEDSYS` / `FONT 8, "MS Shell Dlg"` (FR-008, INV-6, Constitution VI)
- [X] T021 Add a one-line note to the **Copyright rule** bullet in `CLAUDE.md` recording that the two displayed notices live in `src/versinfo.rh2` and are never translated, so the next maintainer does not go looking in the language files
- [X] T022 Walk `specs/040-fix-about-copyright/quickstart.md` end to end and confirm every documented check passes as written

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies — run first, it only reads
- **Foundational (Phase 2)**: depends on T001 for the `LegalCopyright` baseline; **blocks Phase 3**
- **User Story 1 (Phase 3)**: depends on T002 (needs the constants). Independent of Phase 4.
- **User Story 2 (Phase 4)**: depends on T002 for the constants and on Phase 3 having established the verified-good display, which T017/T018 regression-check against. T013 and T014 themselves touch files no other task touches.
- **Polish (Phase 5)**: depends on a completed build from T017

### User Story Dependencies

- **User Story 1 (P1)**: starts after T002. No dependency on User Story 2 — the runtime override makes the display correct while the archives are still corrupted, which is precisely the property T009 verifies.
- **User Story 2 (P2)**: starts after T002. Could technically run before User Story 1, but is sequenced after so that T018's negative test has a known-good display to contrast against.

### Within Each User Story

- Source edits (T004, T005) before the build (T007) before the visual checks (T008–T012)
- Archive edits (T013, T014) before the structural assertions (T015, T016) before the rebuild (T017)

### Parallel Opportunities

- T004 and T005 both edit `src/logo.cpp` — **not** parallel, despite being in different functions
- T013 (`src/lang/lang.rc`) and T014 (`translations/*/salamand.slt`) touch disjoint files and can run in parallel
- T019 and T020 are independent read-only checks and are marked [P]
- T010's per-language checks are independent of one another and can be split across people

---

## Parallel Example: User Story 2

```text
# Disjoint file sets, safe to run together:
Task: "T013 Blank the two IDD_ABOUT captions in src/lang/lang.rc"
Task: "T014 Blank the two copyright rows in all 11 translations/*/salamand.slt"
```

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Phase 1 — capture the baseline
2. Phase 2 — add the two constants (blocking)
3. Phase 3 — wire them into the About dialog and the splash, build, verify
4. **STOP and VALIDATE**: the reported Czech defect is gone and every enabled
   language shows the correct notice
5. Shippable at this point — the bug is fixed

### Incremental Delivery

1. Setup + Foundational → constants exist, metadata provably untouched
2. Add User Story 1 → bug fixed and verified → shippable (MVP)
3. Add User Story 2 → the fix becomes structural; a future translation round
   cannot reintroduce the defect
4. Polish → invariants and documentation

---

## Notes

- [P] tasks touch different files and have no ordering dependency
- Every task names the requirement(s) it satisfies, so traceability runs
  spec → task → verification without a separate matrix
- The three languages disabled by the language build policy (Chinese
  Simplified, Russian, Ukrainian) get their archives corrected in T014 but
  produce no module to inspect — FR-010 is satisfied by the archive edit, not by
  a visual check
- Commit after each phase; each is independently revertable
