# Data Model: Pre-release Review and Release of Version 0.1.2

**Feature**: 056-prerelease-review · **Date**: 2026-08-07

## Finding

One claimed defect or risk raised by a review perspective.

| Field | Values | Notes |
|---|---|---|
| id | F-### | stable across the report |
| perspective | P1…P6 | who raised it |
| location | file:line (repo-relative) | anchored to the delta or its blast radius |
| claim | one sentence | what is wrong |
| failure_scenario | concrete input/state → wrong outcome | required; unfalsifiable claims are rejected at intake |
| severity | critical / high / medium / low | stability or security impact of the *shipped* product |
| introduced | since-0.1.1 / pre-existing | pre-existing blocks only at critical (spec edge case) |
| verdict | confirmed / refuted | set only by verification agents, never by the raiser (SC-002) |
| votes | e.g. 2/3 refute | dissent stays visible |
| resolution | fixed / deferred(reason) / release-blocking / refuted | every finding ends in exactly one |
| fix_commit | hash or — | fixes reference their commit; fix diff re-reviewed |

**State transitions**:
`raised → verified(confirmed|refuted)`; `confirmed → fixed | deferred | release-blocking`;
`release-blocking → fixed` is the only exit that unblocks the stamp (FR-011).

## Review perspective

| Field | Notes |
|---|---|
| id + focus | P1 memory/resources, P2 concurrency/UI-thread, P3 network/protocol security, P4 credentials/secrets, P5 encoding, P6 tooling & data |
| assigned surface | explicit file list from the delta (plan.md table); union of surfaces = 100% of code delta (SC-001) |
| output | findings list in the schema above; an explicit "areas read, nothing found" statement when clean (edge case: clean ≠ unread) |

## Gate

| Gate | Source of truth | Pass condition |
|---|---|---|
| debug-build | `build.cmd full` output | 0 errors, 180 language modules |
| release-build | `build.cmd full release` output | 0 errors |
| saltests | test exe exit code | 0 |
| sftp-harness | `run_keyauth.cmd` output | 7/7 scenarios + leak check pass |
| sftp-fixtures | `build_and_run.cmd` output | pass |
| slt-roundtrip | `translate.slt --verify` | byte-exact, all files |
| smoke | process poll | alive ≥10 s in czech + english, clean exit |
| findings | ledger | no open critical/high, no release-blocking |
| version-sweep | grep + built artifact metadata | 0.1.2/186 everywhere mandated, no stale stamp |

The **release gate** = conjunction of all rows. Any code change resets all
test-gate rows to "must re-run".

## Release report (`review-report.md`)

Sections: delta inventory (files ↔ perspectives map, SC-001) · findings
ledger (all verdicts incl. refuted, SC-006) · gate results table with the
final green run timestamped on the release source state (SC-004) · version
sweep evidence (SC-005) · deferrals for future planning.

## Version-bearing locations (FR-008)

`src/plugins/shared/spl_vers.h` (MINORB, BUILDNUMBER, comment table) ·
`setup/tandemcommander.iss` (MyAppVersion) · `CLAUDE.md` (identity line) ·
`CHANGELOG.md` (release section). Explicitly NOT: `LAST_VERSION_OF_SALAMANDER`.
