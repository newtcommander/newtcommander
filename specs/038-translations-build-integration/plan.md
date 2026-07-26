# Implementation Plan: Translations Build Integration

**Branch**: `038-translations-build-integration` | **Date**: 2026-07-26 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/038-translations-build-integration/spec.md`

## Summary

Ship the product in **12 languages** (English + 10 existing + new Ukrainian) for
the main application and all 19 enabled plugins — 240 language modules.

The runtime side already works: `CLanguageSelectorDialog`, `Configuration.SLGName`,
Windows-locale matching, per-plugin fallback, and the "translation incomplete"
notice are all present and correct (`dialogs2.cpp`, `salamdr1.cpp:3947-4039`,
`plugins1.cpp:1710`). This feature supplies **content** for that machinery.

Three pieces, in dependency order:

1. **`.slg` production pipeline** — the existing Translator tool
   (`src/translator/`) already turns `english.slg` + a `.slt` text archive into a
   translated `.slg` (`CData::Save()` → `BeginUpdateResource` /
   `UpdateResource(RT_STRING|RT_MENU|RT_DIALOG)` / VERSIONINFO patch). It has
   headless command-line modes. A build script drives it per
   (module × language) after `english.slg` links.

2. **Translation-source migration** — and this is the constraint that shapes
   everything: `CData::ImportTextArchive` is **strictly positional**
   (`trldata.cpp:2474`), so the repository's Salamander-4.0-era `.slt` files
   **cannot** be imported into today's resources. The pipeline is therefore
   *export a canonical English template from the current `english.slg` → merge
   legacy translations into it by ID → import the merged result*. Everything the
   legacy data doesn't cover is machine-translated.

3. **Machine translation prep** — an offline Python tool
   (`tools/translate/`) using the `anthropic` SDK with `claude-opus-5` over the
   Message Batches API. It runs by hand, its output is committed as ordinary
   translation source, and `build.cmd` never calls it (FR-023, and what makes
   FR-024 reproducibility trivially true).

No C++ source changes to the application or plugins. No plugin ABI change. No
registry change. The only code added is build tooling.

## Technical Context

**Language/Version**: Python 3.13 for the offline tooling (matches
`tools/pyproject.toml`, `requires-python = ">=3.13"`); Windows Batch + PowerShell
for build integration; no C++ changes
**Primary Dependencies**: existing `translator.exe` (already in `salamand.sln`,
project `{C5833A09-...}`); `anthropic` Python SDK (offline prep only, new entry
in `tools/pyproject.toml`); no new runtime or build dependencies
**Storage**: `translations/<language>/<module>.slt` — UTF-8-with-BOM text
archives, committed. Generated `.atp` projects and seeded `.slg` copies live in
`$(OPENSAL_BUILD_DIR)` and are gitignored
**Testing**: `translator.exe -quiet-validate-layout` per (module × language) as
the automated clipped-control gate (SC-005); a post-build `VS_FIXEDFILEINFO`
verifier (FR-026); manual run-verification with screen captures, following the
pattern in `specs/036-plugin-dark-theme/validation-results.md`
**Target Platform**: Windows 11+
**Project Type**: desktop-app build tooling + resource data
**Performance Goals**: full build adds 220 short-lived `translator.exe`
invocations; incremental builds skip pairs whose inputs are unchanged. No
runtime cost — language modules load exactly as `english.slg` does today
**Constraints**: `.slt` import is positional and all-or-nothing (a single
mismatch aborts the whole file); Translator signals **success with exit code 1**
and reports errors through `MessageBox`, so every invocation must be
timeout-guarded or a build can hang; `IsSLGFileValid` requires an exact
`FILEVERSION` match against the owning module
**Scale/Scope**: 12 languages × 20 modules = 240 language modules; ~6,500
translation units per language; ~20k–50k units to machine-translate (≈$50 at
Opus 5 batch rates)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Gate | Verdict |
|-----------|------|---------|
| I. Build Reproducibility | Machine translation is explicitly **excluded** from the build (FR-023): its output is committed and consumed like any other source, so two clean builds are byte-equivalent and the build stays offline and single-command. `translator.exe` is built from the existing solution — no new manual step, no download. Outputs stay under `OPENSAL_BUILD_DIR` | PASS |
| II. Backward Compatibility | No ABI change, no registry change, no behavior change to any existing path. English remains the default and the fallback. Adding language modules is purely additive — a user who never opens the language chooser sees today's product | PASS |
| III. Incremental Modernization | Reuses the existing Translator data layer rather than reimplementing `DLGTEMPLATEEX`/menu/string serialization; no refactor of `src/translator/` or of adjacent code. New tooling is additive files | PASS |
| IV. Windows Platform Commitment | Build tooling is Batch/PowerShell + a Win32 tool; the Python prep tool is developer-side only and never required to build or run the product | PASS |
| V. Plugin Architecture Preservation | Plugin language loading is used exactly as designed (`CSalamanderPluginEntry::LoadLanguageModule`); shipping a complete language set for every enabled plugin is precisely what removes the per-plugin fallback prompt. No plugin interface touched | PASS |
| VI. UI Consistency | Dialog templates keep their `DIALOGEX` / `DS_SHELLFONT` / `FONT 8, "MS Shell Dlg"` declarations — the Translator patches control text and geometry inside the existing template, never the template class or font. No process-wide visual change. Layout is gated by `-quiet-validate-layout` so no language ships with clipped controls | PASS |

**Post-Phase-1 re-check**: the design adds only build-time file generation
(`.atp` into the build dir), a byte copy of `english.slg` per language, and
committed text data. No new runtime code path exists to violate a gate. All
gates still **PASS**.

One item deserves explicit note rather than a violation entry: Constitution I
requires no manual steps *in the build pipeline*. The machine-translation tool is
a manual step, but it is a **content-authoring** step outside the pipeline —
the same category as `python tools/brand/gen_icons.py` (feature 035), which
CLAUDE.md already documents as hand-run. FR-023 makes that boundary a stated
requirement rather than an accident.

## Project Structure

### Documentation (this feature)

```text
specs/038-translations-build-integration/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output — the .slg pipeline investigation
├── data-model.md        # Phase 1 output — .slt / .atp / coverage entities
├── quickstart.md        # Phase 1 output — how to run each stage
├── contracts/
│   ├── slt-format.md    # The .slt text-archive grammar (import is positional)
│   └── translator-cli.md # translator.exe quiet-mode invocation contract
└── tasks.md             # Phase 2 output (/speckit.tasks — NOT created here)
```

### Source Code (repository root)

```text
translations/                      # COMMITTED translation source (existing dir)
├── czech/ dutch/ french/ german/ hungarian/
├── chinesesimplified/ romanian/ russian/ slovak/ spanish/
│   └── <module>.slt               # regenerated: current-structure, merged
├── ukrainian/                     # NEW — 20 machine-translated modules
│   └── <module>.slt
└── languages.cfg                  # NEW — shipped-language registry
                                   #   (folder, LANGID, display name, author, web)

