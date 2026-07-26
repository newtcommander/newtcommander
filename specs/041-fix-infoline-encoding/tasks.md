# Tasks: Fix Information Line Encoding

**Input**: Design documents from `/specs/041-fix-infoline-encoding/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: No automated test tasks. The repository has no unit-test harness for
the main application and the specification did not request one. Verification is
by scripted GUI checks (launch, focus an item, read the rendered text, capture
the window) and by file-level assertions — each listed as an explicit task with
a pass criterion.

**Organization**: Tasks are grouped by user story. User Story 1 fixes the
reported defect and is shippable on its own. User Story 2 covers the second
manifestation the investigation found. Phase 5 discharges the two scope
obligations the clarifications added (FR-011 classification, FR-013 plugin
review) — they are cross-cutting, not part of either story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2)

## Path Conventions

Repository root is `E:\Projects\newtcommander`. Product sources under `src/`.
Test fixtures go in `temp/` (already gitignored working area); scratch output to
the session scratchpad, never into the repository.

---

## Phase 1: Setup

**Purpose**: A working build and recorded "before" evidence, so the fix can be
shown to change something rather than asserted to.

- [X] T001 Run `cmd /c "E:\Projects\newtcommander\build.cmd full"` and confirm BUILD SUCCEEDED — the `Debug_x64` tree was removed after feature 040 and must be recreated before anything can be verified
- [X] T002 Capture the defect: point a panel at `temp\`, focus `Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv`, screenshot the window, and record the exact garbled string from the information line into the scratchpad as the "before" baseline
- [X] T003 [P] Create the fixture set in `temp\fixtures-041\`: names covering Czech diacritics, a typographic en dash, a non-Latin script, and the same accented name at exactly 999 bytes and at over 1 000 000 bytes (SC-002, SC-003)

**Checkpoint**: The defect is reproduced and recorded; fixtures exist for the threshold and coverage criteria.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The UTF-8 boundary helpers and the classification that says where
they must be applied. Everything else depends on these.

**⚠️ CRITICAL**: T005 and T006 block both user stories.

- [X] T004 Classify all 42 ANSI locale call sites (`GetLocaleInfo(`, `GetDateFormat(`, `GetTimeFormat(` across the 13 files listed in research.md) into Group A (output reaches a UTF-8 consumer — must convert) or Group C (does not — exempt), tracing each to its consumer and recording file, line and reason in `specs/041-fix-infoline-encoding/validation-results.md` (FR-011)
- [X] T005 Add UTF-8 wrappers over `GetLocaleInfoW`, `GetDateFormatW` and `GetTimeFormatW` in `src/common/salunicode.h` / `src/common/salunicode.cpp` (or a new `src/common/sallocale.*` pair), each converting the W result to UTF-8 and preserving the existing failure fallback where the API returns 0 (contract `locale-text.md` C-2)
- [X] T006 In `src/consts.h` and `src/salamdr1.cpp`, widen `DecimalSeparator` and `ThousandsSeparator` from `char[5]` to `char[16]` and read both through the new wrapper, keeping `DecimalSeparatorLen` / `ThousandsSeparatorLen` as byte counts (contract `locale-text.md` C-4)
- [X] T007 Run a full build and confirm it succeeds with the widened buffers and the new wrappers, with no new warnings in the touched files

**Checkpoint**: Locale-derived text is UTF-8 at the producer; the full conversion list is written down rather than assumed.

---

## Phase 3: User Story 1 - File names with diacritics read correctly (Priority: P1) 🎯 MVP

**Goal**: The information line shows the focused item's name exactly as the file
system stores it, and no field can corrupt another.

**Independent Test**: Focus the reported file. The name in the information line
matches the name in the panel above, character for character, and the size still
reads `1 948 456 197`.

### Implementation for User Story 1

- [X] T008 [US1] Convert the four information-line date/time sites in `src/execute.cpp` (around lines 1066, 1090, 1121, 1145) to the UTF-8 wrappers from T005 (FR-004)
- [X] T009 [US1] Add a display-only lenient UTF-8→UTF-16 conversion in `src/common/salunicode.h` / `.cpp`, substituting `U+FFFD` for malformed sequences, documented so it cannot be mistaken for a general-purpose conversion and never used on stored names or paths (FR-003a, contract `locale-text.md` C-6)
- [X] T010 [US1] In `src/stswnd.cpp` build `CStatusWindow`'s wide mirror (`TextW`, around line 158) with the lenient conversion so it is never `NULL`, and remove or update the comment describing the `NULL` fallback (FR-003, INV-2)
- [X] T011 [US1] In `src/stswnd.cpp`, make truncation surrogate-pair safe so a cut never lands between a high and a low surrogate (FR-007, INV-7)
- [X] T012 [US1] Run `clang-format --dry-run --Werror` on every file changed so far and fix any deviation

### Verification for User Story 1

- [X] T013 [US1] Run a full build and confirm BUILD SUCCEEDED with all 8 enabled language modules
- [X] T014 [US1] Focus the reported file and confirm the information line shows the name exactly — `–`, `á`, `ě`, `ů`, `í` intact, no `Ã`/`Ä`/`Å`/`â€` anywhere — and that it matches the panel above character for character (FR-001, FR-002, SC-001)
- [X] T015 [US1] Using the T003 fixtures, confirm the same accented name displays correctly at 999 bytes and at over 1 000 000 bytes — the thousands-separator threshold no longer changes the outcome (FR-003, SC-003)
- [X] T016 [US1] Confirm every fixture name displays identically in the information line and in the panel (SC-002)
- [X] T017 [US1] Confirm a name containing an unrepresentable character shows `�` for that character alone, with the rest of the name and all other fields still correct (FR-003a)
- [X] T018 [US1] Narrow the window until the line truncates and confirm no character is split, including at a surrogate pair (FR-007)

**Checkpoint**: The reported defect is fixed and verified. Shippable increment.

---

## Phase 4: User Story 2 - The selection summary reads correctly (Priority: P2)

**Goal**: The selection summary — localized words plus a formatted number — is
readable in every shipped language, in the information line and in the Find
dialog.

**Independent Test**: In Czech, select files totalling more than 999 bytes and
read the summary; then do the same in the Find dialog.

### Implementation for User Story 2

- [X] T019 [US2] Confirm `ExpandPluralBytesFilesDirs` in `src/salamdr4.cpp:673` produces valid UTF-8 once T006 is in place; if any part of it still mixes encodings, fix it there rather than at its callers (FR-005)

### Verification for User Story 2

- [X] T020 [US2] Select several files totalling more than 999 bytes and confirm the information line summary is fully readable under each of the 8 enabled languages (FR-005, SC-004)
- [X] T021 [US2] Repeat the same check in the Find dialog, which shares the summary through `src/finddlg2.cpp:284` (FR-005)
- [X] T022 [US2] Confirm the byte count in the summary is still formatted exactly as the regional settings prescribe — only its encoding changed, not its content (FR-004, INV-8)

**Checkpoint**: Both user stories complete.

---

## Phase 5: Cross-Cutting Scope Obligations (FR-011, FR-013)

**Purpose**: Discharge the two obligations the clarifications added. These are
not user stories — they are what makes "fix the cause, not the symptom"
checkable rather than aspirational.

- [X] T023 Convert every remaining Group A site identified in T004 — expected across `src/fileswn2.cpp`, `src/salamdr4.cpp`, `src/worker.cpp`, `src/finddlg1.cpp`, `src/packac.cpp`, `src/fileswn4.cpp`, `src/dialogs5.cpp`, `src/dialogs.cpp` and `src/mainwnd1.cpp` — to the UTF-8 wrappers (FR-010, FR-011)
- [X] T024 Correct the misleading comment at `src/fileswn2.cpp:3764` and its siblings — it reads "locale date may be non-ASCII UTF-8" while the value it describes was ANSI; after T023 the comment becomes true and should say so plainly
- [X] T025 Record every Group C exemption in `specs/041-fix-infoline-encoding/validation-results.md` with the reason it cannot reach a UTF-8 consumer, so 0 sites are left unaccounted for (FR-011, SC-007)
- [X] T026 Review each of the 18 plugins that reference `NumberToStr` or `PointToLocalDecimalSeparator` (7zip, automation, checksum, demoplug, filecomp, ftp, pictview, regedt, renamer, shared, tar, uncab, unchm, uniso, unmime, unrar, undelete, zip), recording per plugin whether it is affected and why, and fixing any that fed the result to an ANSI-only path (FR-013, SC-008)
- [X] T027 Confirm `git diff src/plugins/shared/spl_gen.h` is empty, the plugin interface version is still 104, and every shipped plugin loads without error in Plugin Manager (FR-012, SC-009)

**Checkpoint**: Every locale site and every plugin has a recorded outcome.

---

## Phase 6: Polish & Validation

- [ ] T028 [P] Regression sweep on screen: panel columns (name, size, date, time), the directory line, the size-reporting dialogs (occupied space, properties), and the information line while browsing inside an archive (FR-008, SC-006)
- [X] T029 [P] Hold an arrow key through a directory of several thousand items and confirm panel navigation feels unchanged — the information line is rebuilt on every focus change
- [ ] T030 [P] Temporarily switch Windows regional settings to a locale whose short date or time format contains a non-ASCII character, restart, and confirm the date and time fields display correctly and do not poison the name; restore the original settings afterwards (FR-006, SC-005)
- [X] T031 [P] Confirm `git diff src/common/salunicode.h` shows additions only, with `SalU8ToW` / `SalWToU8` signatures and semantics unchanged and no existing caller altered (Constitution III, INV-6)
- [X] T032 Write `specs/041-fix-infoline-encoding/validation-results.md`: requirement coverage, the T004 classification table, the T026 plugin table, before/after evidence for the reported file, and any observation left out of scope
- [X] T033 Walk `specs/041-fix-infoline-encoding/quickstart.md` end to end and confirm every documented check passes as written

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies. T001 must precede T002 (nothing to run otherwise).
- **Foundational (Phase 2)**: T005 and T006 **block both user stories**. T004 is independent of T005/T006 but must precede T023.
- **User Story 1 (Phase 3)**: depends on T005, T006. Independent of User Story 2.
- **User Story 2 (Phase 4)**: depends on T006. Independent of User Story 1 — it could be verified first, but is sequenced after so the P1 defect is closed earliest.
- **Phase 5**: T023 depends on T004 and T005. T026/T027 depend on T006.
- **Phase 6**: depends on all preceding phases.

### User Story Dependencies

- **User Story 1 (P1)**: starts after T006. No dependency on User Story 2.
- **User Story 2 (P2)**: starts after T006. Shares the root-cause fix with User Story 1 but touches different code and is verified separately.

### Within Each User Story

- Source edits before the build; the build before the on-screen checks
- T009 before T010 (the conversion must exist before it can be used)

### Parallel Opportunities

- T003 is independent of T001/T002 and is marked [P]
- T005 and T006 touch different files but T006 consumes T005's wrapper — **not** parallel
- T008 (`execute.cpp`) and T009/T010/T011 (`salunicode.*`, `stswnd.cpp`) touch disjoint files and could proceed in parallel, but all feed the same build so they are listed sequentially
- T028–T031 are independent read-only or observational checks, all marked [P]
- T026's 18 plugin reviews are independent of one another and can be split

---

## Parallel Example: Phase 6

```text
# Four independent checks, no shared state:
Task: "T028 Regression sweep across panel, directory line and size dialogs"
Task: "T029 Panel navigation responsiveness with a large directory"
Task: "T030 Non-ASCII date/time locale check"
Task: "T031 Confirm strict conversion helpers unchanged"
```

---

## Implementation Strategy

### MVP First (User Story 1 only)

1. Phase 1 — build, reproduce, record
2. Phase 2 — wrappers and separators (blocking)
3. Phase 3 — information line correct and verified
4. **STOP and VALIDATE**: the reported file reads correctly and the threshold no
   longer matters
5. Shippable — the reported bug is fixed

### Incremental Delivery

1. Setup + Foundational → locale text is UTF-8 at the source
2. Add User Story 1 → reported defect fixed → shippable (MVP)
3. Add User Story 2 → the second manifestation, including the Find dialog
4. Phase 5 → every locale site and every plugin accounted for
5. Phase 6 → regressions, performance, documentation

---

## Notes

- Every task names the requirement(s) it satisfies, so traceability runs
  spec → task → verification without a separate matrix
- The riskiest task is T023: converting a Group A site whose consumer turns out
  to be ANSI-only would trade one mojibake for another. T004's tracing is what
  prevents that, and T028 is what catches it if the tracing missed something
- T026 is expected to find no affected plugin, but "expected" is why it is a
  task: the review's value is the recorded outcome, not the fix count
- Commit after each phase; each is independently revertable

---

## Status at the end of implementation

29 of 33 tasks complete. Two remain deliberately open and one is partial —
recorded in `validation-results.md` under "Not verified" rather than silently
ticked:

- **T028** (partial): panel columns, directory line and the Find dialog were
  swept; the size-reporting dialogs and archive browsing were not reached.
- **T030** (not done): verifying a locale whose date/time format is non-ASCII
  means changing the machine's Windows regional settings. That is a change to
  the user's system and was not made without asking. The code path is in place;
  the observation is missing.
- **T026** (complete as a review, partial as verification): all 18 plugins are
  classified, but the 8 that format a number into their own dialog text were not
  runtime-verified. This is the most likely place for a residual defect.

Two regressions were introduced during implementation and fixed; the second one
changed the design (a global `LoadStr` conversion was tried and reverted in
favour of a bounded `LoadStrU8`). Both are written up in `validation-results.md`.
