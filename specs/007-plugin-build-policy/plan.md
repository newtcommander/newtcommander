# Implementation Plan: Plugin Build Policy — Remove Obsolete Plugins and Introduce a Build-Time Plugin Configuration

**Branch**: `007-plugin-build-policy` | **Date**: 2026-07-16 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/007-plugin-build-policy/spec.md`

## Summary

Two coupled deliverables:

1. **Permanent removal of 8 obsolete plugins** (pak, unarj, unlha,
   unfat, wmobile, ieviewer, splitcbn, winscp): source trees, solution
   entries (14 projects: 7 plugin + 7 lang; winscp has no project),
   build-script steps, core-code references (standard-plugin table,
   packer defaults, WinSCP x86 special-casing, UI strings), help
   sources, translation sources, dev-tool lists, and docs. Existing
   installations are handled by extending the already-existing
   "silently uninstall plugins that no longer ship" suppress list in
   `plugins2.cpp`.
2. **A build-time plugin policy**: a new root file `plugins.cfg`
   (plain `name=on|off` lines, `#` comments) read by `build.cmd` on
   every run. A new PowerShell helper validates the config (5 error
   classes, fail before compilation), generates a **solution filter
   (`.slnf`)** that excludes disabled plugins from the MSBuild solution
   build, and reconciles the output `plugins\` directory and
   `plugins.ver` with the config on every run. Initial policy: 10
   plugins off, 18 on.

Technical approach: MSBuild natively builds solution-filter files, and
plugin projects pull their `lang_*` and helper projects via
`ProjectReference` (verified on 7zip), so filtering out a disabled
plugin's projects removes it from the build with no `.vcxproj` edits
and no loss of `/m` parallelism. Validation/generation logic lives in
one PowerShell script invoked by `build.cmd` (PowerShell is already a
build.cmd dependency for the plugins.ver timestamp).

## Technical Context

**Language/Version**: Windows Batch (`build.cmd`), PowerShell 5.1 (helper script), C++20 `/std:c++latest` for small core edits, MSBuild 17 (VS2022) solution + solution-filter
**Primary Dependencies**: MSBuild `.slnf` support (VS2019 16.7+, present in required VS2022); no new external dependencies (constitution constraint)
**Storage**: `plugins.cfg` — plain text file in repo root, version-controlled; generated `salamand.gen.slnf` — build artifact, gitignored
**Testing**: No automated test framework in repo — verification matrix in `quickstart.md` (build flavors × config states, 5 validation error classes, output/plugins.ver audit, app launch + Plugin Manager check)
**Target Platform**: Windows 11+, VS2022 x64 build (CI additionally compiles the full solution incl. disabled plugins)
**Project Type**: Desktop-app build pipeline + repository cleanup
**Performance Goals**: Config validation + filter generation adds < 2 s per build; skipping 10 plugin+lang projects shortens full builds measurably (~20 fewer of 90 projects)
**Constraints**: `OPENSAL_BUILD_DIR`-relative outputs, no hardcoded paths, no manual steps, GPLv2-compatible only, UTF-8-BOM for sources
**Scale/Scope**: salamand.sln 91 → 77 project entries; 28 config entries; ~15 core-source files touched for removal references; help/translations cleanup for 8 plugins

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Assessment |
|---|---|
| I. Build Reproducibility | **PASS** — `plugins.cfg` is committed, so identical source revision ⇒ identical plugin set. Build stays single-command; the `.slnf` is generated deterministically from `plugins.cfg` + `salamand.sln` during the build; no manual steps introduced. |
| II. Backward Compatibility | **PASS with notes** — removal is an explicit, documented deprecation (architecture/10 analysis + this spec). Upgrading users: the 8 plugins are added to the existing silent-uninstall suppress list (`plugins2.cpp`) so no error dialogs appear. Known limitation (documented): pre-existing user registry entries for the "PAK (Plugin)" custom packer are not purged — see research.md R7 for why index-shift risk makes purging more dangerous than leaving them. |
| III. Incremental Modernization | **PASS** — core edits are strictly scoped to references of removed plugins; no adjacent refactoring. Build-script change is additive (new helper + hook points in build.cmd). |
| IV. Windows Platform Commitment | **PASS** — Batch + PowerShell 5.1 + MSBuild only; all already required by the toolchain. |
| V. Plugin Architecture Preservation | **PASS with justification** — the plugin system is preserved and *improved* (policy mechanism). winscp's missing proprietary dependency is resolved by removal rather than an open-source replacement: the plugin is a wrapper around WinSCP built with Embarcadero RTL that cannot ship under GPLv2, and the maintenance analysis (architecture/10) recommends removal; FTP plugin remains the supported remote-access path. Disabled plugins remain in the solution, so CI still compiles them (no bit-rot). |

**Post-Phase-1 re-check**: PASS — design artifacts (contracts, data model) introduce no new violations; the `.slnf` mechanism avoids touching 56 plugin project files (III), and the config contract keeps the build one-command (I).

## Project Structure

### Documentation (this feature)

```text
specs/007-plugin-build-policy/
├── plan.md              # This file
├── research.md          # Phase 0 output — decisions R1–R8
├── data-model.md        # Phase 1 output — config/plugin/output entities
├── quickstart.md        # Phase 1 output — usage + verification matrix
├── contracts/
│   ├── plugins-cfg.md   # plugins.cfg file-format contract
│   └── build-cmd.md     # build.cmd behavior contract (validation, reconciliation, exit codes)
└── tasks.md             # Phase 2 output (/speckit.tasks — NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
plugins.cfg                          # NEW — root plugin policy (28 entries: 18 on, 10 off)
build.cmd                            # MODIFIED — call helper before MSBuild; build .slnf; reconcile
                                     #   outputs; drop ieviewer CSS copy; make automation/zip data
                                     #   copies conditional on enabled state; update notes/help text
