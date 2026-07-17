# Tasks: Fix Application Crash When Entering a Long-Path Directory

**Input**: Design documents from `/specs/011-fix-longpath-crash/`
**Prerequisites**: plan.md, spec.md, research.md (audit R2, decisions R3)

**Tests**: Automated verification only (autonomous run — spec Clarifications):
scripted keystroke drive of the built app + static sweep. No unit-test
framework exists in the repo.

**Organization**: The audit is complete (research.md R2); tasks map its
verdicts to fixes. US1 = crash-free entry (global corruptor), US2 = stable
work inside (action-triggered overflows), US3 = audit closure + verification.

## Format: `[ID] [P?] [Story] Description`

## Phase 1: Setup

- [X] T001 Verify baseline Debug x64 build passes on branch `011-fix-longpath-crash` (`build.cmd`)

## Phase 2: Foundational

*(none — no shared infrastructure needed; audit already complete in research.md)*

## Phase 3: User Story 1 — Entering a long-path directory never crashes (P1) 🎯 MVP

- [X] T002 [US1] Fix `CMainWindow::UpdateDefaultDir` in `src/mainwnd1.cpp:483,488`: bound both `strcpy(DefaultDir[...], path)` copies — when the path fits `MAX_PATH` copy verbatim, otherwise store the drive root via `GetRootPath` (decision D1; rows must stay `MAX_PATH` for plugin consumers)
- [X] T003 [P] [US1] Fix listing-error report in `src/fileswn3.cpp:871`: `sprintf(buf, IDS_CANNOTREADDIR, GetPath(), GetErrorText)` → `_snprintf_s(_TRUNCATE)` (fires when listing a long dir fails)
- [X] T004 [P] [US1] Widen `EditWindowSetDirectory` buffer in `src/mainwnd1.cpp:1172` from `char dir[2*MAX_PATH]` to `char dir[SAL_MAX_PATH_UTF8]` so the command line receives the full path (`CInnerText` stores dynamically and compacts for display)

## Phase 4: User Story 2 — Working inside a long-path directory is stable (P2)

- [X] T005 [US2] Fix `CFilesWindow::RenameFileInternal` in `src/fileswn5.cpp:2126-2185`: `tgtPath[MAX_PATH]` + unbounded `memmove` and `path[MAX_PATH]` → `SAL_MAX_PATH_UTF8` buffers; convert every `< MAX_PATH` guard in the function to `sizeof`-based bounds
- [X] T006 [US2] Fix file-times buffer in `src/fileswn5.cpp:250`: `fileName[MAX_PATH]` + `strcpy(GetPath())` + `SalPathAppend(..., MAX_PATH)` → `SAL_MAX_PATH_UTF8` (same file as T005 — apply together)
- [X] T007 [P] [US2] Guard `CFilesWindow::DeleteThroughRecycleBin` in `src/fileswn8.cpp:79`: if `strlen(GetPath())` does not fit the `MAX_PATH` buffer, show the standard too-long-name error and return FALSE (the shell recycle bin cannot handle long paths — safe degradation per FR-002); harden the `textBuf` sprintf with `_snprintf_s`
- [X] T008 [P] [US2] Widen copy/move script source buffer in `src/fileswn6.cpp:1241`: `sourcePath[2*MAX_PATH+10]` → `SAL_MAX_PATH_UTF8 + 10`
- [X] T009 [P] [US2] Bound unpack-target prefill in `src/fileswn7.cpp:1785`: `strcpy(path, GetPath())` → `lstrcpyn` with the dialog buffer size (verify the buffer's declared size first)
- [X] T010 [P] [US2] Harden bug-report path line in `src/bugreprt.cpp:1428`: `sprintf(buf, "Path = %s", panel->GetPath())` → `_snprintf_s(_TRUNCATE)`

## Phase 5: User Story 3 — Audit closure + automated verification (P3)

- [X] T011 [US3] Static closure sweep: re-grep the core for unbounded `strcpy`/`lstrcpy`/`sprintf` of `GetPath()`/`GetGeneralPath` into fixed buffers; confirm zero remaining CRASH verdicts; record the final audit state in `specs/011-fix-longpath-crash/research.md` (append a "Closure" section)
- [X] T012 [US3] Build Debug x64 (`build.cmd`), clang-format touched files, rebuild
- [X] T013 [US3] Automated end-to-end verification: scripted drive of the built app — (a) Enter-navigation of the full test tree (ASCII chain, Unicode L1→L3, 255-char component, edge-260) with survival asserts; (b) F2 rename round-trip inside the deepest dir; (c) Delete (recycle bin) attempt inside a long dir → expect graceful error, process alive; (d) proper close (WM_CLOSE) → restart → survival (startup-restore degradation); (e) sub-260 regression smoke
- [X] T014 [US3] Commit to branch and merge/fast-forward `main` per the established workflow; update memory notes

## Dependencies & Execution Order

- T001 → all. T002 first (fires on every entry). T003/T004 parallel to T002.
- T005+T006 same file (sequential pair), T007–T010 parallel.
- T011–T014 after all fixes.

## Implementation Strategy

Single pass, all fixes in one commit (they are one defect class), then
verification. MVP = T002 alone stops the every-entry global corruption; the
rest closes the action-triggered crashes.
