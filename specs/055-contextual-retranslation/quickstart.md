# Quickstart: Contextual Re-translation — validation guide

**Feature**: 055-contextual-retranslation

End-to-end validation that the feature works. Commands run from the
repository root on Windows; the Python package is used in-place via
`PYTHONPATH=tools` (or `pip install -e tools`).

## Prerequisites

- Release_x64 tree present and current (`build.cmd full release` has run since
  the last source change) — needed once, for template export.
- DeepL API key at `temp\deepl_key.txt` (step 4 only; every other step is
  offline).

## 1. Export current-structure templates (offline)

```bat
src\vcxproj\build_langs.cmd --export-templates release
```

**Expected**: `build\tandemcommander\translator\templates\<module>.slt` exists
for all 20 enabled modules.

## 2. Baseline sanity (offline)

```bat
python -m translate.slt --verify
python -m translate.merge --dry-run
```

**Expected**: round-trip verify passes for every committed `.slt`; the plain
dry-run reports **0 gaps** for the 8 enabled languages (the tree is fully
translated today — this is the pre-change baseline).

## 3. Scope preview with the new flags (offline)

```bat
python -m translate.merge --dry-run --redo-machine --exclude-module sftp
```

**Expected**:
- per-language gap counts equal the `.origin` machine counts (czech 292,
  dutch 559, french 403, german 366, hungarian 324, romanian 650, slovak 300,
  spanish 363; total 3,257 — see research.md R1);
- `sftp` appears in no per-module output;
- character estimate well under the free-tier month (≈80–120k);
- exit 0, no files modified (`git status` clean), no network use.

## 4. The re-translation run (spends DeepL quota)

```bat
python -m translate.merge --all --redo-machine --exclude-module sftp
```

**Expected**: quota printed before/after; coverage table shows human counts
**unchanged** vs. baseline and machine counts ≈ baseline machine counts;
validation failures (new fallbacks) < 2% per language (SC-005); modified files
are only `translations/<lang>/<module>.slt|.origin` for the 8 enabled
languages, never `sftp.*` and never a disabled language's files.

## 5. Do-not-touch verification (SC-002, offline)

Run the verification script (see tasks.md — it compares `HEAD` vs. working
tree using `translate.slt` + `match.entry_key` + the `HEAD` `.origin`):

**Expected**: 0 violations — every entry changed had pre-run provenance
`machine`; every `human`/`skip` entry is byte-identical. Also re-run
`python -m translate.slt --verify` (round-trip still passes).

## 6. Build gate (SC-004, offline)

```bat
build.cmd full
```

**Expected**: build succeeds; every enabled language imports positionally
(zero import errors from `build_langs`); `.slg` files produced for 8 languages
× 20 modules; `tools\check_encoding.py` guard passes.

## 7. Spot-check (SC-003)

Sample ≥20 re-translated entries each for Czech and Slovak from the git diff,
prioritizing one-word labels, buttons, and column headings. **Expected**:
≥90% read correctly for their UI location; no "wrong-sense" translations of
the talk-show-host class among checked entries. Optionally launch
`tandemcommander.exe`, switch language to Czech, and open a few affected
dialogs (e.g. plugin configuration dialogs) to see the strings in place.

## 8. Documentation

**Expected**: `CHANGELOG.md` `[Unreleased]` contains the translation-refresh
entry; `tools/translate/README.md` documents `--redo-machine` and
`--exclude-module`.
