# Tasks: Directory-Listing Crash on Long Multi-Byte Names — Review & Regression Protection

**Input**: Design documents from `/specs/031-longpath-enum-crash/`
**Prerequisites**: plan.md, spec.md, research.md (site inventory R2), data-model.md (buffer rule), quickstart.md

**Tests**: Regression tests are explicitly required by the spec (FR-006, SC-003) — test tasks included (US3).

**Organization**: Tasks grouped by user story; site numbers (#1–#8) refer to the research.md R2 inventory.

## Format: `[ID] [P?] [Story] Description`

## Phase 1: Setup

**Purpose**: Green baseline before any change

- [X] T001 Verify baseline: `build.cmd` (Debug x64) succeeds and `build\salamander\Debug_x64\saltests\saltests.exe` reports 0 failed (record check count)
- [X] T002 Verify the repro directory exists (`D:\Temp\ýášřtščýáíf …`, 215 chars); if missing, recreate per quickstart.md

**Checkpoint**: baseline green, repro available

---

## Phase 2: Foundational

No foundational tasks — the required infrastructure (`SAL_FIND_NAME_U8` in
`src\common\salfileio.h`, `SalWToU8`/`SalConvertFindDataW`, the saltests
harness) already exists from features 004/027.

---

## Phase 3: User Story 1 — Browsing a folder with very long names never crashes (P1) 🎯 MVP

**Goal**: eliminate every stack/global smash reachable by listing/painting a
panel containing a long multi-byte name (R2 sites #1–#6) and the Tiles
rendering corruption (#7–#8), per the data-model.md buffer rule: widen to
`SAL_FIND_NAME_U8 + 4`, guard over-long plugin names (simple-symbol
fallback), bound extension-lowercase loops, add `static_assert` fence at
every touched site.

**Independent Test**: launch the built app, enter `D:\Temp` — listing paints
in all view modes with the full name, no crash, no new WER dump.

- [X] T003 [US1] Fix `CFilesWindow::DrawIcon` in `src\fileswn4.cpp`: widen `fileName[MAX_PATH+4]` (line 220) to `SAL_FIND_NAME_U8 + 4`, guard both `memmove` sites (#1 dir branch lines 227–228 — the dump-confirmed crash — and #2 file branch lines 255–256) with the `NameLen + 4 <= sizeof` check → `drawSimpleSymbol` fallback; add `static_assert`
- [X] T004 [US1] Fix `CFilesWindow::DrawIcon` in `src\fileswn4.cpp`: widen `lowerExtension[MAX_PATH+4]` (line 136) and bound the `f->Ext` lowercase loop (lines 169–172) by buffer capacity (#5); add `static_assert`
- [X] T005 [US1] Fix `CFilesWindow::DrawIconThumbnailItem` in `src\fileswn4.cpp`: same widen+guard for `fileName` (#3, lines 1306–1308); add `static_assert`
- [X] T006 [P] [US1] Fix `WM_USER_REFRESHINDEX` handler in `src\fileswnb.cpp`: widen `buf[MAX_PATH+4]` (line 702), bound the ext lowercase loop (lines 703–706), guard the `memmove` (lines 716–717) (#4); add `static_assert`
- [X] T007 [P] [US1] Fix `InternalGetType` in `src\salamdr4.cpp`: widen global `InternalGetTypeAux3[MAX_PATH+4]` (line 1348) and bound its lowercase loop (lines 1363–1367) (#6); add `static_assert`
- [X] T008 [P] [US1] Fix the synchronized Tiles pair (#7 `src\filesbx1.cpp` lines 2070–2076, #8 `src\fileswn0.cpp` lines 3395–3401): enlarge the `out0` name region per data-model.md layout (`buff[(SAL_FIND_NAME_U8+4) + 2*512]`); add `static_assert` in both
- [X] T009 [US1] Build Debug x64 (`build.cmd`), run `saltests.exe` (still 0 failed), launch `salamand.exe`, navigate to `D:\Temp` in Brief/Detailed/Icons/Thumbnails/Tiles — no crash, no new dump in `%LOCALAPPDATA%\CrashDumps`

**Checkpoint**: US1 delivers the MVP — the reported crash is gone

---

## Phase 4: User Story 2 — All everyday operations work on such entries (P2)

**Goal**: close the review of the remaining flagged sites and prove the
operation matrix on the repro entry (spec FR-003/FR-004/FR-005).

**Independent Test**: operation matrix on the repro directory completes
with names intact; flagged sites re-verified with recorded verdicts.

- [X] T010 [P] [US2] Re-verify flagged sites and record verdicts in `specs/031-longpath-enum-crash/research.md` R2: `src\fileswn3.cpp:631-636` (ext lowercase into `nameU8` — confirm 764+4 ≤ 780 bound incl. `st` start offset) and `src\drivelst.cpp:869` (`root[MAX_PATH+4]` + `GetRootPath` UNC bound); fix any CRASH verdict using the data-model.md rule
- [X] T011 [P] [US2] Sweep-confirm the R2 SAFE list (remaining `LowerCase[*…]` copy loops and `->Name/->NameLen` copies in core `src\*.cpp`): verify each verdict, extend the research.md inventory with any new finding (fix CRASH finds immediately)
- [X] T012 [US2] Operation matrix on the repro entry with the fixed Release build: enter/up, create+rename+delete a file inside, rename the long dir (and rename back), F5 copy to `%TEMP%` and back, Ctrl+C/Ctrl+V clipboard copy, delete of the copy, focus + Type column render; record results in `specs/031-longpath-enum-crash/validation-results.md`

> T012 note: executed as far as the environment allows — listing/entering/
> view-mode/Type-column verification ran live on the Release build (WM_COMMAND
> driving; `SendInput` is blocked in this session, so dialog-driven F5/Ctrl+V
> steps could not be keyboard-automated); the file-operation engine routes are
> covered by saltests on-disk checks (create/enumerate/rename/copy/delete with
> the repro name and at >300-char depth). Details in validation-results.md
> SC-002; interactive GUI walkthrough left to the user (027 precedent).

**Checkpoint**: operation surface verified, review closed

---

## Phase 5: User Story 3 — The defect class is fenced off against regressions (P3)

**Goal**: automated fence per research.md R4 (three layers: static_asserts
are added with US1 tasks; this phase adds the saltests layers).

**Independent Test**: `saltests.exe` contains the new checks and passes;
reverting a buffer widening breaks the build (static_assert).

- [X] T013 [US3] Add `TestLongComponentNames` to `src\saltests\saltests.cpp`: 215-char diacritics repro name → `SalWToU8` == 330 bytes (`> MAX_PATH+4`, `< SAL_FIND_NAME_U8`); 255 × 3-byte char → 765 bytes fits `SAL_FIND_NAME_U8`; 127 surrogate pairs → fits; `SalConvertFindDataW` round-trips a max-length `WIN32_FIND_DATAW` name exactly; deliberately small buffer yields `""` (fail-safe, never truncation); wire the function into `main()`
- [X] T014 [US3] Extend `TestFileIO` in `src\saltests\saltests.cpp`: create the exact 215-char diacritics directory under `%TEMP%\saltests-deep\`, enumerate the parent with `SalFindFirstFile`/`SalFindNextFile`, assert byte-exact name (330 bytes) is returned, then remove it (keep the graceful skip when no temp path)
- [X] T015 [US3] Rebuild Debug x64, run `saltests.exe` — all checks green; then prove the fence: temporarily change one fixed buffer in `src\fileswn4.cpp` back to `MAX_PATH + 4`, confirm the build FAILS on the static_assert, restore the fix (SC-003 evidence — record in validation-results.md)

**Checkpoint**: regression class fenced

---

## Phase 6: Polish & Cross-Cutting

- [X] T016 [P] clang-format verification of all touched files (`src\fileswn4.cpp`, `src\fileswnb.cpp`, `src\salamdr4.cpp`, `src\filesbx1.cpp`, `src\fileswn0.cpp`, `src\saltests\saltests.cpp`, plus any from T010/T011)
- [X] T017 Full builds clean: `build.cmd` (Debug x64) and `build.cmd full release` (Release x64), zero new warnings at the touched sites
- [X] T018 Adversarial code review of the complete diff (off-by-one at every widened bound, DWORD-terminator room, guard inversions, Tiles region sync, static_assert placement); fix findings
- [X] T019 Finalize `specs/031-longpath-enum-crash/validation-results.md` (SC-001…SC-005 evidence: repeated-entry test, saltests counts, fence proof, inventory closure) and commit with `[031]` prefixes per repo convention

---

## Dependencies

- T001–T002 (Setup) → everything
- US1: T003 → T004 → T005 (same file, sequential); T006, T007, T008 parallel to each other and to the T003–T005 chain; T009 after T003–T008
- US2: T010, T011 parallel, anytime after Setup; T012 after T009 (needs fixed build)
- US3: T013, T014 sequential (same file); T015 after T013–T014 and after T003 (needs a static_assert to revert-test)
- Polish: T016–T019 after all stories

## Parallel Example (US1)

```
Chain A: T003 → T004 → T005   (src\fileswn4.cpp)
Chain B: T006                 (src\fileswnb.cpp)
Chain C: T007                 (src\salamdr4.cpp)
Chain D: T008                 (src\filesbx1.cpp + src\fileswn0.cpp)
Then:    T009                 (build + smoke)
```

## Implementation Strategy

MVP = Phase 3 (US1) alone: it removes the reported crash. Phases 4–5 close
the review and fence the class; Phase 6 polishes and documents. Total: 19
tasks (US1: 7, US2: 3, US3: 3, setup 2, polish 4).
