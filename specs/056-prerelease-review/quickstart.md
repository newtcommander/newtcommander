# Quickstart: Reproducing the 0.1.2 release gate

**Feature**: 056-prerelease-review · Commands run from the repository root.

## 1. Baseline sanity

```bat
git fetch --tags & git log --oneline v0.1.1..HEAD   :: the reviewed delta
```

## 2. Test gates (offline unless noted)

```bat
build.cmd full                         :: G1 — Debug + 180 language modules
build.cmd full release                 :: G2 — Release artifacts
:: G3 — unit tests (encoding helpers): build + run the saltests project exe
:: G4/G5 — SFTP (needs Docker Desktop + container tandem-sftp on :2222):
docker start tandem-sftp
src\plugins\sftp\test\run_keyauth.cmd
src\plugins\sftp\test\build_and_run.cmd
python -m translate.slt --verify       :: G6 (PYTHONPATH=tools)
:: G7 — smoke: launch Debug tandemcommander.exe with czech UI, poll ~10 s
```

Expected: G1/G2 zero errors; G3 exit 0; G4 7/7 scenarios + leak check;
G5 pass; G6 byte-exact; G7 process stays alive and terminates cleanly.

## 3. Review (multi-agent)

Run per `contracts/release-gate.md`: 6 perspectives → adversarial
verification → triage. Output: findings ledger in `review-report.md`.
Any fix ⇒ re-run section 2 completely.

## 4. Stamp + sweep (only after G1–G8 green)

- `spl_vers.h`: MINORB 2, BUILDNUMBER 186, comment row for 186
- `tandemcommander.iss`: MyAppVersion 0.1.2
- `CLAUDE.md` identity line; `CHANGELOG.md` → `## [0.1.2] — 2026-08-07`
- re-run G1+G2 on stamped source; verify exe/installer metadata reads 0.1.2
- sweep: no stale `0.1.1`/`185` in mandated locations

## 5. Done when

`review-report.md` shows: perspective↔file coverage map, every finding with
verdict+resolution, the final green gate table on the release source state,
and the version-sweep evidence.
