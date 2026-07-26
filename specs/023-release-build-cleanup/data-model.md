# Phase 1 Data Model: Clean Release Build Output

**Feature**: 023-release-build-cleanup

This feature has no application data entities. The relevant "entities" are the
MSBuild build properties and output-tree locations the change governs.

## Build properties

| Property | Meaning | Before | After (Release) |
|----------|---------|--------|-----------------|
| `OutDir` | Final output directory of a project | `…\newtcommander\Release_x64\…` | **unchanged** |
| `IntDir` | Intermediate (obj/PCH/tlog) directory | `$(OutDir)Intermediate\…` (inside the output tree) | `…\obj\Release_x64\…\Intermediate\…` (outside the output tree) |
| `Configuration` | Build configuration | — | condition key: change applies only when `Release` |
| `ShortPlatform` | `x64` / `x86` | — | part of both output and obj roots |

**Invariant**: `OutDir` (and therefore every runtime deliverable) is never moved;
only `IntDir` is relocated, and only for Release.

## Output-tree locations

| Location | Kind | Disposition (Release) |
|----------|------|-----------------------|
| `salamand.exe`, `salamand.pdb` (root) | deliverable | preserved |
| `lang\english.slg` | deliverable | preserved |
| `plugins\<name>\*.spl`, `plugins\<name>\lang\english.slg`, `plugins.ver` | deliverable | preserved |
| `convert\`, `toolbars\`, `utils\` | runtime data | preserved |
| `Intermediate\` (any level) | scaffolding | **relocated to `obj\` + swept from tree** |
| `plugins\Intermediate\` (salmon/sqlite staging) | scaffolding | **relocated to `obj\` + swept from tree** |
| `saltests\` | test artifact | **not built in Release + swept from tree** |

## State transitions (per Release build)

1. MSBuild compiles each salamander-tree project → objects/PCH written under the
   relocated `obj\Release_x64\…` root (outside the output tree).
2. `saltests` is not selected for the Release solution build → no `saltests\`
   produced.
3. `build.cmd` post-build sweep removes any residual `Intermediate` directories
   and `saltests\` left in the output tree from earlier builds.
4. Result: output tree contains only deliverables + runtime data.