.gitignore                           # MODIFIED — ignore src/vcxproj/salamand.gen.slnf
src/vcxproj/
├── salamand.sln                     # MODIFIED — remove 14 project entries (+config/nesting lines)
├── gen_plugins_filter.ps1           # NEW — validate plugins.cfg, emit salamand.gen.slnf,
│                                    #   reconcile output plugins dir + plugins.ver
└── salamand.gen.slnf                # GENERATED at build time (gitignored)
src/plugins/{pak,unarj,unlha,unfat,wmobile,ieviewer,splitcbn,winscp}/   # DELETED
src/                                 # MODIFIED core references:
├── plugins2.cpp                     #   drop PAK+IEViewer from standard-plugin table; drop
│                                    #   winscp.spl special case; add 8 names to silent-uninstall
│                                    #   suppress list
├── pack3.cpp, packers.cpp           #   drop "pak" default archiver/packer additions (R7)
├── dialogs4.cpp, dialogs5.cpp,      #   remove FIXME_X64_WINSCP special-casing (dead x86-only
│   mainwnd2.cpp, cfgdlg.h, pwdmngr.h#   winscp machinery)
└── lang/lang.rc                     #   reword two Master Password texts mentioning WinSCP
help/src/
├── compileall.bat                   # MODIFIED — remove 8 plugins from help compile list
├── salamand.hhp / salamand.hhc      # MODIFIED — remove merged .chm entries + TOC sections
└── hh/salamand/*.htm                # MODIFIED — pages referencing removed plugins
translations/<lang>/                 # MODIFIED — delete {8 plugins}.slt where present
tools/comments/*.py                  # MODIFIED — remove 8 names from plugin lists
CLAUDE.md, architecture/*.md         # MODIFIED — counts, catalog, build docs (FR-012)
```

**Structure Decision**: Single-repo build-pipeline feature. The policy
mechanism concentrates in three files (`plugins.cfg`,
`gen_plugins_filter.ps1`, `build.cmd`); everything else is reference
cleanup driven by the removal list. No new projects, no new external
interfaces beyond the two documented contracts.

## Complexity Tracking

No constitution violations — table not needed. Two consciously
accepted limitations, documented in research.md:

| Limitation | Rationale |
|---|---|
| Existing user registry entries for the "PAK (Plugin)" custom packer are left in place (R7) | Purging risks index-coupling corruption in versioned config tables; entries degrade gracefully (plugin-missing message only if invoked). Principle II favors not touching user data. |
| `src/vcxproj/build.cmd` / `rebuild.cmd` developer conveniences and CI build the unfiltered solution | Per clarification: the config governs the product pipeline (root `build.cmd`). CI compiling disabled plugins is a feature (keeps them healthy for re-enabling). |
