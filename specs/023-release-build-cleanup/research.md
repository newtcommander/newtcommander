# Phase 0 Research: Clean Release Build Output

**Feature**: 023-release-build-cleanup
**Date**: 2026-07-19

## Problem restatement

The Release output tree `…\salamander\Release_x64\` currently contains build
scaffolding that is not part of a shippable release:

- **`Intermediate` directories** at many levels (verified in the current output):
  - top-level `Intermediate\` (main app objects)
  - `lang\Intermediate\` (app language module)
  - `plugins\<name>\Intermediate\` and `plugins\<name>\lang\Intermediate\` for every built plugin
  - `plugins\Intermediate\salmon\Intermediate\` and `plugins\Intermediate\sqlite\Intermediate\` (the `plugins\Intermediate\` folder exists **only** because the `salmon` and `sqlite` helper projects put their intermediates there; their final binaries live in `utils\`)
  - `saltests\Intermediate\`
- **`saltests\`** — output of the unit-test project (`saltests.exe` + intermediates).

`salopen`, `salspawn`, and `shellext` were checked and are **not** produced in
the default Release build (not in the output), so they need no handling.

## Key decisions

### D1 — Relocate intermediates instead of deleting them

**Decision**: For the Release configuration, redirect every project's `IntDir`
to a sibling `obj\` root **outside** the shipped output tree, rather than
deleting `Intermediate` directories after the build.

**Rationale**: Deleting the object/PCH cache after each build would force a full
recompilation on the next incremental Release build, violating FR-005.
Relocation moves the cache out of the output tree while keeping it persistent,
so incremental builds stay fast.

**Alternatives considered**:
- *Post-build delete of `Intermediate`*: simplest, but breaks incremental builds (rejected by FR-005).
- *Per-release-property-sheet edits* (add `IntDir` override to every `*_release.props` and each sub-library props): ~15+ files, error-prone, easy to miss one; rejected in favor of a single central override.

### D2 — Central `Directory.Build.targets`, using a root-segment rewrite

**Decision**: Add one file `src/Directory.Build.targets` that, for Release only,
sets:
`IntDir = $(IntDir.Replace('salamander\$(Configuration)_$(ShortPlatform)\', 'obj\$(Configuration)_$(ShortPlatform)\'))`.

**Rationale**:
- MSBuild imports `Directory.Build.targets` **after** every project's property
  sheets (via `Microsoft.Cpp.targets` → `Microsoft.Common.targets`), so this
  assignment wins over the `$(OutDir)Intermediate\` value set in the `*.props`.
- Rewriting only the **root segment** (`salamander\<cfg>_<plat>\` → `obj\<cfg>_<plat>\`)
  preserves each project's existing, already-unique relative sub-path. This is
  collision-proof — important because project **names are not unique**
  (`portables` and `lang_portables` each appear twice), so a naive
  `obj\$(MSBuildProjectName)\` scheme would collide.
- `String.Replace` is a **no-op when the token is absent**, so projects whose
  intermediates are *not* under the salamander output tree (the `setup\`
  projects, whose `OutDir` is `$(OPENSAL_BUILD_DIR)$(ProjectName)\…`) are left
  completely untouched — the change is surgically scoped to the salamander tree.
- All projects live under `src/`, so a single `src/Directory.Build.targets` is
  discovered by every one of them. No `Directory.Build.props` exists in the repo
  today, so there is nothing to conflict with.

**Empirical verification** (via `MSBuild <proj> -getProperty:IntDir`):

| Project | Config | Resulting IntDir |
|---------|--------|------------------|
| `salamand` | Release x64 | `obj\Release_x64\Intermediate\` ✅ relocated |
| `zip` (plugin) | Release x64 | `obj\Release_x64\plugins\zip\Intermediate\` ✅ |
| `lang_zip` | Release x64 | `obj\Release_x64\plugins\zip\lang\Intermediate\` ✅ |
| `salmon` | Release x64 | `obj\Release_x64\…\plugins\Intermediate\salmon\Intermediate\` ✅ (outside tree) |
| `saltests` | Release x64 | `obj\Release_x64\saltests\Intermediate\` ✅ |
| `setup` | Release x64 | `setup\Release_x64\Intermediate\` ✅ **unchanged** (no-op) |
| `salamand` | **Debug** x64 | `salamander\Debug_x64\Intermediate\` ✅ **unchanged** |
| `zip` | **Debug** x64 | `salamander\Debug_x64\plugins\zip\Intermediate\` ✅ **unchanged** |

The mechanism works exactly as designed and does not touch Debug.

### D3 — Exclude `saltests` from the Release solution build

**Decision**: Remove the two Release `Build.0` mappings for the `saltests`
project GUID from `src\vcxproj\salamand.sln` (keep the `ActiveCfg` lines and all
Debug mappings).

**Rationale**: `saltests` is a unit-test project — a development/CI asset, not a
shipping deliverable ("not needed for release"). Dropping its `Release|x64` and
`Release|Win32` `Build.0` lines is the standard MSBuild way to exclude a project
from a solution configuration while keeping it fully buildable in Debug (FR-007).
The generated solution filter (`salamand.gen.slnf`) references the `.sln` and
inherits its configuration→build mapping, so the filtered Release build stops
building `saltests`. Because `saltests` is not a plugin, `gen_plugins_filter.ps1`
never removes it from the filter — the `.sln` mapping is the right lever.

**Alternatives considered**:
- *Relocate `saltests` OutDir for Release* (like the intermediates): keeps the
  output tree clean but still compiles the tests in Release and leaves a stray
  `saltests.exe` in `obj\`. Rejected — excluding the build is cleaner and honors
  "not needed for release" literally.

### D4 — Release-only post-build sweep in `build.cmd` (robustness / migration)

**Decision**: After a successful **Release** build, `build.cmd` removes any
residual `Intermediate` directories (recursively) and the `saltests` directory
from the output tree.

**Rationale**: D2/D3 mean a clean rebuild already produces a clean tree. But an
**incremental** build over an already-populated tree (every current build tree
has the old `Intermediate` dirs and `saltests\`) leaves those stale directories
behind, because MSBuild now writes elsewhere and no longer builds `saltests`.
The sweep guarantees SC-001/SC-002 and FR-008 for *any* build mode and migrates
existing trees on the first run. It is safe for incremental builds (FR-005):
after D2 the live object/PCH cache lives under `obj\`, **outside** the swept
output tree, so the sweep only ever removes stale/empty scaffolding — never the
live cache. It runs for Release only, so Debug keeps its `Intermediate` (FR-004).

## Constraints honored

- **Constitution I (Build Reproducibility)**: single-command build unchanged;
  artifacts still under `OPENSAL_BUILD_DIR`; no hardcoded paths (the `obj\` root
  derives from `$(OPENSAL_BUILD_DIR)`); no manual steps.
- **Constitution III (Incremental Modernization)**: three small, independently
  reviewable/revertible changes; no adjacent refactoring.
- **Backward compatibility**: Debug output byte-for-byte structurally unchanged;
  all runtime deliverables unchanged.

## Open questions

None. All decisions validated against the current build and, for D2, empirically
against MSBuild's evaluated `IntDir`.
