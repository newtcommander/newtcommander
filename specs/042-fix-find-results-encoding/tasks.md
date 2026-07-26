---
description: "Task list for 042-fix-find-results-encoding"
---

# Tasks: Fix File Name Encoding in Find Results and Name Notices

**Input**: Design documents from `/specs/042-fix-find-results-encoding/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: Test tasks ARE included — FR-016 and FR-017 require automated regression
protection as a shipped deliverable, and SC-012 requires it demonstrated failing.

**Organization**: Grouped by user story so each is independently implementable and
testable. US1 and US2 are both P1 and fully independent of each other.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on incomplete work)
- **[Story]**: US1 / US2 / US3 / US4 — maps to the user stories in spec.md

## Path Conventions

Single MSBuild solution at repository root. Application sources in `src/`, tests in
`src/saltests/`, build tooling in `tools/` and `build.cmd`. Spec deliverables in
`specs/042-fix-find-results-encoding/`.

---

## Phase 1: Setup

**Purpose**: Establish the baseline and capture "before" evidence, so every later
claim of repair is measured against something recorded rather than remembered.

- [X] T001 Confirm the fixture set exists and is intact at `C:\Users\pavel\AppData\Local\Temp\salamander-test` — the `010` subtree (Latin/Greek/Cyrillic/CJK/emoji/long-paths) and the canonically equivalent `č-dir` pair per quickstart.md
- [X] T002 Produce a clean baseline Debug x64 build via the repository-root `build.cmd` with `OPENSAL_BUILD_DIR` set, and record that it succeeds before any change
- [X] T003 [P] Capture "before" evidence for Report 1: run the reported Ctrl+F search (`🙂-d` in `…\salamander-test\010`) and screenshot the results list showing `emoji-??-dir`, into `specs/042-fix-find-results-encoding/evidence/before-find.png`
- [X] T004 [P] Capture "before" evidence for Report 2: with the UI language set to Czech, enter `…\salamander-test` and screenshot the notice showing `ÄŤ-dir`, into `specs/042-fix-find-results-encoding/evidence/before-notice.png`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Build the measurement and verification instruments the whole feature
depends on. `tools/check_encoding.py` is built here rather than late because it is
the *same program* twice: first the FR-009 inventory instrument, later the FR-016
guard. Writing it once, early, is what makes the inventory reproducible.

**⚠️ CRITICAL**: No repair work should begin until T005–T008 are complete.

- [X] T005 Create `tools/check_encoding.py` implementing the three forbidden-pattern rules from research.md R6: (1) `WideCharToMultiByte(CP_ACP, …)` on a name-carrying path; (2) a printf-family call with a `LoadStr(` format plus a name argument reaching a message box; (3) an `LVN_GETDISPINFOW` handler in a dialog that never sends `NF_REQUERY`. Report-only mode by default; exit non-zero under `--strict`
- [X] T006 Run `tools/check_encoding.py` against the unmodified tree and record the baseline hit counts per rule into `specs/042-fix-find-results-encoding/inventory.md` — this is the "before" measurement the inventory is built from
- [X] T007 [P] Establish the GUI verification harness: a script that launches the built `newtcommander.exe`, drives it to a named surface, and captures a screenshot. Must enumerate top-level windows and match on class/title — the process `MainWindowHandle` is the splash screen, not the main window
- [X] T008 [P] Establish and document the language-switch verification procedure in `specs/042-fix-find-results-encoding/quickstart.md`: `build.cmd full` to regenerate language modules, then switching the UI language across all 9 shipped languages (English, czech, german, french, dutch, hungarian, romanian, slovak, spanish)

**Checkpoint**: Instruments ready — the inventory is measurable and every surface is reachable and photographable.

---

## Phase 3: User Story 1 - Found names read correctly (Priority: P1) 🎯 MVP

**Goal**: The Find results list shows every found name exactly as the file system
stores it, for all scripts and symbols.

**Independent Test**: Run the reported search (`🙂-d` under `…\salamander-test\010`)
and compare each row of the Name column against the directory listing on disk.

### Implementation for User Story 1

- [X] T009 [US1] In `src/finddlg1.cpp`, send `SendMessage(FoundFilesListView->HWindow, WM_NOTIFYFORMAT, (WPARAM)HWindow, NF_REQUERY)` from the `WM_INITDIALOG` handler of `CFindDialog::DialogProc`, so the control re-asks for its notification format at a point where the parent object is attached and can answer (research.md R1, contracts/notification-format.md §1)
- [X] T010 [US1] Confirm at runtime that `LVN_GETDISPINFOW` (`src/finddlg1.cpp:3878`) is now the live handler and the ANSI `LVN_GETDISPINFO` (`:4162`) no longer fires — a temporary `TRACE` in both, removed before commit
- [X] T011 [US1] Remove the lossy `WideCharToMultiByte(CP_ACP, …)` conversion block from the ANSI `LVN_GETDISPINFO` handler in `src/finddlg1.cpp` (added by feature 041 at `:4174-4185`), per FR-002 — an ANSI handler may remain only as an ASCII-only fallback
- [X] T012 [US1] In the `LVN_GETDISPINFOW` handler in `src/finddlg1.cpp`, use the lenient `SalU8ToWDisplay` rather than strict `SalU8ToW`, so a malformed stored name costs exactly one `U+FFFD` instead of blanking the whole cell (FR-005, current code blanks on failure at `:3887-3891`)
- [X] T013 [US1] Update the stale comment at `src/finddlg1.cpp:3864-3866` which asserts the `WM_NOTIFYFORMAT` handler is what makes the list Unicode — record that the `NF_QUERY` arrives before `WM_INITDIALOG` and only `NF_REQUERY` makes it effective

### Verification for User Story 1

- [X] T014 [US1] Verify the reported search shows `emoji-🙂-dir`, `emoji-🙂-dir - Copy`, `emoji-🙂-dir - Copyě 😍😍😍` and the fourth match with no `?` absent from disk (SC-001); capture into `evidence/after-find.png`
- [X] T015 [P] [US1] Verify names in Latin, Greek, Cyrillic and CJK scripts across the `010` fixtures match the file system exactly, and match what a file panel shows for the same items (SC-002)
- [X] T016 [P] [US1] Verify the no-regression set: the Path column, sorting by Name, long-path results, and the searched-directory progress text all behave as before (FR-012)
- [X] T017 [P] [US1] Verify a malformed (unpaired-surrogate) name shows exactly one `�` with every other column of that row intact (FR-005)
- [X] T018 [P] [US1] Verify a 10,000-item result set scrolls with no perceptible delay, unchanged from baseline (SC-009)

**Checkpoint**: The reported defect is fixed and demonstrated. This alone is a shippable increment.

---

## Phase 4: User Story 2 - The duplicate-name notice reads correctly (Priority: P1)

**Goal**: The duplicate-name notice shows the name it is talking about, correctly,
while its surrounding localized text stays correct.

**Independent Test**: With a localized UI, enter
`C:\Users\pavel\AppData\Local\Temp\salamander-test` and read the notice.

**Independence note**: Shares no code with US1. Can be implemented before, after or
alongside it.

### Implementation for User Story 2

- [X] T019 [US2] In `src/fileswnb.cpp:815`, change `LoadStr(IDS_EQUIVNAMESPAIR)` to `LoadStrU8(IDS_EQUIVNAMESPAIR)` so the composed message is UTF-8 end to end and `CMessageBox` accepts its own wide drawing path (research.md R3, contracts/composed-message.md §1)
- [X] T020 [US2] Add an English comment at that call site recording *why* the U8 variant is required — that a single ANSI ingredient costs the whole message its wide path — so a future edit does not silently revert it

### Verification for User Story 2

- [X] T021 [US2] Verify in Czech that the notice reads `č-dir` with no `Ä`/`Å`/`Ã`/`â€` sequence anywhere, and that the surrounding Czech sentences are still correct (SC-001); capture into `evidence/after-notice.png`
- [X] T022 [US2] Verify the notice in all 9 shipped languages — localized text and name simultaneously correct in each (SC-004, FR-009c: this is a composed surface)
- [X] T023 [P] [US2] Verify with a duplicate pair whose names use emoji and CJK characters that the name is shown exactly, with no `?` substitution
- [X] T024 [P] [US2] Confirm `SalMessageBox`, `CMessageBox` and `LoadStr` are unmodified — `git diff` over `src/msgbox.cpp` and `src/salamdr2.cpp` must be empty (FR-014, FR-014a)

**Checkpoint**: Both reported defects are fixed and independently demonstrated.

---

## Phase 5: User Story 3 - Type-to-search reaches non-ASCII names (Priority: P2)

**Goal**: Typing the leading characters of a visible result selects it, for any script.

**Independent Test**: In a results list containing non-ASCII names, type the leading
characters of one and observe the selection.

**Dependency**: Requires T009 (the list must be in wide notification mode before
`LVN_ODFINDITEMW` can fire).

### Implementation for User Story 3

- [X] T025 [US3] Confirm `LVN_ODFINDITEMW` (`src/finddlg1.cpp:3897`) is now the live handler and that its `SalWToU8` + `SalNameEqualCI` comparison path executes
- [X] T026 [US3] Retire or ASCII-gate the ANSI `LVN_ODFINDITEM` handler in `src/finddlg1.cpp` (the `StrICmp`/`StrNICmp` comparisons around `:4118-4147`), which compares ANSI text against UTF-8 stored names and silently fails for every non-ASCII name (FR-006)

### Verification for User Story 3

- [X] T027 [US3] Verify typing the leading characters of `Тест-Ελλάδα-测试 +ěš` selects that item (SC-003)
- [X] T028 [P] [US3] Verify that with `emoji-🙂-dir` and `emoji-🙂-dir - Copy` present, typing `emoji-` selects the first and typing again advances to the next match
- [X] T029 [P] [US3] Verify ASCII type-to-search is unchanged, and that text matching no item moves the selection nowhere and raises no error

**Checkpoint**: All three symptoms of the two reports are resolved.

---

## Phase 6: User Story 4 - Every remaining affected surface is found and repaired (Priority: P2)

**Goal**: Wherever a name appears in the application it is correct, and a written
inventory records every place a name is composed into displayed text.

**Independent Test**: Read the inventory, pick any surface at random, exercise it
against the fixture set, and confirm the recorded verdict.

**⚠️ This phase contains the feature's only open-ended item. T033 is a hard gate.**

### Inventory (FR-009) — must complete before any repair in this phase

- [X] T030 [US4] Run the mechanical axis: `tools/check_encoding.py` over `src/` excluding `plugins/`, `saltests/`, `tserver/`, `shellext/`, `setup/`, `salmon/`; record every hit with file, line, rule and display route (FR-009a). Expected order of magnitude from research.md R5: 75 sites under a strict name pattern, 119 under a broadened one
- [X] T031 [US4] Run the user-interface axis: walk the reachable dialogs, notices, captions and status surfaces against the fixture set, in a localized language, and record what is actually broken (FR-009a)
- [X] T032 [US4] Reconcile the two axes in `specs/042-fix-find-results-encoding/inventory.md`. For every surface record: which axis found it, its verdict (verified-correct / to-fix / deferred-with-reason), how the verdict was reached, and its language class — composed-with-localized-text vs bare-name (FR-009a, FR-009b, FR-009c). Any surface the UI walk found that the mechanical pass missed MUST also have the missing pattern recorded — research.md R5 already documents one such miss (`fileswnb.cpp:815`, the reported defect itself)
- [X] T033 [US4] **SCOPE GATE**: compare the classified work list against the ~119-candidate estimate. If materially larger, stop and re-cut scope with the real number rather than absorbing it — this is the checkpoint the spec's Assumptions reserve for exactly this moment. Record the decision in `inventory.md`

### Repair (FR-010) — only after T033

- [X] T034 [US4] Apply the US1 repair to `src/packac.cpp`: send `NF_REQUERY` from its dialog initialisation so its `LVN_GETDISPINFOW` handler (`:198`) becomes reachable, and correct the inaccurate comment at `:181` claiming the ANSI route is active "only if the Unicode format was refused" — it has always been refused there (research.md R2)
- [X] T035 [US4] Repair the composed-message sites in the file-panel operation sources — `src/fileswn0.cpp`, `fileswn2.cpp`, `fileswn3.cpp`, `fileswn5.cpp`, `fileswn6.cpp`, `fileswn7.cpp`, `fileswn8.cpp`, `fileswn9.cpp`, `fileswna.cpp`, `fileswnb.cpp` — switching `LoadStr` to `LoadStrU8` at each site the inventory marks to-fix
- [X] T036 [P] [US4] Repair the composed-message sites in the dialog sources — `src/dialogs3.cpp`, `dialogs4.cpp`, `dialogs5.cpp`, `dialogs6.cpp`
- [X] T037 [P] [US4] Repair the composed-message sites in the core/shell sources — `src/salamdr1.cpp`, `salamdr2.cpp`, `salamdr3.cpp`, `salamdr5.cpp`, `salshlib.cpp`, `shellsup.cpp`, `execute.cpp`, `callstk.cpp`
- [X] T038 [P] [US4] Repair the composed-message sites in the main-window and viewer sources — `src/mainwnd1.cpp`, `mainwnd2.cpp`, `mainwnd3.cpp`, `mainwnd4.cpp`, `mainwnd5.cpp`, `viewer3.cpp`, `codetbl.cpp`, `plugins1.cpp`
- [X] T039 [US4] Re-run `tools/check_encoding.py` and confirm every to-fix hit is gone and every remaining hit corresponds to an inventory entry explicitly marked deferred (FR-010)

### Verification for User Story 4

- [X] T040 [US4] Verify every repaired surface in the running application, per its recorded language class: composed surfaces in all 9 languages, bare-name surfaces once (SC-007, FR-009c)
- [X] T041 [P] [US4] Spot-check three entries the inventory marks "verified correct" by exercising them against the fixture set (SC-006)
- [X] T042 [P] [US4] Confirm every entry marked "deferred" states its reason and its revisit condition (FR-009, spec User Story 4 scenario 4)

**Checkpoint**: The defect class — not just its two instances — is repaired and documented.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: The recurrence protection, the plugin review, and the evidence record.
FR-017 requires the guard demonstrated against completed fixes, which is why it is
wired up here rather than in Phase 2.

### Automated protection (FR-016, FR-017)

- [X] T043 Add a regression suite to `src/saltests/saltests.cpp` alongside the existing `TestConversions`/`TestNormalization` groups, asserting: a localized template composed with a non-ASCII name round-trips as UTF-8; `LoadStrU8` output is valid UTF-8 for a non-ASCII resource string; lenient conversion yields exactly one `U+FFFD` per malformed unit with neighbours intact; truncation never splits a surrogate pair (FR-016a)
- [X] T044 Wire `tools/check_encoding.py --strict` into `build.cmd` so a forbidden pattern fails the ordinary build with no manual step a contributor could skip (FR-016b, SC-013)
- [X] T045 Demonstrate the guard by reverting the US1 fix alone and observing the build fail; restore, revert the US2 fix alone and observe the build fail; restore and confirm both pass. Record all three outcomes (FR-017, SC-012)

### Plugin review (FR-011, FR-014)

- [X] T046 [P] Review each of the 18 shipped plugins for this defect class and record the finding per plugin in `specs/042-fix-find-results-encoding/validation-results.md`. No plugin source may be modified (FR-011)
- [X] T047 [P] Verify plugin output is unchanged: exercise at least one plugin that displays file names in a message box, before and after, and confirm the dialogs are visually identical (SC-008a)
- [X] T048 Confirm the plugin interface is untouched — `git diff` over `src/plugins.h` and `src/spl_gen.h` empty, plugin ABI version still 104 (FR-014)

### Regression re-verification (FR-013)

- [X] T049 [P] Re-verify feature 041's surfaces: the panel information line with the reported `.mkv` file, and the selection summary in all 9 shipped languages (FR-013, SC-005)
- [X] T050 [P] Re-verify feature 041's edge cases: the size-threshold cases at 999 B and 2,000,000 B, the unpaired-surrogate case, and the truncation sweep (SC-005)

### Close-out

- [X] T051 Confirm SC-010 structurally: `tools/check_encoding.py` covers every name-carrying display path and reports no legacy-codepage conversion on any of them — codepage independence by construction, no machine locale changed (Clarifications Q4)
- [X] T052 Write `specs/042-fix-find-results-encoding/validation-results.md` recording every verification, its outcome, and — explicitly — anything not verified and why, following the precedent set by feature 041 (FR-015)
- [X] T053 Run `clang-format` / `normalize.ps1` over every modified source file per the project's formatting rule
- [X] T054 Produce a clean Release x64 build via `build.cmd full release` and confirm `saltests` passes with zero failures
- [X] T055 Run the full `quickstart.md` validation end to end as a final acceptance pass

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: no dependencies
- **Foundational (Phase 2)**: depends on Setup — blocks all repair work
- **US1 (Phase 3)** and **US2 (Phase 4)**: both depend only on Foundational, and are **independent of each other** — either can ship alone
- **US3 (Phase 5)**: depends on T009 (US1) — the list must be in wide notification mode first
- **US4 (Phase 6)**: depends on Foundational; its repairs additionally depend on the T033 scope gate
- **Polish (Phase 7)**: depends on US1 and US2 for the FR-017 demonstration; on US4 for the inventory-driven claims

### Critical Gate

**T033 blocks T034–T042.** No repair in Phase 6 begins until the inventory is
complete and its size accepted. This is the spec's own scope checkpoint.

### Parallel Opportunities

- T003, T004 (evidence capture) in parallel
- T007, T008 (harness, language procedure) in parallel
- **US1 and US2 can be developed fully in parallel by two people** — disjoint files, disjoint mechanisms
- Within US1: T015–T018 verification in parallel
- Within US2: T023, T024 in parallel
- Within US4: T036, T037, T038 repair batches in parallel (disjoint file sets); T035 is listed separately because it includes `fileswnb.cpp`, already touched by US2
- Within Polish: T046/T047, T049/T050 in parallel

### Parallel Example: the two P1 stories

```text
Developer A: T009 → T010 → T011 → T012 → T013 → T014 → [T015, T016, T017, T018]
Developer B: T019 → T020 → T021 → T022 → [T023, T024]
```

---

## Implementation Strategy

### MVP scope

**User Story 1 alone** is a coherent shippable increment: it fixes the originally
reported defect, is independently testable, and touches one file.

**US1 + US2** is the recommended first delivery — both are P1, both were reported,
and together they cost little more than US1 alone because they share no code.

### Incremental delivery

1. Setup + Foundational → instruments ready, baseline recorded
2. **US1** → reported Find defect fixed → demonstrable
3. **US2** → reported notice defect fixed → demonstrable
4. **US3** → the unreported third symptom closed
5. **US4** → the class repaired, inventory written (gated at T033)
6. **Polish** → recurrence protection proven, plugins reviewed, evidence recorded

Stopping after step 3 delivers everything the user reported. Steps 4–6 are what
prevent the next report, which is the stated reason this feature exists.

### Risk notes carried from plan.md

- The Find dialog is a known-fragile neighbour: feature 041 broke it with a
  comparable change and reverted. T016 exists specifically to catch that.
- English builds cannot reproduce US2's defect (research.md R3). T022's
  all-9-languages check is not thoroughness theatre — English is the one
  configuration guaranteed to pass regardless.
- The mechanical pass has a proven blind spot. T031's UI walk is load-bearing.

---

## Task Summary

| Phase | Tasks | Count |
|---|---|---|
| 1 — Setup | T001–T004 | 4 |
| 2 — Foundational | T005–T008 | 4 |
| 3 — US1 (P1) 🎯 MVP | T009–T018 | 10 |
| 4 — US2 (P1) | T019–T024 | 6 |
| 5 — US3 (P2) | T025–T029 | 5 |
| 6 — US4 (P2) | T030–T042 | 13 |
| 7 — Polish | T043–T055 | 13 |
| **Total** | | **55** |

Parallelizable tasks: 21 marked `[P]`.
