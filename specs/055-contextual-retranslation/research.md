# Research: Contextual Re-translation of Machine-Translated UI Strings

**Feature**: 055-contextual-retranslation · **Date**: 2026-08-07

All unknowns from the Technical Context were resolved by reading the existing
pipeline (`tools/translate/*`, features 038/039/051) and probing the working
tree. No NEEDS CLARIFICATION markers remain.

## R1. How to select exactly the machine-provenance entries

**Decision**: add a `--redo-machine` flag to `translate.merge` that demotes
every `machine` entry in the loaded `.origin` sidecar to `english_fallback`
before matching — the exact mechanism `--redo-accelerators` already uses for
its narrower set (`merge.py:99–107`). `match()` then classifies those entries
as gaps, they are collected with per-section context by `collect_gaps()`, and
re-translated; `human` and `skip` entries never enter the gap set.

**Rationale**: `match()` already implements the three-way provenance logic
(machine kept without spending quota / human carried / fallback+gap re-sent).
Demotion-at-load reuses that logic unchanged, so the do-not-touch guarantee
for human entries is enforced by the same code path that has protected them
since feature 038, not by new filtering code.

**Alternatives considered**:
- `--retranslate` (exists): passes `legacy=None` — re-translates *everything*,
  destroying human translations. Correct for the SFTP first-run in 051 (no
  human legacy existed for a brand-new plugin), wrong here. Rejected.
- A standalone script diffing `.origin` and patching `.slt` files directly:
  duplicates validation, layout, override, and rebrand logic; bypasses the
  positional-structure guarantees. Rejected.

**Verified**: `.origin` sidecars exist for all 20 enabled modules in all 11
languages; enabled-language machine counts excluding sftp: czech 292,
dutch 559, french 403, german 366, hungarian 324, romanian 650, slovak 300,
spanish 363 → **3,257 entries**. Zero `english_fallback` entries exist in the
in-scope population, so FR-003's "do not re-send existing fallbacks" holds
vacuously — no suppression code is needed.

## R2. How to exclude the SFTP module from the default run

**Decision**: add a repeatable `--exclude-module <name>` flag to
`translate.merge`, applied after `load_enabled_modules()`; the feature run
uses `--all --redo-machine --exclude-module sftp`.

**Rationale**: sftp's machine entries were already translated with context in
feature 051 — re-sending them spends quota to reproduce (or worse, churn)
existing output. Exclusion must not cost the cross-module deduplication of
identical `(context, text)` pairs, which module-by-module runs would lose.

**Alternatives considered**:
- 18 separate `--module <name>` runs: loses cross-module dedup within each
  language, multiplies invocations and failure points. Rejected.
- Temporarily switching `sftp=off` in `plugins.cfg`: that file governs what
  the build compiles and ships; abusing it for a translation run risks
  committing or building a wrong product state. Rejected.

## R3. Context quality for the 13 modules without a domain description

**Decision**: extend `_DOMAINS` in `uicontext.py` with a one-clause English
description for every enabled module, sourced from
`architecture/09-plugin-catalog.md`: 7zip (7-Zip archives), dbviewer
(dBase/FoxPro/CSV viewer), diskmap (disk-usage treemap), filecomp (file
comparison), folders (shell virtual folders), peviewer (PE executable
inspector), portables (MTP/PTP portable devices), regedt (Windows Registry
panel), renamer (batch renamer), tar (tar/gzip archives), uncab (CAB
archives), undelete (FAT/NTFS file recovery), uniso (ISO images). Existing
entries (sftp, ftp, zip, checksum, mdview, pictview, salamand) stay.

Also broaden `_WORDS` (the greedy symbol-splitting vocabulary) with common
terms from the newly covered modules (archive, extract, compress, registry,
rename, compare, mask, drive, volume, device, preview, …) so
`humanize_symbol()` produces readable role descriptions outside the SFTP
vocabulary it was written for.

