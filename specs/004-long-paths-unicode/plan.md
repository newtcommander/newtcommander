# Implementation Plan: Long Path and Unicode File Name Support

**Branch**: `004-long-paths-unicode` | **Date**: 2026-07-13 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/004-long-paths-unicode/spec.md`

## Summary

Fix the two root defects that prevent Open Salamander from handling
(1) paths longer than the legacy 260-character limit and (2) file
names outside the active ANSI code page — most visibly Unicode names
in decomposed (NFD) form such as "c" + combining caron.

Technical approach (details in [research.md](research.md)): change
the in-program string contract for names/paths from ANSI to **UTF-8
in the existing `char*` plumbing**, convert to UTF-16 at OS
boundaries through a new central I/O + conversion layer that
normalizes absolute paths to extended-length (`\\?\`) form for `W`
APIs, migrate rendering/measurement/input of names to `W` calls (GDI
does not honor per-process UTF-8), add NFC-normalized matching for
search/masks and locale collation for sort, bump the plugin interface
version (UTF-8 semantics + long-path limits) with a load-time
adaptation shim for legacy third-party binaries, and port all 35
bundled plugins to the new interface. The process code page is left
untouched to preserve legacy third-party plugin behavior.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI (no frameworks); internal shared libs (`src/common/`); no new external dependencies
**Storage**: Windows Registry for configuration (`REG_SZ` string values); NTFS/exFAT/FAT/network file systems as managed objects
**Testing**: No pre-existing automated test infrastructure. This feature adds (a) a small unit-test host for the new pure helpers (path normalization, UTF-8↔UTF-16, NFC matching, collation), (b) the manual verification matrix in [quickstart.md](quickstart.md) with scripted fixtures
**Target Platform**: Windows 11+ (per constitution); x64 primary, x86 shell extension kept building
**Project Type**: Native desktop application — 90-project MSBuild solution (1 core app, 35 plugins + lang modules, helpers)
**Performance Goals**: SC-009 — listing/sorting/scrolling 100,000-item directories within ±10% of the previous release; ASCII-only fast paths preserve current comparator costs
**Constraints**: Plugin ABI: legacy third-party `.spl` binaries must keep loading and working at current capability (adaptation shim, FR-014/FR-015); GPLv2-compatible code only; no cross-platform abstraction layers; `\\?\` never visible in UI or persisted data
**Scale/Scope**: ~2,224 source files; ~4,900 `MAX_PATH` occurrences in 441 files (per-site classification, R10); ~40 core files carry the hot name pipeline; 35 bundled plugins to port; plugin SDK headers (`src/plugins/shared/spl_*.h`) revised

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| # | Principle | Evaluation | Status |
|---|-----------|------------|--------|
| I | Build Reproducibility | Manifest additions (`longPathAware`) and SDK header changes live in the MSBuild project/props files; no manual build steps introduced; single-command build preserved | PASS |
| II | Backward Compatibility | Plugin interface change rides the SDK's existing load-time version negotiation: legacy binaries keep loading via the adaptation shim at their current capability (FR-014/FR-015); migration path = recompile against v-next SDK, documented in [contracts/](contracts/). Default-on behavior change is a defect fix — previously *failing* inputs start working; previously *working* inputs are regression-guarded (FR-015, SC-006). Justification recorded in Complexity Tracking | PASS (justified) |
| III | Incremental Modernization | The scope is program-wide (stakeholder decision, spec Clarifications), but delivery is sequenced into independently reviewable, testable increments (see Increment Sequencing); each increment leaves the app fully functional; no adjacent-code refactoring beyond the name pipeline | PASS (justified) |
| IV | Windows Platform Commitment | Pure WinAPI throughout (`NormalizeString`, `CompareStringEx`, `W` file APIs); Windows 11+ only; no new dependencies | PASS |
| V | Plugin Architecture Preservation | Interface documented **before** modification in [contracts/plugin-interface-vnext.md](contracts/plugin-interface-vnext.md); plugin system preserved and extended; bundled plugins ported as part of the feature | PASS |

**Post-Phase-1 re-check**: design artifacts (research.md,
data-model.md, contracts/) introduce no new violations; the two
justified items stand with their mitigations. GATE: PASS.

## Architecture Decisions (summary)

Authoritative rationale in [research.md](research.md):

- **R1** UTF-8 as the internal narrow encoding; `char*` shapes preserved
- **R2** Central W-API I/O layer with `\\?\` normalization; no dependency on the `LongPathsEnabled` registry opt-in
- **R3** Process ACP untouched (legacy third-party plugin safety); no `activeCodePage` manifest
- **R4** NFC normalization at compare time for matching (FR-008); stored names never modified
- **R5** Sort via `CompareStringEx` with ASCII fast path; cached keys only if the SC-009 benchmark demands
- **R6** Name rendering/measurement via `W` GDI calls (GDI ignores per-process UTF-8)
- **R7** Unicode window class for quick-search input; W text APIs for edit controls
- **R8** Plugin interface version bump + load-time legacy adaptation shim
- **R9** Registry string I/O switched to W APIs (old configs load correctly by construction)
- **R10** `MAX_PATH` retained only for single components/8.3/non-path uses; full-path buffers become dynamic `SalPathBuf`

### Increment Sequencing (constitution III)

1. **Foundation** — `src/common/`: UTF-8↔UTF-16 conversion, `SalPathBuf`, `\\?\` normalizer, NFC/collation helpers + unit-test host. No behavior change.
2. **File I/O core** — `safefile.cpp`, `worker.cpp`, panel `ReadDirectory` (`fileswn3.cpp`) onto the W layer; `CFileData` carries UTF-8; long paths + Unicode names work in panels and copy/move/delete/rename (US1+US2 core).
3. **Display & input** — draw/measure sites to W (R6), quick-search class flip (R7), dialogs/title/progress texts; sorting (R5).
4. **Search & persistence** — Find, wildcard masks, NFC matching (US3); registry layer to W (R9); history/hot paths/session.
5. **Shell integration** — clipboard, drag & drop, context menus, external launch (`shellib.cpp`, `shellsup.cpp`).
6. **Plugin SDK v-next** — `spl_*.h` revision, load-time version gate + legacy shim, FR-014 refusal UX; SDK migration guide.
7. **Bundled plugin ports** — by category: archivers → viewers → filesystem/network → utilities; each plugin an independent increment validated against SC-008.
8. **Hardening** — equivalent-pair notice (FR-007), edge-case matrix, SC-009 benchmark, regression pass (SC-006).

## Project Structure

### Documentation (this feature)

```text
specs/004-long-paths-unicode/
├── plan.md              # This file
├── spec.md              # Feature specification (clarified)
├── code-analysis.md     # Source evidence (pre-plan analysis)
├── research.md          # Phase 0 — decisions R1–R10
├── data-model.md        # Phase 1 — name/path representation model
├── quickstart.md        # Phase 1 — build, fixtures, verification matrix
├── contracts/
│   ├── plugin-interface-vnext.md   # Plugin SDK contract (v-next + legacy shim)
│   └── app-manifest.md             # Application manifest contract
└── tasks.md             # Phase 2 (/speckit.tasks — not created here)
```

### Source Code (repository root)

```text
src/
├── common/                    # NEW: salpath.{h,cpp} (SalPathBuf, \\?\ normalizer),
│   │                          #      salunicode.{h,cpp} (UTF-8↔UTF-16, NFC, collation)
│   ├── strutils.{h,cpp}       # MODIFIED: matching helpers (R4) replace dead FoldString experiment
│   └── winlib.cpp             # MODIFIED: activate dormant W window-class path (R7)
├── regwork.cpp                # MODIFIED: registry façade SetValueAux/GetValueAux → W APIs (R9)
├── manifest.xml               # MODIFIED: + longPathAware (contracts/app-manifest.md)
├── safefile.cpp               # MODIFIED: W-API open path, drop MAX_PATH rejection + 8.3 workaround
├── worker.cpp                 # MODIFIED: file-operation engine onto W layer
├── fileswn*.cpp, fileswnd.h   # MODIFIED: ReadDirectory, panel model (UTF-8 names, dynamic Path)
├── sort.cpp                   # MODIFIED: UTF-8-aware comparators, ASCII fast path
├── salamdr1-3.cpp, consts.h   # MODIFIED: Sal* path helpers → UTF-8/dynamic
├── find*.cpp, viewer*.cpp     # MODIFIED: search + viewer open paths
├── shellib.cpp, shellsup.cpp  # MODIFIED: clipboard/DnD/context-menu surfaces
├── plugins*.cpp               # MODIFIED: plugin loader — version gate + legacy shim
├── plugins/
│   ├── shared/spl_*.h         # MODIFIED: SDK v-next (UTF-8 semantics, widened NameLen, limits)
│   └── <35 plugin dirs>/      # MODIFIED: ported per category (increments 7a–7d)
├── vcxproj/                   # MODIFIED: manifest (longPathAware), new common files,
│                              #           NEW: test host project for foundation helpers
└── lang/ + translations/      # MODIFIED: new strings (FR-007 notice, FR-014 messages)
```

**Structure Decision**: Single native-app solution retained. New code
is confined to two new `src/common/` modules plus a unit-test host
project; everything else is in-place migration of existing files,
sequenced per the increments above so each lands independently
buildable and testable.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Program-wide scope in one feature (tension with Constitution III) | Stakeholder decision (spec Clarifications): both defects affect the whole program incl. plugins; a core-only fix leaves bundled archivers corrupting the very names the panels now handle | Core-only scope with plugin follow-up was proposed and explicitly rejected by the stakeholder; mitigation = 8 independently deliverable increments, each leaving the app shippable |
| Default-on behavior change without opt-in (Constitution II) | The change is a defect fix: inputs that previously failed (long paths, non-ACP/NFD names) start working; nothing previously working changes (FR-015, SC-006 regression gate) | An opt-in "Unicode mode" would fork every name-handling code path into two live variants — permanent doubled maintenance and the exact silent-corruption hazard this feature removes |
