# Research: Pre-release Review and Release of Version 0.1.2

**Feature**: 056-prerelease-review · **Date**: 2026-08-07

## R1. Baseline and delta

**Decision**: baseline = git tag `v0.1.1` (release of 2026-08-05, build 185).
Delta `v0.1.1..HEAD`: 272 files. Code surface: 13 C++ files
(+868/−297 — 052 encoding contract in core, 053/054 SFTP plugin), 4 Python
tooling files (055), SFTP `lang.rc2`; data surface: 153 translation
`.slt`/`.origin` files (055) already machine-verified (SC-002: 59,360
entries, 0 violations); rest is specs/docs/skills housekeeping.

**Rationale**: the tag exists and `git log v0.1.1..HEAD` confirms the
feature set (052, 053, 054, 055 + housekeeping). The compact code delta
makes full (not sampled) code review feasible.

## R2. Gate inventory (what "testably green" means here)

| Gate | Command | Expected |
|---|---|---|
| Debug build + languages | `build.cmd full` | 0 errors, 180 language modules, encoding guard passes (built-in) |
| Release build | `build.cmd full release` | 0 errors — the artifact 0.1.2 ships from |
| Unit tests (encoding helpers) | build + run `saltests` project exe | exit code 0 (= number of failed checks) |
| SFTP behavioural harness | `src\plugins\sftp\test\run_keyauth.cmd` | all 7 scenarios pass incl. CRT leak check |
| SFTP key-format fixtures | `src\plugins\sftp\test\build_and_run.cmd` | pass |
| Translation round-trip | `python -m translate.slt --verify` | byte-exact, 290 files |
| App smoke (Czech UI) | launch `tandemcommander.exe`, poll alive, close | process runs, no crash; manual-free |

**Docker**: the harness needs container `tandem-sftp` (localhost:2222); the
Docker daemon is **currently not running**. Plan: start Docker Desktop,
wait for the engine, `docker start tandem-sftp` (container exists from
051/053/054 work). If the engine cannot be brought up, the gate is blocked
and the release halts per spec edge case — no silent skip.

**saltests**: console exe, exit code = failed checks; covers
`salunicode.cpp`/`salpath.cpp` — precisely the helpers feature 052 extended
(+46 lines) — so it doubles as the regression net for any P5 finding fix.

## R3. Multi-agent orchestration vehicle

**Decision**: the Workflow tool, single run with phases: parallel
perspective review (6 agents, structured findings schema) → adversarial
verification (independent skeptics per finding; ≥2 concurring votes to
confirm critical/high) → synthesis. Fixes, if any, are made in the main
loop (not by agents in worktrees) so each fix is followed deterministically
by the full gate re-run; the fix diff is then re-reviewed by one fresh agent.

**Rationale**: the user explicitly requested independent agents; Workflow
gives deterministic fan-out/verification and a machine-readable findings
ledger. Fixing in the main loop keeps the no-regression mandate enforceable
(one place re-runs all gates after every change).

**Alternatives considered**: per-agent worktree fixes — parallel but each
worktree would need its own gate run (build ~7 min each) and merge risk;
rejected for a delta this small.

## R4. Version stamp mechanics (constitution release rule)

**Decision**: one change containing:
- `src/plugins/shared/spl_vers.h`: `VERSINFO_SALAMANDER_MINORB 1` → `2`;
  `VERSINFO_BUILDNUMBER 185` → `186`; new comment row
  `// 186 - Tandem Commander 0.1.2 (...)` under the build-number table
  (mandated by the header's own instructions).
- `setup/tandemcommander.iss`: `MyAppVersion "0.1.1"` → `"0.1.2"`.
- `CLAUDE.md`: product-identity line → version 0.1.2 (internal build 186).
- `CHANGELOG.md`: `## [0.1.2] — 2026-08-07` with a one-line summary
  ("Build 186. …") absorbing the `[Unreleased]` entries; an empty
  `[Unreleased]` head is kept for the next cycle.
- `LAST_VERSION_OF_SALAMANDER` stays 106 — no plugin API change in the delta
  (verified: no `spl_*.h` interface files in the diff).

**Verification**: a sweep greps for stale `0\.1\.1` / `185` in the mandated
locations plus `git grep -n "0\.1\.1"` over non-changelog, non-spec files to
catch unmandated stragglers; the built exe/installer metadata is checked
after the Release build.

## R5. Translation-data risk classes (P6 scope)

Already machine-verified in 055 (placeholder/accelerator validation at merge
time; provenance-scoped diff; 180 modules import positionally). Residual
risks for the reviewer: printf-style placeholders in *dialog* text that the
C++ formats at runtime (crash class), `\t` shortcut labels, control-geometry
sanity after widening, and the 20 hand-pinned overrides. P6 re-checks these
classes directly on the current tree rather than re-litigating 055's diff.

## R6. Smoke test shape

**Decision**: process-level smoke only — set the Czech language in the
per-user configuration (registry `HKCU\Software\Tandem Commander\0.1`),
launch the Debug build, poll the process stays alive ~10 s, then terminate
it; repeat once with English. GUI interaction automation is out of scope —
the behavioural coverage lives in the SFTP harness and 052/053/054 recorded
verifications.

**Rationale**: catches missing-resource startup failures and language-load
crashes (the realistic smoke regressions for this delta) without brittle UI
scripting.

## R7. No-regression enforcement (user mandate)

**Decision**: after ANY code change made in this feature (finding fix or
version stamp), the full gate suite re-runs from scratch; a code fix
additionally gets (a) a targeted test exercising the fixed path where the
existing suites provide one (saltests / harness scenario), and (b) an
independent re-review of the fix diff. Gate results per run are appended to
review-report.md, so the report shows the final green run on the exact
release source state.