tools/translate/                   # NEW — offline machine-translation prep
├── __init__.py
├── slt.py                         # .slt parse / serialize (round-trip exact)
├── merge.py                       # template + legacy + machine → merged .slt
├── translate.py                   # anthropic Batches API driver
├── validate.py                    # placeholders, accelerators, \t-shortcuts
├── rebrand.py                     # legacy product/vendor/URL rewrite (FR-018..021)
└── README.md

tools/pyproject.toml               # add `anthropic` dep + console entry points

src/vcxproj/
├── build_langs.cmd                # NEW — the (module × language) driver:
│                                  #   seed .slg, emit .atp, run translator,
│                                  #   verify FILEVERSION, emit coverage report
└── gen_atp.ps1                    # NEW — .atp project generator

build.cmd                          # extend: invoke build_langs.cmd in `full`
                                   #   mode; report per-language counts in place
                                   #   of the current "(english)" line
```

**Structure Decision**: three cleanly separated layers, matching the three
stages. `translations/` stays the single committed source of truth (FR-017,
FR-010) and keeps its existing per-language folder layout so the shape stays
familiar. `tools/translate/` follows the established `tools/comments/` and
`tools/brand/` precedent for developer-side Python that is never part of the
build. Build integration lives in `src/vcxproj/` beside the other build scripts
(`signslgs.cmd`, `gen_plugins_filter.ps1`) rather than in the project files,
because the work is a per-pair loop over an external tool, not an MSBuild target.

`contracts/` is included here — unlike feature 037 — because this feature does
depend on two external formats it must not break: the `.slt` grammar that
`ImportTextArchive` parses, and the `translator.exe` quiet-mode CLI. Both are
frozen by existing code, so they are documented as consumed contracts.

## Phased delivery

Maps to the spec's user stories; each phase is independently verifiable.

| Phase | Delivers | Gate |
|---|---|---|
| **P1 — pipeline** | `build_langs.cmd` + `gen_atp.ps1`; one language (Czech) round-trips end to end for the main app | Czech `salamand.slg` loads, app runs in Czech |
| **P2 — migration** | `tools/translate/slt.py` + `merge.py`; all 10 legacy languages regenerated to current structure for every enabled module | US1 + US2: chooser lists 11, no per-plugin prompts |
| **P3 — machine translation** | `translate.py` + `validate.py`; gaps filled for the 4 untranslated plugins and post-4.0 drift | US3: SFTP/MDView localized |
| **P4 — Ukrainian** | `translations/ukrainian/` in full | US4 |
| **P5 — rebrand + polish** | `rebrand.py`; layout fixes; coverage report wired into `build.cmd` | US5, SC-005, SC-007 |

## Complexity Tracking

No Constitution Check violations — table not needed.
