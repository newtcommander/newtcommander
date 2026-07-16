# Validation Results: Plugin Build Policy (feature 007)

**Date**: 2026-07-16
**Environment**: Windows 11, VS2022 Community (MSBuild 17.10.4), Debug/Release x64, `OPENSAL_BUILD_DIR` defaulted to `.\build\`
**Baseline (T001)**: pre-change `build.cmd full` — BUILD SUCCEEDED, 35 plugins in plugins.ver, 36 language modules.

## Verification matrix (quickstart.md)

| # | Check | Result | Evidence |
|---|-------|--------|----------|
| 1 | Clean full build, default policy | ✅ PASS | `build.cmd rebuild full` + full: BUILD SUCCEEDED; summary `Plugin policy: 18 plugins enabled`; output `plugins\` = exactly 18 dirs; plugins.ver = 18 entries; 19 lang modules (18 plugin + 1 main app) |
| 2 | No functional references to the 8 removed plugins | ✅ PASS | repo-wide `git grep` clean; remaining mentions are historical (`introduction_news.htm` release notes), provenance comments (`ftp/sockets.h`, `checkver/data.cpp`), and docs describing the removal itself |
| 3 | `renamer=off` → incremental `build.cmd` | ✅ PASS | `renamer.vcxproj` absent from MSBuild log; output dir deleted by reconcile; plugins.ver filtered to 17 |
| 4 | `renamer=on` → incremental `build.cmd` | ✅ PASS | recompiled, output dir + `.spl` back, plugins.ver = 18 |
| 5 | Same toggle honored by `rebuild`, `full`, `full release` | ✅ PASS | rebuild: not compiled, dir absent; full: ver = 17; full release: Release_x64 has no renamer, ver = 17 |
| 6 | Missing `plugins.cfg` (V1) | ✅ PASS | exit 1 before MSBuild; `ERROR: plugins.cfg not found at '...'` names path + purpose |
| 7 | Unknown entry `bogus=on` (V3) | ✅ PASS | exit 1 pre-MSBuild; `ERROR: plugins.cfg line 42: unknown plugin 'bogus' - no directory src\plugins\bogus exists.` |
| 8 | Duplicate `zip=on` (V4) | ✅ PASS | exit 1 pre-MSBuild; `ERROR: plugins.cfg line 42: duplicate entry 'zip' ...` |
| 9 | Unlisted plugin (`tar=` line removed) (V5) | ✅ PASS | exit 1 pre-MSBuild; `ERROR: plugins.cfg: plugin directory src\plugins\tar has no entry - add 'tar=on' or 'tar=off'.` |
| 10 | Syntax error `tar=maybe` (V2) | ✅ PASS | exit 1 pre-MSBuild; `ERROR: plugins.cfg line 33: cannot parse 'tar=maybe' ...` (plus the consequent V5 for tar — all errors reported in one pass) |
| 11 | Case-insensitive entry `ZIP=ON` | ✅ PASS | build succeeds, 18 enabled, `zip\zip.spl` present and registered |
| 12 | Launch with default policy | ✅ PASS* | no dialogs related to disabled/removed plugins; after exit the registry lists 17 plugins — see deviation D1 (unrar) |
| 13 | Launch over an upgrade profile (7 removed plugins registered) | ✅ PASS | zero dialogs about removed plugins (silent-uninstall suppress list); after exit all 7 unregistered, 27 remained at that stage |
| 14 | All 28 entries `off` | ✅ PASS | build succeeds (`Plugin policy: 0 plugins enabled`, plugins.ver generation skipped gracefully); app launches and runs with an empty plugin set, no dialogs |

## Success criteria

- **SC-001** ✅ — full build ships exactly 18 plugins, none of the 8 removed / 10 disabled present.
- **SC-002** ✅ — zero functional references to the 8 names (historical changelog mentions and provenance comments retained per spec).
- **SC-003** ✅ — one-line toggle; the very next run of any flavor reflects it (verified for incremental, rebuild, full, full release).
- **SC-004** ✅ — all five error classes stop before compilation with file/line/entry named.
- **SC-005** ✅* — zero plugin-related error dialogs caused by this feature; see D1.
- **SC-006** ✅ — all documented flavors honor the config.

## Deviations / notes

- **D1 (pre-existing, unrelated to 007)**: the `unrar` plugin is enabled by policy but its runtime dependency `unrar.dll` is not in the repository (documented in CLAUDE.md / architecture/04). On first launch after a build it shows its own "UnRAR" load-error dialog and does not register, so the Plugin Manager lists 17 of the 18 enabled plugins. Identical behavior existed before this feature (34 of 35). Resolving unrar.dll is out of scope for 007 (constitution V lists it as a standing task).
- **D2 (formatting)**: `normalize.ps1` requires PowerShell 7.4, which is not installed on this machine; the VS-bundled clang-format 17.0.3 disagrees with the repository's formatting baseline on lines unrelated to this change (trailing-comment alignment), so whole-file reformatting was deliberately skipped to keep diffs minimal. All edited hunks were reviewed to match surrounding style; dangling multi-line conditions left by `#ifdef` block removal were collapsed manually.
- **D3**: `src/vcxproj/build.cmd` and `rebuild.cmd` developer conveniences and the CI workflow build the full unfiltered solution by design (clarification Q2) — disabled plugins stay compile-covered.
- The interactive launch checks were performed programmatically (window enumeration for `#32770` dialogs, registry inspection of `HKCU\Software\Open Salamander\5.0\Plugins` before/after). A final interactive confirmation of the Plugin Manager list by a human remains advisable.
