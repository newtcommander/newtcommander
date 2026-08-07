# Implementation Plan: Pre-release Review and Release of Version 0.1.2

**Branch**: `056-prerelease-review` | **Date**: 2026-08-07 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/056-prerelease-review/spec.md`

## Summary

Release gate for 0.1.2. The unreleased delta (`v0.1.1..HEAD`, 272 files) is
reviewed by a **multi-agent workflow** — six independent perspectives over the
compact C++/Python code delta (868 C++ insertions in 13 files + 4 tooling
files), every finding adversarially verified before it counts, confirmed
critical/high findings fixed and re-gated. In parallel, the full automated
gate suite runs: Debug + Release builds (all 8 languages), the `saltests`
unit suite (encoding helpers changed in 052), the SFTP behavioural harness
(7 scenarios + leak check, Docker reference server) and key-format fixtures,
translation round-trip, and an app-start smoke test in Czech. Only when the
gate holds are the version stamps applied (0.1.2 / build 186 in
`spl_vers.h`, `tandemcommander.iss`, `CLAUDE.md`) and the changelog released
as `## [0.1.2] — 2026-08-07`. The user's explicit instruction: **no
regression may ship** — every code change made from a finding triggers a
full gate re-run plus an independent re-review of the fix diff.

## Technical Context

**Language/Version**: review targets C++20/MSVC v143 (core + SFTP plugin),
Python 3.13+ (`tools/translate`), Windows batch/PowerShell (build); the
review itself is orchestrated with the Workflow tool (explicit user opt-in:
"Alokuj několik nezávislých agentů")
**Primary Dependencies**: git (`v0.1.1` tag = baseline), Docker Desktop +
`tandem-sftp` reference container (localhost:2222) for the SFTP harness —
**currently not running, must be started**; MSBuild/VS2022
**Storage**: release report + findings ledger persisted in
`specs/056-prerelease-review/` (`review-report.md`); version stamps in
`src/plugins/shared/spl_vers.h`, `setup/tandemcommander.iss`, `CLAUDE.md`,
`CHANGELOG.md`
**Testing**: `build.cmd full` (Debug) and `build.cmd full release`;
`src/saltests` console exe (exit code = failed checks);
`src/plugins/sftp/test/run_keyauth.cmd` (7 scenarios, CRT leak check) and
`test/build_and_run.cmd` (key-format fixtures); `tools/check_encoding.py`
(runs inside build); `python -m translate.slt --verify`; process-alive smoke
of `tandemcommander.exe` with Czech UI
**Target Platform**: Windows 11, x64
**Project Type**: release-engineering feature — review + gates + stamps; no
planned code changes except fixes for confirmed findings
**Performance Goals**: n/a
**Constraints**: version stamp only after all gates pass (FR-011); any fix
triggers full gate re-run (user's no-regression mandate); plugin interface
version (`LAST_VERSION_OF_SALAMANDER` = 106) is NOT bumped — no API change
in this cycle
**Scale/Scope**: code review surface: 13 C++ files (+868/−297), 4 Python
tooling files (+309/−31), SFTP `lang.rc2`; data surface: 153 translation
files already machine-verified in 055; 6 review perspectives, ~15 agents

## Constitution Check

| Principle | Verdict | Rationale |
|---|---|---|
| I. Build Reproducibility | PASS | No build-system change; gates rerun the standard scripts. Version bump is source-only. |
| II. Backward Compatibility | PASS | Release gate exists to protect the 0.1.0 baseline; no interface change; plugin API version untouched. |
| III. Incremental Modernization | PASS | Only targeted fixes for confirmed findings; each independently reviewable and re-gated. |
| IV. Windows Platform Commitment | PASS | No new dependencies; review-only. |
| V. Plugin Architecture Preservation | PASS | No plugin API change; SFTP plugin reviewed, not redesigned. |
| VI. UI Consistency | PASS | No new UI. |
| Release Documentation | PASS (core of the feature) | The constitution's release rule (MINORB + BUILDNUMBER + iss + CLAUDE.md line + changelog, one change) is FR-008/FR-009 verbatim. |

**Post-Phase-1 re-check**: no new projects/dependencies — all gates PASS.

## Project Structure

### Documentation (this feature)

```text
specs/056-prerelease-review/
├── plan.md                     # This file
├── research.md                 # Phase 0: baseline, gates inventory, risks
├── data-model.md               # Phase 1: finding/perspective/gate shapes
├── quickstart.md               # Phase 1: how to reproduce the gate run
├── contracts/
│   └── release-gate.md         # Gate conditions + review workflow protocol
├── review-report.md            # OUTPUT: findings ledger + gate results (FR-010)
└── tasks.md                    # Phase 2 (/speckit-tasks)
```

### Source Code (repository root)

```text
# Version stamps (US3, only after the gate holds):
src/plugins/shared/spl_vers.h   # MINORB 1→2, BUILDNUMBER 185→186 + comment row
setup/tandemcommander.iss       # MyAppVersion "0.1.1" → "0.1.2"
CLAUDE.md                       # "version 0.1.1 (internal build 185)" line
CHANGELOG.md                    # [Unreleased] → ## [0.1.2] — 2026-08-07 + summary

# Review surface (read; modified only on confirmed findings):
src/common/salunicode.{cpp,h}   src/dialogs5.cpp   src/fileswn7.cpp
src/plugins.h  src/plugins1.cpp  src/plugins2.cpp  src/saltests/saltests.cpp
src/plugins/sftp/{dialogs.cpp,session.cpp,sftp.cpp,sftp.h,lang/lang.rc2}
tools/translate/{match.py,merge.py,relayout.py,uicontext.py}
```

**Structure Decision**: no new source directories. The only planned writes
are the four version-bearing files, the changelog, the feature's own
documentation, and — exclusively for confirmed findings — targeted fixes in
the files above, each followed by the full gate re-run.

## Review Architecture (drives Phase 2 tasks)

Six perspectives, each an independent workflow agent reading the relevant
part of `git diff v0.1.1..HEAD` plus surrounding context:

| # | Perspective | Primary surface |
|---|---|---|
| P1 | Memory & resource safety | salunicode conversions, SFTP buffers/handles, dialog lifetimes |
| P2 | Concurrency & UI-thread discipline | SFTP worker thread, cancellable wait window, prompt handshake, overlapping connect |
| P3 | Network & protocol security | connect/retry flow, host-key trust path, socket lifetime, libssh2 call contracts, error classification |
| P4 | Credential & secret handling | password/passphrase storage, zeroization, logging, registry writes |
| P5 | Encoding correctness | UTF-8 contract sites (052), LoadStrU8/Sal*U8 usage, ANSI/UTF-8 mixing |
| P6 | Tooling & data integrity | match/merge/layout/relayout/uicontext changes, lang.rc2 delta, translation-data risk classes |

Protocol (contracts/release-gate.md): perspectives run in parallel →
findings in a structured schema → each finding verified by independent
skeptic agents (majority; ≥2 votes for critical/high) → triage → fixes →
fix diff re-reviewed by a fresh agent → **full gate suite re-run**. Refuted
and deferred findings stay in the ledger.

## Complexity Tracking

No constitution violations — table not needed.
