# Tasks: Long-Path File Operations

**Input**: Design documents from `/specs/012-longpath-file-operations/`
**Prerequisites**: plan.md, spec.md, research.md (audit R1–R6)

**Tests**: Automated verification only (autonomous / headless env): build
(Debug+Release), `-a` navigation, static sweep. No unit-test framework.

**Organization**: By the audit clusters (research.md). US1 = copy/move
(builder crash), US2 = view/edit, US3 = shell actions, US4 = panel
persistence/refresh, US5 = audit closure.

## Phase 1: Setup

- [X] T001 Confirm the long-path test tree exists (`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths`) with a 291-char dir containing a file, a short copy-target dir, and a file to copy in; regenerate if the earlier copy attempt damaged it
- [X] T002 Verify baseline Debug x64 build passes on branch `012-longpath-file-operations`

## Phase 2: Foundational (blocking — the single-file gate blocks every file op)

- [X] T003 Raise the `BuildName` gate in `src/salamdr1.cpp:1014`: `if (len >= MAX_PATH) return NULL` → `>= SAL_MAX_PATH_UTF8` (keep the heap `malloc(len+1)`); this unblocks single-file copy/move/delete/attrs for long paths (research R1 #12 / R4 #23). Verify no caller assumes the result ≤ MAX_PATH

## Phase 3: User Story 1 — Copy/move never crashes (P1) 🎯 MVP

- [X] T004 [US1] Fix the copy/move/delete script builder in `src/fileswn6.cpp` (research R1): widen `text[2*MAX_PATH+100]` (used at :1676,1884,2102,2168,2259), `finalName[2*MAX_PATH+200]` (:1874,2087), and `name[2*MAX_PATH]` (:2904) to `SAL_MAX_PATH_UTF8`-based sizes; convert every `sprintf(text/name, …, sourcePath)` and the `memmove(name, sourcePath, …)` to bounded `_snprintf_s(_TRUNCATE)` / size-checked copy. Prefer heap or a shared reused buffer if stack growth in one frame is excessive
- [X] T005 [US1] Fix `BuildScriptMain2` (paste/drag path building) in `src/fileswn6.cpp:613-669,962-987`: `sourcePath[2*MAX_PATH]` (:655 memcpy), `targetPath[2*MAX_PATH+200]` (:619 strcpy, :669 targetName), and `message[MAX_PATH+100]` (:963,975,987 sprintf) → SAL-sized with bounded copies
- [X] T006 [US1] Verify `finalName` truncation→wrong-dir (research R1 #11) is eliminated by the widening in T004 (no `SalFindFirstFile`/enumeration on a truncated path); confirm move never scripts a source it could not build

## Phase 4: User Story 2 — View/edit/execute (P2)

- [X] T007 [P] [US2] Fix `CFilesWindow::ViewFile` (F3) in `src/fileswn5.cpp:708/744/748/754`: widen `path[MAX_PATH+10]` → `SAL_MAX_PATH_UTF8`, replace the manual concat + `lstrcpyn(path,GetPath(),MAX_PATH)` + `>=MAX_PATH` gate with `SalPathAppend(path, name, sizeof(path))`; keep the DosName fallback (research R2 #13 — the reported F3 bug)
- [X] T008 [US2] Fix the viewer name intake in `src/viewer.cpp:562-563`: `name[MAX_PATH]` + `lstrcpyn(…,MAX_PATH)` → `SAL_MAX_PATH_UTF8`; verify `SalGetFullName` accepts it (heap `FileName` store is already safe)
- [X] T009 [US2] Fix `CFilesWindow::EditFile` (F4) in `src/fileswn5.cpp:1239/1268/1272/1278` (mirror of ViewFile) → SAL + `SalPathAppend`
- [X] T010 [P] [US2] Widen the viewer next/prev enum channel: `src/fileswnb.cpp:1288-1289` (`lstrcpyn(GetPath(),MAX_PATH)`+`SalPathAppend(…,MAX_PATH)`) and the transport struct fields `FileName`/`LastFileName` in `src/consts.h:2138/2142` (+ `src/salamdr6.cpp:170,205,223` handlers) → SAL (research R2 #15)
- [X] T011 [US2] Widen external viewer/editor expand buffers in `src/fileswn5.cpp:1070-1072/1104` and `:1393-1395/1427` (`expCommand/expArguments/expInitDir`, `cmdLine`) → SAL where internal; keep the residual `SalCreateProcess` OS command-line cap as a bounded error (safe degradation)

## Phase 5: User Story 3 — Shell actions never crash (P2)

- [X] T012 [US3] Fix the shared shell sink `GetShellFolder` in `src/shellib.cpp:1671-1675`: the `strcpy(root, dir)` of `GetPath()` into `root[MAX_PATH]`. First establish whether a wide `\\?\`/`ILCreateFromPathW` binding lets Properties/context work for long paths; if yes, convert to it; if not, bound the copy (no overflow) and return a failure the callers surface as a bounded "name too long" (safe degradation). Covers Properties, New, right-click menu, clipboard Copy/Cut, drag source (research R3 #19)
- [X] T013 [P] [US3] Fix `GetCurrentDir` in `src/shellsup.cpp:505-511`: hoist the `l + NameLen >= MAX_PATH` guard **before** the `memcpy(fullName, GetPath(), l)` (or widen `fullName`) so drag-drop onto a long-path panel cannot overflow (research R3 #20)
- [X] T014 [US3] Verify the shell callers (`shellsup.cpp` Properties/New/context/clipboard/drag paths) handle a `GetShellFolder` failure without crashing and show a bounded message (no `__try/__except` reliance for `/GS`)

## Phase 6: User Story 4 — Panel stays in / returns to a long path (P2)

- [X] T015 [US4] Make panel-path config restore long-path capable in `src/mainwnd2.cpp:2396` (+ callers' `leftPanelPath/rightPanelPath[MAX_PATH]` at :3827-3828, and `LoadPanelConfig`'s `panelPath` param): read into a SAL buffer / dynamic read so a saved >260 path round-trips (save at :1359 already writes full); fall back to a valid ancestor if the value genuinely will not fit (research R5 #28)
- [X] T016 [P] [US4] Fix `AcceptChangeOnPathNotification` in `src/fileswn7.cpp:2031-2054`: `lstrcpyn(path2, GetPath(), MAX_PATH)` → SAL so the change-refresh match uses the real path (research R5 #29)
- [X] T017 [US4] Widen the change-notification path channel: `COperations::WorkPath1/2[MAX_PATH]` + `SetWorkPath1/2` (`src/worker.h:275/277/313/319`), `dialogs.cpp:305/308` `workPath1/2`, and `CChangeNotifData::Path[MAX_PATH]` (`src/mainwnd.h:326`, `src/mainwnd3.cpp:516/4499`) → SAL so post-op refresh targets the real source/target (research R5 #30)
- [X] T018 [P] [US4] Fix the reparse/link delete path in `src/fileswn8.cpp:422`: `lstrcpyn(…, GetPath(), MAX_PATH+200)` → SAL (research R4 #27)

## Phase 7: User Story 5 — Audit closure (P3)

- [X] T019 [US5] Fix Create Directory (F7) in `src/fileswn5.cpp:1985`: pass `sizeof(path)` (not the default `MAX_PATH`) to `SalGetFullName` so a subdir beyond ~248 chars can be created; verify the `>=PATH_MAX_PATH` pre-gate at :1986 is widened consistently (research R4 #21)
- [X] T020 [US5] Verify delete/attrs/case/convert/size builder skips (`fileswn6.cpp:1536` `BuildScriptDir` `>=MAX_PATH-2`, and the `BuildName` leaf) now build long-path items after T003/T004; widen any remaining `>=MAX_PATH` guard in the BuildScript* family
- [X] T021 [US5] Static closure sweep: re-grep `src/` for unbounded `strcpy`/`memmove`/`memcpy`/`sprintf` of a full path (GetPath / sourcePath / targetPath / fileName) into a fixed buffer across the file-operation surface; confirm zero remaining CRASH/TRUNC verdicts; append a "Closure" section to research.md

## Phase 8: Polish & Verification

- [X] T022 Build Debug x64 + Release x64 (`build.cmd` / `build.cmd full release`); clang-format all touched files; rebuild clean
- [X] T023 Automated verification: `-a` startup navigation across the test tree survives (regression); static sweep zero; if the environment permits any scripted drive, exercise F3/copy; document what could and could not be driven (headless-env limits)
- [X] T024 Commit to branch and fast-forward `main`; update memory notes and feature 011 implementation-notes cross-reference

## Dependencies & Execution Order

- T001/T002 → all. **T003 (BuildName gate) is foundational** — it blocks every
  single-file operation; land it first.
- US1 (T004–T006) is the MVP (the copy crash). T004 before T006.
- US2 (T007–T011), US3 (T012–T014), US4 (T015–T018) are independent clusters,
  parallel after T003.
- US5 (T019–T021) after the fixes. Polish (T022–T024) last.

## Implementation Strategy

Single implementation pass grouped by cluster, one commit (one defect class:
"operations compose full paths into fixed buffers"), then build + verify.
MVP = T003 + US1 (files copy again, directory copy stops crashing). The
remaining clusters close view/edit, shell actions, and the panel-location
persistence so the long directory no longer "disappears".
