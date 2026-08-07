# Tasks: Contextual Re-translation of Machine-Translated UI Strings

**Input**: Design documents from `/specs/055-contextual-retranslation/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/redo-machine-cli.md, quickstart.md

**Tests**: The spec's success criteria are verified by dedicated validation
tasks (offline dry-run contract checks, a provenance-scoped diff check, the
language build gate, and a human spot-check) rather than a unit-test suite —
the tooling package has none and the deliverable is committed data.

**Organization**: Tasks are grouped by user story. Python commands assume
`PYTHONPATH=tools` from the repository root (the package is not installed).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = correct non-English UI, US2 = human translations
  untouched, US3 = safe/repeatable operation

## Phase 1: Setup (baseline & templates)

**Purpose**: Establish the current-structure templates and the pre-change
baseline every later check compares against.

- [X] T001 Export current-structure templates from the up-to-date Release
      tree: `src\vcxproj\build_langs.cmd --export-templates release`; verify
      `build\tandemcommander\translator\templates\<module>.slt` exists for all
      20 enabled modules
- [X] T002 [P] Baseline round-trip check: `python -m translate.slt --verify`
      passes for every committed `translations/<lang>/<module>.slt`
- [X] T003 Baseline dry-run (needs T001): `python -m translate.merge --dry-run`
      reports **0 gaps** for all 8 enabled languages (committed `.slt` in sync
      with current resources) and record the baseline coverage table
      (human/machine/skip counts per language) for comparison in T012

**Checkpoint**: templates exported, tree verified in sync, baseline recorded.

---

## Phase 2: Foundational (tooling delta)

**Purpose**: The merge-mode and context changes every story depends on.
Contract: `contracts/redo-machine-cli.md`.

- [X] T004 Add `--redo-machine` to `tools/translate/merge.py`: generalize the
      `load_origin()` demotion (all `machine` → `english_fallback` before
      matching, superset of `--redo-accelerators`), thread the flag through
      `collect_gaps()`, `run()`, and `main()`; help text documents that
      `--retranslate` dominates when combined
- [X] T005 Add repeatable `--exclude-module NAME` to
      `tools/translate/merge.py`: applied after enabled-module resolution;
      unknown name → stderr error + exit 1; empty resulting module set →
      error + exit 1
- [X] T006 [P] Extend `_DOMAINS` in `tools/translate/uicontext.py` with
      one-clause descriptions for all enabled modules currently missing one
      (7zip, dbviewer, diskmap, filecomp, folders, peviewer, portables,
      regedt, renamer, tar, uncab, undelete, uniso) per research.md R3
- [X] T007 Extend `_WORDS` in `tools/translate/uicontext.py` with common
      vocabulary of the newly covered modules (archive, extract, compress,
      registry, rename, compare, mask, drive, volume, device, preview, …) so
      `humanize_symbol()` splits their symbols readably
- [X] T008 Offline contract validation (needs T004–T007):
      `python -m translate.merge --dry-run --redo-machine --exclude-module sftp`
      → per-language gap counts equal the `.origin` machine counts
      (czech 292, dutch 559, french 403, german 366, hungarian 324,
      romanian 650, slovak 300, spanish 363; total 3,257), `sftp` absent from
      output, exit 0, working tree unmodified (`git status` clean)

**Checkpoint**: flags behave per contract, scope numbers confirmed — quota
spend is now safe to plan.

---

## Phase 3: User Story 1 — Non-English UI reads correctly (Priority: P1) 🎯 MVP

**Goal**: Every machine-provenance entry outside SFTP re-translated with
context; product rebuilt and spot-checked.

**Independent Test**: quickstart.md steps 3–4, 6–7 — dry-run scope, the run,
the language build, and the Czech/Slovak spot-check.

- [X] T009 [US1] Pre-flight (needs T008): review the T008 dry-run character
      estimate (expect ≈80–120k, abort threshold 350k) and confirm
      `temp\deepl_key.txt` present
- [X] T010 [US1] Execute the re-translation:
      `python -m translate.merge --all --redo-machine --exclude-module sftp`;
      capture full console output; summarize coverage table, validation
      failures, and quota spend into
      `specs/055-contextual-retranslation/run-notes.md`
- [X] T011 [US1] Post-run scope check: `git status` shows modified files only
      under `translations/<8 enabled languages>/` with extensions
      `.slt`/`.origin`, never `sftp.*`, never russian/ukrainian/
      chinesesimplified, never `ui-overrides.json`; per-file section/row
      counts unchanged (structure intact)
- [X] T012 [US1] Report checks against the T003 baseline: per-language human
      counts unchanged, machine counts ≈ baseline (minus entries that became
      overrides/fallbacks), new fallbacks < 2% per language (SC-005), quota
      spend within one free-tier month (SC-006); record in run-notes.md
- [X] T013 [US1] Language build gate (SC-004): run `build.cmd full` (Debug,
      absolute path, background) — build succeeds, `build_langs` imports all
      8 languages with zero positional-import errors, `.slg` produced for
      8 languages × 20 modules
- [X] T014 [US1] Spot-check (SC-003): sample ≥20 re-translated entries each
      for Czech and Slovak from `git diff` (prioritize one-word labels,
      buttons, column headings); require ≥90% correct-for-location and zero
      wrong-sense survivals among checked entries; record the sample table in
      run-notes.md

**Checkpoint**: refreshed translations exist, build ships them, quality
sampled — US1 deliverable complete pending US2's safety gate before commit.

---

## Phase 4: User Story 2 — Human translations never touched (Priority: P1)

**Goal**: Machine-checked proof that the run changed only machine-provenance
entries.

**Independent Test**: quickstart.md step 5 — provenance-scoped diff of HEAD
vs. working tree.

- [X] T015 [US2] Write the SC-002 verification script in the scratchpad
      (not committed): for each (enabled language, enabled module ≠ sftp)
      load the `HEAD` `.slt`/`.origin` (via `git show`) and the working-tree
      `.slt` with `translate.slt`, key rows by `match.entry_key`, assert
      (a) every entry with HEAD provenance `human`/`skip` has identical text,
      (b) every text-changed entry had HEAD provenance `machine`
- [X] T016 [US2] Run the verification (needs T010, T015): 0 violations across
      8 languages × 19 modules; re-run `python -m translate.slt --verify`
      (round-trip still byte-exact); record counts in run-notes.md
- [X] T017 [P] [US2] Pinned-override check: the ZIP plugin name is still the
      literal "ZIP" in every enabled language's `zip.slt`, and every
      `ui-overrides.json` key for in-scope modules still holds its pinned
      text with `human` provenance in the new `.origin`

**Checkpoint**: do-not-touch guarantee proven — the refresh is safe to commit.

---

## Phase 5: User Story 3 — Safe and repeatable operation (Priority: P2)

**Goal**: The remaining operability contract points proven (preview and scope
already covered by T008).

**Independent Test**: contracts/redo-machine-cli.md invocations run offline.

- [X] T018 [P] [US3] Budget cap:
      `python -m translate.merge --all --redo-machine --exclude-module sftp --budget 1`
      stops before the first language starts translating — no network, no
      writes, working tree unchanged
- [X] T019 [P] [US3] Scoping and errors:
      `--dry-run --redo-machine --language czech --module zip` restricts to
      that pair; `--exclude-module bogus` → error exit 1;
      `--module sftp --exclude-module sftp` (empty set) → error exit 1
- [X] T020 [US3] Report completeness (FR-011, needs T010): the captured run
      output contains the per-language coverage table, validation-failure
      list, widen/accelerator counts, and characters spent + quota remaining

**Checkpoint**: operability contract fully demonstrated.

---

## Phase 6: Polish & documentation

- [X] T021 [P] Document `--redo-machine` and `--exclude-module` in
      `tools/translate/README.md` (Commands section + a note under the
      language-policy table that exclusion composes with `--all`)
- [X] T022 [P] Add `CHANGELOG.md` `[Unreleased]` → `### Changed` entry: all
      machine-translated UI strings re-translated with usage context across
      the 8 shipped languages (user-visible wording improvements product-wide)
