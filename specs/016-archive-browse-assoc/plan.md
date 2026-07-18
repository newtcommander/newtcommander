# Implementation Plan: Archive Associations Self-Heal (Feature 016)

**Branch**: `016-archive-browse-assoc` | **Date**: 2026-07-18 | **Spec**: [spec.md](spec.md)

## Summary

Enter on ZIP (and every plugin-handled archive) fell through to Explorer
because plugin archive associations were destroyed by the feature-010 pre-105
packers reset + `CheckData` legacy-entry cull, and the install-only creation
path never re-adds them (research.md R2). Fix: a standing **self-heal in
`CPlugins::CheckData()`** — after table validation, any registered plugin with
panel-view support and declared extensions that has no association entry gets
one for its unclaimed extensions (R3).

## Technical Context

**Language/Version**: C++20, MSVC v143 | **Dependencies**: none new
**Files**: `src/plugins2.cpp` (CheckData) only
**Testing**: Debug + Release builds; controlled run of the fixed Release binary
against the user's current broken config with registry inspection before/after
(launch + graceful close); double-restart stability check; user's Enter test
**Constraints**: no plugin-ABI change; user-remapped extensions untouched;
idempotent across restarts; safe with zero plugins loaded

## Constitution Check

| # | Principle | Verdict |
|---|-----------|---------|
| I | Build Reproducibility | PASS — code-only |
| II | Backward Compatibility | PASS — restores historical behavior (archives browse in panel); external-archiver rows untouched |
| III | Incremental Modernization | PASS — small reconcile step in the function already responsible for table consistency |
| IV | Windows Platform Commitment | PASS |
| V | Plugin Architecture Preservation | PASS — uses existing registration data (`Extensions`, `SupportPanelView/Edit`), no interface change |
| VI | UI Consistency | PASS — no UI change |

## Project Structure

```text
specs/016-archive-browse-assoc/  spec.md, plan.md, research.md, tasks.md, checklists/
src/plugins2.cpp                 CheckData(): self-heal block after format validation
```

## Verification plan

1. Build Debug + Release clean.
2. Snapshot `Archive Association` (6 rows, no zip) → run fixed Release →
   graceful close → re-dump: rows for zip;pk3;pk4;jar, 7z, tar…, iso…, cab
   present with plugin references; external rows unchanged.
3. Second run + close → table byte-stable (idempotence).
4. User: Enter on `unicode-test.zip` → browses in panel.
