# Phase 0 Research: Plugin Build Policy

All spec-level unknowns were resolved in the clarification session
(format, scope, reconciliation timing, unlisted-plugin behavior).
This document records the implementation-level decisions and the
repository evidence they rest on.

## R1 — Config file name and location

**Decision**: `plugins.cfg` in the repository root.

**Rationale**: The spec fixes root placement and the `name=on|off`
syntax; `.cfg` matches the existing runtime-data naming precedent
(`convert.cfg`). Root placement makes the policy visible next to
`build.cmd`, its only consumer.

**Alternatives considered**: `src/vcxproj/plugins.cfg` (closer to the
solution, but hidden from the casual maintainer and contradicts the
spec's "repository root"); `plugins.txt` (weaker signal that the file
is machine-consumed configuration).

## R2 — Mechanism for skipping disabled plugins in the MSBuild build

**Decision**: Generate a **solution filter file**
(`src/vcxproj/salamand.gen.slnf`, gitignored) from `salamand.sln` +
`plugins.cfg`, and point the existing single MSBuild invocation at the
`.slnf` instead of the `.sln`. Inclusion rule: every solution project
whose path is **not** under `..\plugins\<name>\` for a disabled
plugin.

**Rationale**:
- MSBuild builds `.slnf` files natively since 16.7; VS2022 (required
  toolchain) qualifies. Targets `build`/`rebuild` work unchanged, and
  `/m` parallelism over the whole dependency graph is preserved —
  build.cmd keeps its single MSBuild call.
- Plugin projects reference their satellites via `ProjectReference`
  (verified: `7zip.vcxproj` references `7za.dll.vcxproj`,
  `7zwrapper.vcxproj`, `lang_7zip.vcxproj`), so including a plugin's
  main project transitively builds everything it needs even when a
  satellite were filtered out; conversely, excluding all projects under
  a disabled plugin's directory removes it completely because nothing
  else references it.
- Zero edits to any of the 56 plugin/lang `.vcxproj` files
  (constitution III), and the checked-in `salamand.sln` stays the
  single source of project truth for the IDE (clarification: disabled
  plugins must stay IDE-buildable).

**Alternatives considered**:
- `msbuild salamand.sln /t:proj1;proj2;...` target lists — fragile
  (project-name mangling for dots/dashes, must enumerate all 70+
  enabled projects, breaks `rebuild` semantics per target).
- Per-project msbuild invocations in a loop — loses cross-project
  scheduling, multiplies MSBuild startup cost, and reimplements
  dependency ordering that the solution already encodes.
- `Condition` attributes / early-exit targets injected into plugin
  `.vcxproj` or `plugin_base.props` — touches every plugin project,
  still runs MSBuild for skipped projects, pollutes the IDE experience
  (constitution III violation for no benefit).
- Committing a hand-maintained `.slnf` — drifts from `plugins.cfg`;
  the config must remain the single authority.

## R3 — Where validation and generation logic lives

**Decision**: One new PowerShell 5.1 script,
`src/vcxproj/gen_plugins_filter.ps1`, invoked by `build.cmd` before
MSBuild. It performs: config parsing, the five validation classes,
`.slnf` generation, and output reconciliation (R5). `build.cmd` aborts
on non-zero exit.

**Rationale**: Batch alone cannot reasonably express set comparison
(config entries × plugin directories, both directions), duplicate
detection, and JSON emission. `build.cmd` already shells out to
PowerShell (plugins.ver version token), so this adds no new toolchain
dependency (constitution I/IV). One script keeps the contract testable
in isolation and `build.cmd` a thin orchestrator.

**Alternatives considered**: pure-batch parsing (unmaintainable set
logic, poor error messages); an MSBuild targets file doing validation
(runs too late — spec requires failure *before any compilation*);
a compiled helper tool (new build-of-the-build bootstrap problem).

## R4 — Validation semantics (five error classes)

**Decision**: The helper fails the build (exit ≠ 0, message names
`plugins.cfg` and the offending line/entry) for: (1) missing config
file, (2) syntactically unreadable line (not `name=on|off`, not
comment/blank), (3) entry matching no `src/plugins/<name>` directory,
(4) duplicate entry for the same plugin, (5) plugin directory with no
entry. Name matching is case-insensitive; `shared` is infrastructure
and exempt from (5).

**Rationale**: Directly implements FR-009/FR-010 and SC-004 as
clarified (unlisted plugin = hard error). The plugin-directory set is
discovered by enumerating `src/plugins/*/` minus `shared`, so the
check self-adapts when future plugins are added.

**Alternatives considered**: warning-and-continue for unlisted plugins
— rejected in clarification Q4 (silent product gaps).

## R5 — Output reconciliation (stale binaries, plugins.ver)

**Decision**: On every `build.cmd` run, after successful validation
and *before* MSBuild, the helper deletes
`%OPENSAL_BUILD_DIR%newtcommander\<Config>_<Platform>\plugins\<name>\`
for every disabled plugin and for any leftover directory not matching
an existing plugin (covers the 8 removed ones). If a `plugins.ver`
exists in the output, its entry lines for non-enabled plugins are
dropped (header version kept). The `full` flavor then regenerates
`plugins.ver` as today — which stays correct automatically because it
scans the already-reconciled output tree.

**Rationale**: Implements the clarified "every run, any flavor"
guarantee (FR-006, SC-003). Doing it pre-build means a subsequent
successful compile can never resurrect a stale state, and the
existing `.spl`-scan design of plugins.ver generation needs no change.
plugins.ver auto-install semantics tolerate rewrites: entries with a
version ≤ the registry counter are skipped, and already-known plugins
are never re-installed (documented in build.cmd comments).

**Alternatives considered**: reconcile only in `full` builds
(contradicts clarification Q3); post-build reconcile (leaves a window
where a failed build keeps stale outputs).

## R6 — Handling upgrades of existing installations (removed plugins)

**Decision**: Add the 8 removed plugins' `<dir>\<name>.spl` paths to
the existing silent-uninstall suppress list in `plugins2.cpp`
(`...Data[i]->DLLName != "fsearch\\fsearch.spl" ...` block, currently
suppressing fsearch, eroiica, unace, diskcopy). Also remove the
`AddPlugin("PAK", ...)` and `AddPlugin("Internet Explorer Viewer",
...)` entries from the standard-plugins table and the
`winscp\winscp.spl` special case.

**Rationale**: Salamander already auto-uninstalls plugins whose `.spl`
vanished and the suppress list exists precisely to do it without
alarming the user — the established pattern for "plugin no longer
supported" (fsearch/eroiica/unace/diskcopy precedents). This satisfies
the spec's upgrade edge case with a minimal, idiomatic change.

**Alternatives considered**: new config-version migration that purges
registry keys — heavier, touches user data (principle II), and
redundant given the suppress-list mechanism.

## R7 — Default packer/archiver table entries for `pak`

**Decision**: Remove the `pak` additions from the versioned default
tables — `SetPacker(index, 3, "PAK (Plugin)", "pak", TRUE)` in
`packers.cpp` and `SetFormat(index, "pak", ...)` in `pack3.cpp` — so
fresh configurations and upgrades no longer create them. Do **not**
purge already-persisted entries from existing users' registry
configurations. Implementation must first verify how
`CPackerConfig`/format tables are persisted (by value vs by index):
if any stored data references these tables **by index**, entries may
only be removed from the *append* steps (the `case N:` upgrade
blocks), never in a way that renumbers earlier entries.

**Rationale**: The tables are cumulative `switch`-fallthrough upgrade
steps; deleting an append step is safe for fresh installs and for
upgrades that have not run it yet. Existing entries degrade gracefully
(a "PAK (Plugin)" custom packer only errors if actually invoked), and
deleting user-authored packer lists violates the backward-compatibility
principle more than a stale row does.

**Alternatives considered**: registry-purging upgrade step (risk of
index-shift corruption, touches user data); leaving default tables
untouched (fresh installs would keep offering a packer whose plugin
does not exist — violates FR-011 "not offered").

## R8 — CI and secondary build scripts

**Decision**: Leave `.github/workflows/pr-msbuild.yml` (builds
`salamand.sln` directly) and the developer conveniences
`src/vcxproj/build.cmd`/`rebuild.cmd` building the full solution.
Only the root `build.cmd` consumes `plugins.cfg`.

**Rationale**: Clarification Q2 scoped the policy to the scripted
product pipeline. CI compiling disabled plugins is desirable: it keeps
them warm for re-enabling (they stay in the solution deliberately).
Removed plugins disappear from the solution, so CI drops them
automatically.

**Alternatives considered**: making CI build the filtered `.slnf` —
would silently stop compiling disabled plugins and let them rot,
undermining the "disabled ≠ abandoned" policy intent.

## R7 Addendum — T006 investigation findings (2026-07-16)

**Persistence semantics verified** (`src/pack.h`, `src/pack3.cpp`,
`src/packers.cpp`, `src/plugins1.cpp`, `src/plugins2.cpp`):

1. `CPackerFormatConfig` entries store `PackerIndex`/`UnpackerIndex`
   **by value in each entry**: negative values encode plugin
   references (`-pluginIndex-1` into the runtime `Plugins` list),
   non-negative values index `ArchiverConfig` (external archivers).
   Custom packers/unpackers (`CPackerConfig`/`CUnpackerConfig`) use
   the same encoding in their `Type` field.
2. **Existing configs self-heal**: the startup check pass
   (`plugins2.cpp` ~2153) deletes format entries, custom
   packers/unpackers, and viewer masks whose plugin reference no
   longer resolves (`IsArchiveIndexOK`); `CPluginData::Remove`
   renumbers references when a plugin is uninstalled. Once `pak.spl`
   disappears and the plugin auto-uninstalls, all user-side "PAK
   (Plugin)" entries are pruned automatically — the "known limitation"
   in plan.md Complexity Tracking is in fact handled by existing
   machinery.
3. **Safe removal set** (no index renumbering — PAK and IEViewer are
   the 3rd/4th standard plugins; ZIP=0 → -1 and TAR=1 → -2 references
   stay valid):
   - `plugins2.cpp`: `AddPlugin("PAK", ...)` + `AddPlugin("Internet
     Explorer Viewer", ...)` in the standard table;
   - `packers.cpp` `CPackerConfig::AddDefault` case 2: PAK packer;
   - `packers.cpp` `CUnpackerConfig::AddDefault` case 2: PAK unpacker
     (~line 1033 — found during investigation, missing from original
     task description);
   - `pack3.cpp` `CPackerFormatConfig::AddDefault` case 2: "pak"
     format (−3/−3);
   - `mainwnd1.cpp` ~355: default viewer mask `*.htm;*.html;*.xml;
     *.mht` → ViewerType −4 (IEViewer) — additional site found;
   - `plugins2.cpp` old-config viewer conversion `case 1:` (old "IE
     viewer" type) → change from mapping to −4 to deleting the mask
     (IEViewer no longer exists; the check pass would otherwise
     mis-resolve −4 against arbitrary plugin index 3).
4. ARJ/LHA/UC2/ACE default formats reference **external archivers**
   (non-negative indexes), not the unarj/unlha plugins — no default
   table changes needed for the other removed plugins; their
   associations were registered dynamically and self-heal per (2).

**Conclusion**: full removal per T007 is safe; proceed with deletion
(no index-coupled blockers). Old-type conversion switches
(`case 3: → -3` for PAK) are left intact — they become unreachable
for fresh configs and still correctly convert ancient configs, whose
dead references the check pass then prunes.