- [X] T023 [P] Add the feature 055 line to `CLAUDE.md` "Recent Changes"
- [X] T024 Final idempotence check: plain `python -m translate.merge --dry-run`
      again reports 0 gaps (post-state consistent); commit the tooling delta,
      refreshed `translations/`, run-notes.md, and docs

## Dependencies

```text
Phase 1 (T001→T003; T002 ∥ T001)
  └─▶ Phase 2 (T004→T005 same file; T006→T007 same file, ∥ T004/T005; T008 last)
        ├─▶ US1: T009 → T010 → {T011, T012} → T013 → T014
        ├─▶ US2: T015 (∥ US1 until T010) → T016 (needs T010) ; T017 needs T010
        └─▶ US3: T018, T019 anytime after Phase 2 ; T020 needs T010
Phase 6: T021–T023 anytime after Phase 2; T024 last (needs US1+US2 complete)
```

- US2's verification gates the commit of US1's output — run T016/T017 before
  T024 even though US1 is the MVP.
- Only T010 spends DeepL quota; every other task is offline.

## Parallel Execution Examples

- After T003: T004+T005 (merge.py) alongside T006+T007 (uicontext.py).
- After Phase 2: T015 (write verifier), T018, T019, T021 while T010 runs.
- After T010: T011, T012, T017, T020 in parallel; T013 (long build) in the
  background while T014/T016 proceed.

## Implementation Strategy

MVP = Phases 1–3 (US1): the refreshed, rebuilt, spot-checked product. US2
(Phase 4) is the mandatory safety gate before committing that MVP's data —
cheap and fast, so in practice both P1 stories land together in one commit.
US3 (Phase 5) and polish (Phase 6) close the operability and documentation
contract. Everything is revertable as one commit of `tools/translate` +
`translations/` + docs.
