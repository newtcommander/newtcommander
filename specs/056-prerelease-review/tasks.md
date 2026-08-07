# Tasks: Pre-release Review and Release of Version 0.1.2

**Input**: Design documents from `/specs/056-prerelease-review/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/release-gate.md, quickstart.md

**Tests**: This feature IS the test: the release gate G1–G9
(contracts/release-gate.md) plus a multi-agent review. Any code fix made
here re-runs the complete gate suite (user's no-regression mandate).

**Organization**: grouped by user story. US1 = independent review,
US2 = green gates, US3 = version stamp & changelog. The review (US1) and the
long-running gates (US2) deliberately interleave in wall-clock time; the
stamp (US3) is strictly last.

## Format: `[ID] [P?] [Story] Description`

## Phase 1: Setup

- [X] T001 Delta inventory: `git diff --name-status v0.1.1..HEAD`,
      categorize every file into the P1–P6 perspective map (plan.md table) +
      docs/data buckets; write the coverage map into a skeleton
      `specs/056-prerelease-review/review-report.md` (SC-001 basis)
- [X] T002 [P] Bring up the SFTP reference server: start Docker Desktop,
      wait for the engine, `docker start tandem-sftp`, verify port 2222
      accepts a TCP connection (G4/G5 prerequisite; currently daemon is down)
- [X] T003 [P] Start gate G1 `build.cmd full` (Debug, background) — also
      produces the binaries T007/T008/T011 need

**Checkpoint**: delta mapped, server coming up, build baking.

---

## Phase 2: User Story 1 — Independent stability & security review (P1) 🎯

**Goal**: 6 independent perspectives over the whole code delta, findings
adversarially verified, confirmed blockers fixed with no regression.

**Independent Test**: review-report.md shows the perspective↔file map,
every finding with verdict + votes + resolution.

- [X] T004 [US1] Author and run the review Workflow per
      contracts/release-gate.md: 6 perspective agents (P1 memory, P2
      concurrency, P3 network security, P4 credentials, P5 encoding, P6
      tooling/data) with explicit file lists over `git diff v0.1.1..HEAD`,
      structured Finding schema, intake filter, adversarial verification
      (3 verifiers majority for critical/high, 1 for medium/low), raiser
      never verifies own finding
- [X] T005 [US1] Triage verified findings into the ledger in
      review-report.md: severity, introduced-since-0.1.1, resolution
      (fix-now / defer-with-reason / release-blocking); refuted findings
      recorded too (SC-006)
- [X] T006 [US1] Fix loop for confirmed critical/high + regressions (if
      any): minimal fix in the affected file(s) → fresh-agent re-review of
      the fix diff → **re-run full gate suite G1–G7** → update ledger;
      repeat until no open blocking findings (G8)

**Checkpoint**: findings ledger complete, no open critical/high.

---

## Phase 3: User Story 2 — Release is testably green (P1)

**Goal**: every gate from contracts/release-gate.md passes on the release
source state.

**Independent Test**: gate table in review-report.md, each row with its
run evidence.

- [X] T007 [US2] Gate G3: build the `saltests` project (Debug x64) and run
      the exe; exit code 0 (unit tests of `src/common/salunicode.cpp` /
      `salpath.cpp` — the 052 surface)
- [X] T008 [US2] Gate G4 (needs T002, T003):
      `src\plugins\sftp\test\run_keyauth.cmd` — all 7 scenarios pass
      including the CRT leak check
- [X] T009 [P] [US2] Gate G5: `src\plugins\sftp\test\build_and_run.cmd`
      key-format fixtures pass
- [X] T010 [P] [US2] Gate G6: `python -m translate.slt --verify`
      (PYTHONPATH=tools) byte-exact on all committed `.slt`
- [X] T011 [US2] Gate G7 (needs T003): smoke — set Czech UI language in
      `HKCU\Software\Tandem Commander\0.1`, launch Debug
      `tandemcommander.exe`, poll alive ≥10 s, terminate cleanly; repeat
      with English; restore prior language setting
- [X] T012 [US2] Gate G2: `build.cmd full release` (background) — 0 errors
- [X] T013 [US2] Consolidate the gate table in review-report.md: G1–G8 all
      green on the current source state (with timestamps); if any fix
      happened in T006 after a gate ran, that gate must show a re-run

**Checkpoint**: G1–G8 green — the stamp may proceed.

---

## Phase 4: User Story 3 — Version 0.1.2 stamped and documented (P2)

**Goal**: constitution's release rule executed in one change, verified by
sweep; nothing stale.

**Independent Test**: version sweep evidence in review-report.md.

- [X] T014 [US3] Stamp (needs T013): `src/plugins/shared/spl_vers.h` —
      `VERSINFO_SALAMANDER_MINORB` 1→2, `VERSINFO_BUILDNUMBER` 185→186,
      comment row `// 186 - Tandem Commander 0.1.2 (...)`;
      `setup/tandemcommander.iss` — `MyAppVersion` "0.1.2"; `CLAUDE.md` —
      identity line "version **0.1.2** (internal build 186)";
      `LAST_VERSION_OF_SALAMANDER` untouched (106)
- [X] T015 [US3] `CHANGELOG.md`: insert `## [0.1.2] — 2026-08-07` with a
      one-line summary ("Build 186. …") absorbing all current
      `[Unreleased]` entries; keep an empty `[Unreleased]` head
- [X] T016 [US3] Re-run G1 (`build.cmd full`) and G2
      (`build.cmd full release`) on the stamped source; verify the built
      Release `tandemcommander.exe` VERSIONINFO reads 0.1.2 / build 186 and
      a sample plugin `.spl` carries the same file version
- [X] T017 [US3] Gate G9 sweep: grep the four mandated files for the new
      stamp; `git grep -n "0\.1\.1"` / `"\b185\b"` over
      non-changelog/non-spec files shows no stale mandated stamp; record
      evidence in review-report.md

**Checkpoint**: 0.1.2 consistent everywhere; gates green on stamped source.

---

## Phase 5: Polish & release record

- [X] T018 Finalize `specs/056-prerelease-review/review-report.md`:
      coverage map, findings ledger (confirmed/refuted/deferred), final
      gate table, version-sweep evidence, deferral list for future planning
- [X] T019 Mark tasks complete and make the release commit ("[056] release
      0.1.2 …") containing stamps + changelog + report together (FR-008's
      same-change rule)

## Dependencies

```text
T001 ─┬─▶ T004 → T005 → T006 ─┐(loop re-runs G1–G7 on any fix)
T002 ─┼─▶ T008, T009           │
T003 ─┼─▶ T007, T008, T011     ├─▶ T013 ─▶ T014 → T015 → T016 → T017 → T018 → T019
      └─▶ T010, T012 (anytime) ┘
```

- T004 can run while T003's build bakes (review reads source, not binaries).
- T014–T017 strictly after T013 (FR-011); T016 re-validates builds on the
  stamped source, T017 completes G9.

## Parallel Execution Examples

- At start: T002 + T003 + T001 together; T004's workflow launches while G1
  builds and Docker boots.
- After T003: T007, T010, T012 in parallel with the review workflow;
  T008/T009 as soon as T002 confirms the server.

## Implementation Strategy

US1+US2 are co-equal P1 and interleave for wall-clock efficiency; the fix
loop (T006) is the only place code changes, and it always re-runs the whole
gate suite — the user's explicit no-regression requirement. US3 is
mechanical and gated. Everything lands in one release commit plus the
report.
