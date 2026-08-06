# Implementation Plan: Fix Plugin Name Encoding in Plugin Manager

**Branch**: `052-fix-plugin-name-encoding` | **Date**: 2026-08-06 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/052-fix-plugin-name-encoding/spec.md`
**Root-cause record**: [investigation.md](investigation.md) · **Decisions**: [research.md](research.md)

## Summary

Plugin names of not-loaded plugins render as mojibake in the Czech Plugins
Manager because `CPluginData::Name` carries CP1250 when set by a loaded plugin
(`LoadStringA` path) but UTF-8 when read back from the registry (feature-004
facade), and the name column is filled through the ANSI `ListView_SetItemText`.
Fix = define the encoding of `CPluginData` translated metadata as UTF-8 and
normalize at the producer boundary (`SetBasicPluginData`), render the name
column through the tolerant `SalListViewSetItemTextU8`, convert the 15
now-genuinely-mixed `LoadStr`-template composition sites to `LoadStrU8`,
harden the encoding guard (contract-tracked identifiers, runtime tests,
build fails when the checker cannot run), and set the ZIP plugin's display
name to the identical identifier "ZIP" in all languages with an
`ui-overrides.json` pin against future machine re-translation.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022); Windows Batch + PowerShell 5.1 build scripts; Python 3.13 for repo tooling
**Primary Dependencies**: Pure WinAPI (no frameworks); internal shared libs `src/common/` (salunicode, winlib); existing helpers `SalListViewSetItemTextU8`, `LoadStrU8`, `SalU8ToW`/`SalWToU8`
**Storage**: Windows Registry `HKCU\Software\Tandem Commander\0.1\Plugins\<n>` (REG_SZ, UTF-16 at rest; crosses the feature-004 facade as UTF-8) — format unchanged, no migration
**Testing**: `src/saltests` (in-app test suite, pattern from features 042/043 at `saltests.cpp:802-927`); `tools/check_encoding.py --strict` wired into `build.cmd`; manual validation per [quickstart.md](quickstart.md)
**Target Platform**: Windows 11+, x64
**Project Type**: Desktop application (existing monolithic WinAPI app + plugin DLLs)
**Performance Goals**: N/A (one-time per-string conversions at plugin registration and dialog fill; imperceptible)
**Constraints**: Plugin ABI (interface version 105) MUST NOT change; registry data format MUST NOT change; no configuration migration (constitution II); UI unchanged except correct text (constitution VI)
**Scale/Scope**: ~6 metadata fields × 20 shipped plugins; 1 producer boundary + 1 listview sink + 15 composition sites across 4 core files; 6 `.slt` lines + 1 JSON; guard changes in 1 Python tool + `build.cmd` + saltests

## Constitution Check

*GATE: evaluated against Tandem Commander Constitution v3.1.0 — PASS (pre-Phase-0 and re-checked post-Phase-1).*

| Principle | Verdict | Note |
|---|---|---|
| I. Build Reproducibility | PASS (justified change) | Build stays single-command. Change: missing python now fails the build instead of silently skipping the encoding guard. Python 3.13 is already a committed repo technology (`pyproject.toml`, translation tooling); prerequisite documented in quickstart. Justification: FR-005 — a guard that can silently not run is not a guard (this exact hole coexisted with the defect). |
| II. Backward Compatibility | PASS | Registry format and values untouched; no migration; plugin ABI 105 unchanged (host-side normalization only; `SetBasicPluginData` signature intact). Display-only behavior change = the bug fix itself. |
| III. Incremental Modernization | PASS | Only touched sites are modified; the 15-site sweep is bounded, mechanical, and each site independently reviewable; no adjacent refactoring. |
| IV. Windows Platform Commitment | PASS | WinAPI conversions only (`MultiByteToWideChar` et al. via existing helpers). |
| V. Plugin Architecture Preservation | PASS | Plugin interface documented before modification: [contracts/plugin-metadata-encoding.md](contracts/plugin-metadata-encoding.md); no interface change. |
| VI. UI Consistency | PASS | No dialog/control changes; text rendering only. |
| Release Documentation | PASS | CHANGELOG.md entry included in the implementation (research D7); version bump deferred to the release change per constitution. |

## Project Structure

### Documentation (this feature)

```text
specs/052-fix-plugin-name-encoding/
├── spec.md              # Feature specification
├── investigation.md     # Root-cause record (3-agent evidence)
├── plan.md              # This file
├── research.md          # Phase 0 decisions D1–D7
├── data-model.md        # Phase 1: CPluginData metadata encoding states
├── quickstart.md        # Phase 1: validation guide
├── contracts/
│   └── plugin-metadata-encoding.md   # Phase 1: the encoding contract
└── tasks.md             # Phase 2 (/speckit-tasks — not created here)
```

### Source Code (repository root)

```text
src/
├── plugins.h                  # CPluginData: document field encoding contract
├── plugins1.cpp               # SetBasicPluginData: normalize to UTF-8 (D1);
│                              #   9 mixed-composition sites → LoadStrU8 (D3)
├── plugins2.cpp               # AddNamesToListView: name column → SalListViewSetItemTextU8 (D2)
├── dialogs5.cpp               # 4 mixed-composition sites → LoadStrU8 (D3)
├── fileswn7.cpp               # 2 mixed-composition sites → LoadStrU8 (D3)
├── common/
│   ├── salunicode.h/.cpp      # new helper: probe-valid-UTF-8-else-ACP→UTF-8 (D1)
│   └── winlib.h/.cpp          # no change expected (SalListViewSetItemTextU8 exists)
└── saltests/saltests.cpp      # new runtime tests (D4.2)

