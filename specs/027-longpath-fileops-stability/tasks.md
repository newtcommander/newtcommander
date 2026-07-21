# Tasks: Long-Path & Unicode File-Operation Stability Revision

**Input**: Design documents from `/specs/027-longpath-fileops-stability/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/clipboard-dataobject.md

**Tests**: saltests additions are requested by the plan (R9) — test tasks included.

**Organization**: By user story. Both P1 stories are independent; US2 (crash
fixes) goes first — smallest change, highest severity, unblocks the user's
Release testing of everything else.

## Phase 1: Setup

- [X] T001 Verify/extend the long-path test tree at `%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\` (Unicode L1/L2/L3 present; add `edge-259`/`edge-261` boundary dirs and a recursive folder with long Unicode inner names for matrix tests); record layout in `specs/027-longpath-fileops-stability/audit/testdata.md`
- [X] T002 Baseline: Debug x64 build (`build.cmd`) green + locate and run the existing `saltests` suite (`src/` — find project, record invocation in `specs/027-longpath-fileops-stability/quickstart.md`); confirm all tests pass before any change

## Phase 2: Foundational

*(No shared blocking infrastructure — the fronts are independent point fixes on the established `SAL_MAX_PATH_UTF8` pattern. Phase intentionally empty.)*

**Checkpoint**: Baseline green — user stories can start, in parallel where marked.

---

## Phase 3: User Story 2 — No operation ever crashes (Priority: P1) 🎯 MVP

**Goal**: Eliminate both dump-confirmed Release crashes (Alt+F1 Change Drive, F4 external editor) — research R1.

**Independent Test**: In the L3 Unicode dir (~570-byte path): Alt+F1 opens the drive menu; F4 launches the editor or shows a bounded message; process alive in both. (Statically: no fixed `MAX_PATH` sink reachable from these two chains.)

- [X] T003 [P] [US2] Widen `CDrivesList::CurrentPath` to `SAL_MAX_PATH_UTF8` in `src/drivelst.h` (:110) and replace `lstrcpy` with a bounded copy in `src/drivelst.cpp` (:1069); audit every other writer/reader of `CurrentPath` in `src/drivelst.cpp` for size assumptions
- [X] T004 [P] [US2] Widen `CExecuteExpData::Buffer` to `SAL_MAX_PATH_UTF8` in `src/execute.cpp` (:580) and bound ALL callback writers (`ExecuteExpPath` :607, `ExecuteExpFullPath2` :794/:878, and every sibling `Execute*` callback writing `Buffer`) with `_snprintf_s(_TRUNCATE)`/explicit length checks
- [X] T005 [US2] Build Debug x64; verify no new warnings in `drivelst.cpp`/`execute.cpp`; run saltests

**Checkpoint**: Both crash chains dead — MVP deliverable.

---

## Phase 4: User Story 1 — Clipboard copy & paste with long Unicode paths (Priority: P1)

**Goal**: Ctrl+C/Ctrl+X out of long dirs places valid data; Ctrl+V into long dirs (incl. foreign/Explorer data) runs the long-path engine — research R2/R3/R5.

**Independent Test**: Clipboard matrix — {copy, cut} × {long-Unicode source, ordinary source} × {ordinary, long-Unicode target}: byte-identical results, exact names, process alive; sub-260 combinations take the legacy code paths (verified by tracing the gate).

- [X] T006 [US1] Implement the own long-path IDataObject in `src/shellib.cpp` + declaration in `src/shellib.h` per `contracts/clipboard-dataobject.md`: wide `DROPFILES` (fWide=1, double-NUL wide full-path list, heap via `SalU8ToWAlloc`), `CFSTR_PREFERREDDROPEFFECT` DWORD, `EnumFormatEtc`, `TYMED_HGLOBAL` `GetData`
- [X] T007 [US1] Length-gated copy-out selection in `src/shellsup.cpp` `ShellAction` saCopyToClipboard/saCutToClipboard (~:1676-1711): if panel path + any selected full name ≥ `MAX_PATH` → `OleSetClipboard` (own object) + existing `SetClipCutCopyInfo` marker; else legacy shell-verb route unchanged
- [X] T008 [US1] Length-gated paste takeover in `src/fileswn9.cpp` `ClipboardPaste` route C (~:297-319): when CF_HDROP holds disk paths AND (target dir ≥ `MAX_PATH` or any source ≥ `MAX_PATH`) → parse via existing `ProcessClipboardData` machinery and run `DoCopyMove` honoring `CFSTR_PREFERREDDROPEFFECT`; `CFSTR_FILECONTENTS`-only and `CM_CLIPPASTELINKS` stay shell; sub-260 unchanged; keep menu-enable test (`onlyTest`) consistent (`src/mainwnd1.cpp:2832`)
- [X] T009 [P] [US1] Heap-based common-prefix compare in `IsSimpleSelection` in `src/shellib.cpp` (:445 `prefixBuf[MAX_PATH]`, compare sites :502-503/:611-612) so long selections classify correctly
- [X] T010 [P] [US1] Explicit refusal gates (clear bounded message, no truncation) at both `SalShExtSharedMem::TargetPath` write sites in `src/shellib.cpp` (:1238, :1316); ABI struct `src/shexreg.h:218` untouched; record on external-limit list
- [X] T011 [P] [US1] saltests: DROPFILES wide-block build/parse round-trip with >260-char Czech-diacritics paths + preferred-drop-effect encode/decode (new test file alongside existing saltests conventions, `src/saltests/`)
- [X] T012 [US1] Build Debug x64 + run saltests; trace-verify the sub-260 gate keeps legacy routes (code inspection note in `specs/027-longpath-fileops-stability/audit/G.md` addendum)

**Checkpoint**: US1 + US2 both independently functional.

---

## Phase 5: User Story 3 — F5/F6 works in ALL cases (Priority: P2)

**Goal**: Close the remaining in-program engine gaps — research R4 + drag gate R5.

**Independent Test**: F5/F6 matrix {file, recursive folder} × {long→normal, normal→long, long→long}: folder operations no longer rejected with "too long"; new intermediate target dirs created with exact Unicode names; drag onto a subdirectory item composes targets ≥260.

- [X] T013 [P] [US3] Raise `BuildScriptDir` source gate to `SAL_MAX_PATH_UTF8` in `src/fileswn6.cpp` (:1587, stale `MAX_PATH - 2` reject; update stale comment)
- [X] T014 [P] [US3] Raise `BuildScriptDir` target gate to `SAL_MAX_PATH_UTF8` in `src/fileswn6.cpp` (:1655, `PATH_MAX_PATH` reject; constant itself untouched — plugin ABI)
- [X] T015 [P] [US3] Replace ANSI `CreateDirectory` with `SalCreateDirectory` in `src/salamdr5.cpp` (:968, intermediate target dirs); switch `SalGetFileSize2` ANSI `CreateFile` (:1446) to `SalCreateFile` after checking callers
- [X] T016 [P] [US3] F7 Create Directory long-path: widen dialog/aux buffers and `PATH_MAX_PATH` gate in `src/fileswn5.cpp` (:1963, :1986/:1988 `IDS_TOOLONGPATH` gate, `checkPath` :2003, `newDir` :2009) to `SAL_MAX_PATH_UTF8`
- [X] T017 [P] [US3] Copy-security route to wide APIs in `src/worker.cpp`: `GetNamedSecurityInfoA`/`SetNamedSecurityInfoA` → `W` via `SalPathToWExtAlloc` (:1505-1620 `DoCopySecurity`, :5644 `DoMoveFile`)
- [X] T018 [P] [US3] Encrypt/Decrypt route to wide APIs in `src/worker.cpp`: `EncryptFileA`/`DecryptFileA` → `W` (:1793/:1812/:3149, :1852/:1871/:3158); verify `\\?\` acceptance at runtime, degrade with clear bounded message if the API refuses
- [X] T019 [P] [US3] Widen `DoConvert` temp-name buffer `tmpFileName[MAX_PATH]` in `src/worker.cpp` (:7295)
- [X] T020 [P] [US3] Drag-onto-subdirectory target compose on heap + `SAL_MAX_PATH_UTF8` gate in `src/shellsup.cpp` `GetCurrentDir` (:477 `l + NameLen >= MAX_PATH` reject; archive variant :310 stays bounded)
- [X] T021 [US3] Build Debug x64 + saltests; add saltests case for gate arithmetic (source/target length thresholds) if expressible against linkable units

**Checkpoint**: All three functional stories complete.

---

## Phase 6: User Story 4 — Long-path operations are fast (Priority: P3)

**Goal**: Remove the avoidable per-call canonicalization cost; prove the ≤10% bar — research R6.

**Independent Test**: Timing harness — fixed file set copied ordinary→ordinary vs long→long on the same volume; long ≤ 1.10 × ordinary; sub-260 no slower than baseline.

- [ ] T022 [US4] Clean-path pre-scan in `src/common/salpath.cpp` `SalPathToWExtAlloc` (skip `SalCanonicalizePathW` when the UTF-8 input has no `/`, no `.` path segment, no doubled separator past the root)
- [ ] T023 [P] [US4] saltests: pre-scan correctness cases (clean absolute, `.`/`..` segments, doubled separators, UNC, trailing dot/space names → canonicalization still applied where required; output identical to always-canonicalize) in `src/saltests/`
- [ ] T024 [US4] Timing harness script `specs/027-longpath-fileops-stability/audit/perf.ps1`: create 500×4 KB + 5×50 MB set, copy via `Sal*`-wrapper test binary (saltests bench) or file-system fallback in ordinary vs long-Unicode dirs; record before/after numbers in `specs/027-longpath-fileops-stability/audit/perf-results.md`

**Checkpoint**: SC-005 evidence recorded.

---

## Phase 7: User Story 5 — Provably complete revision (Priority: P3)

**Goal**: Execute feature 014's never-run exhaustive audit to completion; fix every CRASH verdict — research R7.

**Independent Test**: `audit/INVENTORY.md` covers the enumerated population with verdicts; static check reports zero unresolved CRASH sites.

- [ ] T025 [US5] Run 8 parallel subsystem audit passes (A viewer, B engine/workers, C file-op UI, D navigation/notify/dirline, E core primitives, F mainwnd/persistence/history, G shell/clipboard/drag, H dialogs/find/drive/plugin-handoff) per `specs/014-longpath-systematic-sweep/audit/_BRIEF.md` methodology → `specs/027-longpath-fileops-stability/audit/A.md` … `H.md`
- [ ] T026 [US5] Consolidate `specs/027-longpath-fileops-stability/audit/INVENTORY.md`: one verdict row per site (schema in data-model.md §1) + §External list (R8) + §Verification
- [ ] T027 [US5] Fix ALL sites with CRASH verdict found by the audit (files as discovered; follow `_BRIEF.md` fix vocabulary — widen / heap on recursive routes / eliminate intermediate); update INVENTORY rows with resolution commits
- [ ] T028 [US5] Static exhaustion check script `specs/027-longpath-fileops-stability/audit/check.ps1` (re-scans the inventoried sites, fails on unclassified/unresolved CRASH) + run to green

**Checkpoint**: The class is provably closed for the core app.

---

## Phase 8: Polish & Final Verification

- [ ] T029 Format changed files per repo config (clang-format / `normalize.ps1`); UTF-8-BOM preserved
- [ ] T030 Full Debug x64 build clean (`build.cmd`) — zero new warnings in changed files
- [ ] T031 Full Release x64 build clean (`build.cmd release`; close any running Release `salamand.exe` first — LNK1104 pitfall)
- [ ] T032 Run complete saltests suite — green
- [ ] T033 Re-validate spec checklists; update `specs/027-longpath-fileops-stability/quickstart.md` walkthrough with any changes; final commit(s) `[027]` prefix, author Pavel Stupka

---

## Dependencies & Execution Order

- **Phase 1 → all**: baseline must be green first.
- **US2 (Phase 3)**: independent — MVP, do first.
- **US1 (Phase 4)**: T006 → T007/T008 (builder before wiring); T009/T010/T011 parallel to T006-T008.
- **US3 (Phase 5)**: T013-T020 all [P] (different files/sites); T021 gates.
- **US4 (Phase 6)**: independent of US1-US3; T022 → T023/T024.
- **US5 (Phase 7)**: audit passes AFTER US1-US4 fixes land (so verdicts reflect final code); T025 → T026 → T027 → T028.
- **Phase 8**: last.

## Implementation Strategy

MVP = Phase 3 (crash fixes). Then US1 (the user's headline), US3, US4, then
the audit sweep proves closure over the final code. Commit after each story
checkpoint with `[027]` prefix. The interactive GUI walkthrough
(quickstart.md) remains the user's follow-up after final build.
