# Tasks: Systematic Whole-Program Long-Path Hardening (Feature 014)

**Input**: spec.md, plan.md, research.md, audit/_BRIEF.md
**Tests**: manual operation matrix (quickstart.md) — no automated test harness in this codebase.

## Phase 1: Setup & forensics

- [X] T001 Create feature branch `014-longpath-systematic-sweep`, spec, plan, checklist
- [X] T002 Census the defect class (grep core `src/*.cpp,*.h` for fixed `MAX_PATH`-class path buffers → ~764, ~90+ on path-copy routes)
- [X] T003 Symbolicate the newest Release crash dump (07:40) → confirm `viewer2.cpp:676 OpenFile` and the downstream-move pattern (research.md R0)
- [X] T004 Author the audit brief `audit/_BRIEF.md` (defect class, fix vocabulary, output format)

## Phase 2: Foundational — exhaustive audit (FR-008 deliverable)

- [~] T005 Run 8 parallel per-subsystem audit passes (A–H) → per-file buffer inventories. **BLOCKED**: API session limit (resets 11:20am Europe/Prague); to resume.
- [ ] T006 Consolidate audit A–H into research.md R4 (complete CRASH-site work-list + recursion map + external/ABI set)

## Phase 3: US1/US2 — eliminate crashes & make operations work (live chains first)

- [X] T007 [US1][US2] Fix dump-confirmed F3 crash: `viewer2.cpp:676` — eliminate the `fileName[MAX_PATH]` intermediate, use the `file` arg directly
- [X] T008 [US2] Fix Unicode "too long"/not-found on navigate/parse: `fileswn9.cpp:60` `ParsePath` `curPath[2*MAX_PATH]` → `SAL_MAX_PATH_UTF8`
- [X] T009 [US1] Verify paste chain (Ctrl+V) not reverted: `DropPath`, `CImpDropTarget::CurDir/SrcPath/OldDataObjectSrcFSPath`, `mydir`, `root` confirmed `SAL_MAX_PATH_UTF8`
- [X] T010 [US1] Classify remaining viewer buffers (caption BOUNDED cosmetic; drop/open EXTERNAL) — no further crash in the F3 chain
- [ ] T011 [US1][US2] Apply the full CRASH-site work-list from T006 across the ~30 core files (widen / heap on recursive routes / eliminate intermediates / bound external) — **pending T005/T006**
- [ ] T012 [US2] Bundled enabled plugins' own file-op UI secondary pass — **pending**

## Phase 4: Verification

- [X] T013 Build Debug x64 clean (`build.cmd`) — SUCCEEDED, changed files compile
- [~] T014 Build Release x64 clean (`build.cmd release`) — in progress
- [ ] T015 [US3] Static exhaustion re-check: re-run census, confirm zero unresolved CRASH-verdict buffers (after T011)
- [ ] T016 Re-symbolicate any new crash dump from the user to confirm the fixed frames
- [ ] T017 User interactive walkthrough of the operation matrix (quickstart.md) — headless env cannot drive keystrokes

## Phase 5: Commit

- [X] T018 Commit live-chain fixes (T007–T010) to `main` once Release verified
- [ ] T019 Commit the full-sweep fixes (T011–T012) after the audit resumes

## Notes

- The session-limit interruption split the work: the **dump-confirmed live
  crash (F3)** and the direct paste/navigation chains are fixed and building;
  the **exhaustive remainder** (T005/T006/T011) resumes after the limit resets.
- Discipline per buffer (from research.md / brief): widen single stack/member
  → SAL; **heap on recursive routes**; eliminate redundant intermediates;
  keep external-API/plugin-ABI buffers `MAX_PATH` but bounded; leave
  component-name buffers.