tools/check_encoding.py        # UTF8_IDENT += plugin metadata identifiers (D4.1)
build.cmd                      # encoding guard: python missing → build FAILS (D4.3)

translations/
├── czech/zip.slt              # 1007,1,"PSČ"          → "ZIP" (D5)
├── slovak/zip.slt             # 1007,1,"PSČ"          → "ZIP"
├── french/zip.slt             # 1007,1,"Code postal"  → "ZIP"
├── spanish/zip.slt            # 1007,1,"Código postal"→ "ZIP"
├── german/zip.slt             # 1007,1,"ZIP-Archiv"   → "ZIP"
├── chinesesimplified/zip.slt  # 1007,1,"邮编"          → "ZIP"
└── ui-overrides.json          # zip.IDS_PLUGINNAME = "ZIP" for all 10 languages (D5)

CHANGELOG.md                   # Fixed + Changed entries (D7)
```

**Structure Decision**: existing monolithic layout; no new projects, files
limited to one new helper pair in `src/common/salunicode.*` — everything else
modifies files in place.

## Implementation Phases (for /speckit-tasks)

1. **Contract & producer** (D1): audit `CPlugins::Save` for the complete list
   of REG_SZ translated fields (Name, Version, Copyright, Extensions,
   Description, ChDrvMenuFSItemName confirmed; check menu items /
   BugReport* / thumbnail masks region below `plugins2.cpp:1655`); add the
   normalization helper; apply in `SetBasicPluginData` (and the
   ChDrv/menu-item setters if their strings persist); document the contract
   in `plugins.h` and `contracts/`.
2. **Sinks & compositions** (D2+D3): listview name column; then the 15
   annotated sites — convert template to `LoadStrU8`, verify sink is
   UTF-8-capable (pattern: features 042/043), delete the allow-annotation.
3. **Guard** (D4): checker identifiers; saltests; `build.cmd` hard-fail.
4. **ZIP name** (D5): 6 `.slt` edits + `ui-overrides.json` pins.
5. **Docs & validation**: CHANGELOG entry; run quickstart scenarios
   (Czech UI restart test, guard seeded-defect test, all-languages ZIP check,
   saltests, full build).

## Complexity Tracking

No constitution violations to justify. The single gate-relevant change
(python becomes a hard build prerequisite) is recorded and justified in the
Constitution Check table above.
