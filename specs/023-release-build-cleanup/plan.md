# Implementation Plan: Clean Release Build Output (No Intermediate / saltests Directories)

**Branch**: `023-release-build-cleanup` | **Date**: 2026-07-19 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/023-release-build-cleanup/spec.md`

## Summary

The Release output tree (`…\salamander\Release_x64\`) is polluted with build
scaffolding — `Intermediate\` directories at every level plus the `saltests\`
test-binary directory — none of which belong in a shippable release. This plan
keeps that tree clean by: (1) relocating every salamander-tree project's `IntDir`
to a sibling `obj\` root outside the output tree, for **Release only**, via a
single central `src/Directory.Build.targets`; (2) excluding the `saltests`
project from the Release solution build; and (3) adding a Release-only post-build
sweep in `build.cmd` that removes any residual `Intermediate`/`saltests` folders
from the tree (for incremental builds over pre-existing output). Relocation (not
deletion) keeps incremental Release builds fast; Debug is left entirely unchanged.

The core mechanism (central `Directory.Build.targets` overriding `IntDir`) was
verified empirically with `MSBuild -getProperty:IntDir` before writing this plan
— see [research.md](./research.md).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`) — but this feature touches **no C++ code**; it is a build-system change (MSBuild + batch).
**Primary Dependencies**: MSBuild (VS2022 v143), MSBuild property evaluation (`Directory.Build.targets`, `String.Replace` property function); `build.cmd` (Windows batch) + PowerShell for the sweep.
**Storage**: N/A (filesystem output layout under `OPENSAL_BUILD_DIR`).
**Testing**: Manual verification via `build.cmd release` + PowerShell directory checks (see quickstart.md). MSBuild `-getProperty:IntDir` used to verify property overrides without a full build.
**Target Platform**: Windows 11+, x64 Release build.
**Project Type**: Desktop application build tooling (single repo, `src/` tree).
**Performance Goals**: No full recompilation on incremental Release builds (FR-005); build time otherwise unchanged.
**Constraints**: Debug output must be structurally unchanged (FR-004); all runtime deliverables preserved (FR-003); no hardcoded paths — obj root derives from `$(OPENSAL_BUILD_DIR)` (Constitution I).
**Scale/Scope**: 3 files changed, 0 C++ files. Affects all ~86 salamander-tree projects' Release intermediates via one central file.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Assessment |
|-----------|------------|
| **I. Build Reproducibility** | ✅ Reinforced. Build stays single-command; artifacts remain under `OPENSAL_BUILD_DIR`; the new `obj\` root is derived, not hardcoded; no manual steps. Cleaner separation of scaffolding from deliverables. |
| **II. Backward Compatibility** | ✅ No user-facing/runtime behavior change. All Release deliverables preserved; Debug unchanged; unit tests still build/run in Debug. |
| **III. Incremental Modernization** | ✅ Three small, independently reviewable/revertible changes; no adjacent refactoring; no big-bang. |
| **IV. Windows Platform Commitment** | ✅ Pure MSBuild/WinAPI toolchain (VS2022); no new dependencies or abstractions. |
| **V. Plugin Architecture Preservation** | ✅ Plugin `.spl`/`.slg`/`plugins.ver` outputs and `plugins.cfg` policy untouched; only intermediate locations move. |
| **VI. UI Consistency** | N/A — no UI. |

**Result: PASS** (initial and post-design). No violations; Complexity Tracking not required.

## Project Structure

### Documentation (this feature)

```text
specs/023-release-build-cleanup/
├── plan.md              # This file
├── research.md          # Phase 0 output (decisions + empirical IntDir verification)
├── data-model.md        # Phase 1 output (build properties & output-tree locations)
├── quickstart.md        # Phase 1 output (build + acceptance checks)
├── checklists/
│   └── requirements.md   # Spec quality checklist (from /speckit.specify)
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

No `contracts/` directory: this is internal build tooling with no external
interface to contract.

### Source Code (repository root)

Files changed by this feature:

```text
src/
└── Directory.Build.targets      # NEW — Release-only IntDir relocation (central)

src/vcxproj/
├── salamand.sln                 # EDIT — drop saltests Release Build.0 mappings
build.cmd                        # EDIT — Release-only post-build sweep of the output tree
```

No changes to any `.cpp`/`.h`, to per-project `.vcxproj`/`.props`, or to
`plugins.cfg`.

**Structure Decision**: Single-repo desktop application. The change is confined
to build configuration at the `src/` tree root (`Directory.Build.targets`), the
solution file, and the root build script — the three natural leverage points for
output-layout policy. A central `Directory.Build.targets` is chosen over editing
~15 per-project property sheets because it is one reviewable file, is discovered
automatically by every project under `src/`, and (via a root-segment
`String.Replace`) is both collision-proof and self-scoping to the salamander
output tree.

## Complexity Tracking

No constitution violations — section intentionally empty.