**Rationale**: the domain clause is the highest-leverage part of the context
sentence ("of an SFTP/SSH file-transfer client" is what turned "Host:" from a
talk-show presenter into a server address). The generic fallback "the X plugin
of a Windows file manager" is weaker for plugins whose name says nothing
(uncab, uniso, regedt). Cost is a dozen literal strings.

**Alternatives considered**: leaving the generic fallback — free but weakens
precisely the disambiguation this feature exists to add. Rejected.

**Verified**: `load_symbols()` works for all 20 enabled modules (salamand
2,660 symbols … diskmap 4), so per-control roles are available everywhere;
no loader changes needed.

## R4. Behaviour when a new translation fails validation

**Decision**: keep the existing policy unchanged (spec FR-006): `repair()` is
attempted, then `check()`; a still-invalid translation keeps English, the
entry's provenance becomes `english_fallback`, and the failure is listed in
the report. No old-text resurrection logic.

**Rationale**: matches the spec, matches `--redo-accelerators` precedent, and
keeps the merge deterministic. Risk is bounded: placeholders/accelerators are
protected by DeepL's `tag_handling=xml` + `ignore_tags` before the engine can
break them, historical fallback rate for the in-scope population is currently
zero, and SC-005 caps acceptable new fallbacks at <2% per language — the
post-run report verifies this directly.

## R5. Template source and freshness

**Decision**: export templates from the existing Release_x64 tree:
`src\vcxproj\build_langs.cmd --export-templates release`.

**Rationale**: templates must describe the *current* `english.slg` structure
(positional import). The working tree has no Debug_x64 build; Release_x64 was
built 2026-08-07 01:12 from the current source state (last code commit
b195fec [054] predates it), so it is current. `build_langs.cmd` builds
`translator.exe` itself if missing.

**Alternatives considered**: full Debug rebuild first — slower for no
structural difference (resources are identical between configurations).
Rejected as unnecessary; if the export ever reports a structure mismatch
against committed `.slt` files, rebuilding is the documented fallback.

## R6. Quota plan

**Decision**: run without a `--budget` cap, but only after a
`--dry-run --redo-machine --exclude-module sftp` preview confirms the
estimate. Expected volume: ≈3,257 texts, average UI-string length ~25–35
chars → **roughly 80–120k characters**, far under the 500k monthly free tier;
the tool prints remaining quota before sending and spend after.

**Rationale**: per-language processing already gives clean stopping points
(the budget check runs before each language starts), and the report prints
spend/remaining (FR-010/FR-011). The dry-run number is authoritative — if it
unexpectedly exceeds ~350k chars, fall back to running language-by-language
across quota months (not anticipated).

**Verified**: DeepL key file present at `temp/deepl_key.txt`; deduplication of
identical `(context, text)` pairs is already in `collect_gaps()`; DeepL's
`context` request field does not count toward the character bill (per DeepL
API documentation; texts do).

## R7. Verifying the do-not-touch guarantee (SC-002)

**Decision**: after the run, a verification script (scratchpad, not shipped)
loads each pre-run `.slt`/`.origin` pair from git (`HEAD` version) and the
post-run working-tree version via `translate.slt`, keys entries with
`match.entry_key`, and asserts: every entry whose pre-run provenance was
`human` or `skip` has identical text afterwards; every changed entry had
pre-run provenance `machine`. Output (counts per language/module, zero
violations) is recorded in the implementation notes.

**Rationale**: uses the pipeline's own parser and identity rules, so the check
cannot drift from what the merge actually did; git provides the authoritative
"before" state without a manual backup step.

**Alternatives considered**: eyeballing `git diff` — 3,257 changed lines
across ~150 files is not reviewable by eye for a byte-level guarantee.
Rejected as the sole check (a sampled human diff review still happens for
SC-003).

## R8. Changelog / versioning

**Decision**: add a `### Changed` entry to the existing `[Unreleased]` section
of `CHANGELOG.md` describing the translation refresh (user-visible: better
non-English UI wording across the product). No version bump — the constitution
ties the bump to a release, and `[Unreleased]` already carries 053/054 entries
awaiting one.
