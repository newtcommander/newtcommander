# Contract: Release gate and review workflow protocol (0.1.2)

**Feature**: 056-prerelease-review

## Review workflow protocol

1. **Fan-out**: 6 perspective agents run in parallel; each receives its
   assigned file list (plan.md table), reads `git diff v0.1.1..HEAD` for
   those files plus enough surrounding source to judge context, and returns
   findings in the Finding schema (data-model.md) — or an explicit
   "read X files, no findings" statement.
2. **Intake filter**: findings without a concrete failure scenario are
   returned as refuted("unfalsifiable") — they never reach verification.
3. **Adversarial verification**: each finding goes to independent skeptic
   agents prompted to refute it against the actual source (not the diff
   alone). Critical/high: 3 verifiers, majority decides, ties → confirmed
   (fail-safe). Medium/low: 1 verifier. The raiser never verifies its own
   finding.
4. **Triage**: confirmed findings get severity + introduced-since-0.1.1
   classification. Critical/high (any origin) and regressions → fix now.
   Medium/low pre-existing → defer with reason.
5. **Fix loop** (main loop, not agents): smallest correct fix → fresh-agent
   re-review of the fix diff → **full gate suite re-run**. Loop until no
   open blocking findings.
6. **Ledger**: every finding — confirmed, refuted, deferred — lands in
   review-report.md with its votes.

## Gate conditions (all must hold before the version stamp)

```text
G1 debug-build      build.cmd full                 → 0 errors, 180 lang modules
G2 release-build    build.cmd full release         → 0 errors
G3 saltests         saltests exe                   → exit 0
G4 sftp-harness     run_keyauth.cmd                → 7/7 + leak check (needs tandem-sftp container)
G5 sftp-fixtures    build_and_run.cmd              → pass
G6 slt-roundtrip    python -m translate.slt --verify → byte-exact
G7 smoke            launch exe (czech, english)    → alive ≥10 s, clean termination
G8 findings         ledger                         → no open critical/high, none release-blocking
G9 version-sweep    (after stamping)               → 0.1.2 + build 186 in spl_vers.h, iss,
                                                     CLAUDE.md, CHANGELOG (dated 2026-08-07);
                                                     no stale 0.1.1/185 in mandated spots;
                                                     Release exe/installer metadata reads 0.1.2
```

Rules:
- Any source change (fix or stamp) invalidates G1–G7 results → re-run.
  Exception: the changelog/CLAUDE.md text edits invalidate only G1/G2 if a
  resource embeds them (they do not — G9 covers the stamp files; but the
  spl_vers.h/iss stamp DOES invalidate G1–G2, which must re-run on the
  stamped source so the shipped artifacts carry 0.1.2).
- G4 blocked (no Docker) ⇒ release halts; no silent skip (spec edge case).
- Stamp order: G1–G8 green → stamp → re-run G1, G2 (+G3 smoke-level:
  saltests binary from the stamped build) → G9 sweep → release commit.

## Workflow-usage authorization

The user's instruction "Alokuj několik nezávislých agentů…" is the explicit
opt-in for multi-agent orchestration (Workflow tool) in this feature's
implementation.
