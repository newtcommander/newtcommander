# Implementation Plan: Contextual Re-translation of Machine-Translated UI Strings

**Branch**: `055-contextual-retranslation` | **Date**: 2026-08-07 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/055-contextual-retranslation/spec.md`

## Summary

Feature 038 filled translation gaps by machine-translating bare strings;
feature 051 introduced context-aware translation (module domain + section +
control roles + neighbouring labels sent to the engine) but applied it only to
the SFTP plugin. This feature re-runs machine translation **with context** for
every entry whose `.origin` provenance is `machine`, across the 8 enabled
languages and all enabled modules except `sftp` — ≈3,257 entries — while
guaranteeing human/skip entries stay byte-identical.

Two small tooling deltas make the existing pipeline do this safely, then the
run itself is executed and its output committed:

1. `tools/translate/merge.py`: new `--redo-machine` flag — generalizes the
   existing `--redo-accelerators` demotion so **all** `machine`-provenance
   entries become gaps and are re-sent with context; new repeatable
   `--exclude-module` flag to keep `sftp` out of the run without losing
   cross-module deduplication.
2. `tools/translate/uicontext.py`: extend `_DOMAINS` with a one-clause domain
   description for every enabled module (13 of 20 currently fall back to a
   generic phrase) and broaden the `_WORDS` vocabulary used to humanize
   resource symbols beyond SFTP's terms.

Then: export current templates, dry-run to fix the exact scope and character
estimate, run the re-translation per the quota plan, verify (provenance-scoped
diff, `.slt` round-trip, full language build), spot-check Czech and Slovak,
and record the refresh in `CHANGELOG.md`.

## Technical Context

**Language/Version**: Python 3.13+ (`tools/`, stdlib-only code path for
`translate.merge`/`deepl`; run via `PYTHONPATH=tools` or `pip install -e tools`);
Windows batch + PowerShell for the language build (`src/vcxproj/build_langs.cmd`)
**Primary Dependencies**: DeepL REST API v2 free tier (key present at
`temp/deepl_key.txt`, gitignored); existing `tools/translate` package
(feature 038/039/051); `translator.exe` (built on demand by `build_langs.cmd`)
**Storage**: committed `translations/<language>/<module>.slt` (UTF-8-BOM,
positional import) + `translations/<language>/<module>.origin` JSON sidecars;
templates exported to `build/tandemcommander/translator/templates` (never
committed)
**Testing**: `python -m translate.slt --verify` (byte-exact round-trip),
`merge --dry-run` (scope/cost preview, offline), a provenance-scoped diff
check proving human/skip entries unchanged (SC-002), `build_langs.cmd` /
`build.cmd full` (positional import + .slg production for all 8 languages),
manual spot-check of ≥20 entries in Czech and Slovak (SC-003)
**Target Platform**: developer workstation (Windows 11); the shipped product
is unaffected at build time — the build never touches the network
**Project Type**: developer tooling delta + committed localization-data refresh
**Performance Goals**: n/a — one-shot batch; DeepL batches of ≤50 texts,
grouped per section context
**Constraints**: `.slt` import is strictly positional (no row add/remove/
reorder vs. the current template); DeepL free tier bills characters of the
texts sent (500,000/month); runs must stop cleanly at language boundaries
**Scale/Scope**: 8 languages × 19 modules, ≈3,257 machine-provenance entries
(czech 292, dutch 559, french 403, german 366, hungarian 324, romanian 650,
slovak 300, spanish 363), estimated well under one monthly quota

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Verdict | Rationale |
|---|---|---|
| I. Build Reproducibility | PASS | The translation tooling stays maintainer-run and offline from the build's perspective; the build consumes committed `.slt` source exactly as before. No build-script changes. |
| II. Backward Compatibility | PASS | No code, config, or registry change; only shipped UI text improves. Positional `.slt` structure is preserved by construction (template reproduction). |
| III. Incremental Modernization | PASS | Tooling delta is two small, reviewable changes generalizing an existing mechanism (`--redo-accelerators`); the data refresh is a single revertable commit. |
| IV. Windows Platform Commitment | PASS | No application code touched. |
| V. Plugin Architecture Preservation | PASS | No plugin API or packaging change; `.slg` modules rebuilt from refreshed source only. |
| VI. UI Consistency | PASS | Existing layout machinery (control widening, accelerator dedupe) applies to the new texts, same as for SFTP in 051. |
| Release Documentation | PASS (action) | FR-013: add a `CHANGELOG.md` entry under `[Unreleased]` describing the translation refresh, following the 052–054 pattern. |

**Post-Phase-1 re-check**: design introduces no new projects, dependencies, or
build steps — all gates still PASS.

## Project Structure

### Documentation (this feature)

```text
specs/055-contextual-retranslation/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   └── redo-machine-cli.md   # CLI contract for the new merge flags
└── tasks.md             # Phase 2 output (/speckit-tasks)
```

### Source Code (repository root)

```text
tools/translate/
├── merge.py             # + --redo-machine, + --exclude-module (CLI + gap logic)
├── uicontext.py         # + _DOMAINS entries for all enabled modules, + _WORDS
└── README.md            # document the new flags alongside the existing ones

translations/
├── <language>/<module>.slt      # refreshed machine entries (8 languages × 19 modules)
├── <language>/<module>.origin   # provenance sidecars rewritten by the run
└── ui-overrides.json            # unchanged; pinned texts keep winning

CHANGELOG.md             # [Unreleased] entry for the translation refresh
```

**Structure Decision**: no `src/` change at all. The feature lives entirely in
the developer-side translation tooling plus the committed localization data it
regenerates. Templates land in `build/tandemcommander/translator/templates`
(gitignored build output), exported from the up-to-date Release_x64 tree.

## Complexity Tracking

No constitution violations — table not needed.
