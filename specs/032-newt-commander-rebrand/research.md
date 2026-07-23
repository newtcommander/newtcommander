# Research: Newt Commander Application Rebrand

**Feature**: 032-newt-commander-rebrand | **Date**: 2026-07-23
**Sources**: two codebase inventory passes (branding surfaces; rendering/version/build pipelines), tool-availability probe, icon-rasterizer feasibility spike.

## R1 — Executable rename mechanism

**Decision**: Add `<TargetName>newtcommander</TargetName>` to `src/vcxproj/salamand.vcxproj` (project file name stays `salamand.vcxproj` per FR-016). Update every functional `salamand.exe` literal.

**Rationale**: MSBuild derives `salamand.exe` from the project name only because no `TargetName` is set. `build.cmd` never names the exe (verified — output name is inherited), so a single project-level property renames the binary without touching the build scripts or solution.

**Affected literals** (functional, must follow the rename):
- `src/salmon/salmon.cpp:331` — crash reporter relaunch path
- `src/translator/restart.cpp:51` — process detection (dev tool, cheap to fix)
- `src/plugins/filecomp/fcremote/fcremote.cpp:234` — remote-compare launcher
- `src/plugins/shared/vcxproj/x86.props:8`, `x64.props:8` — `<SalPath>` debug macro
- `src/lang/texts.rc2:1323,1855,1856` — user-visible usage strings
- `src/versinfo.rh2:29-30` — `VERSINFO_INTERNAL`/`VERSINFO_ORIGINAL` → `NEWTCOMMANDER`/`NEWTCOMMANDER.EXE`

**Alternatives considered**: renaming the vcxproj/solution — rejected (violates FR-016 no-source-rename rule; large churn in solution filters and build scripts).

## R2 — Version identity 0.1.0

**Decision**: Set `VERSINFO_SALAMANDER_MAJOR 0`, `MINORA 1`, `MINORB 0` in `src/plugins/shared/spl_vers.h` and change the version-string composition to always emit all three components (today the `#if (MINORB == 0)` branch collapses "0.1.0" to "0.1"). Keep `VERSINFO_BUILDNUMBER 184` (continues monotonically per D07). Numeric tuples become `0,1,0,184`. Update `src/manifest.xml` to `version="0.1.0.0"`, identity `name="NewtCommander.NewtCommander"`, description "Newt Commander File Manager".

**Rationale**: The user fixed the version as 0.1.0 (three components); the display macro chain must not drop the trailing zero. Build number continuity avoids breaking any build-comparison logic and preserves history.

**Plugin ABI**: `LAST_VERSION_OF_SALAMANDER 104` unchanged (FR-015, D10). The user-visible refusal string `REQUIRE_LAST_VERSION_OF_SALAMANDER` is reworded to name "Newt Commander 0.1.0".

## R3 — Icon asset generation (no external tools)

**Decision**: Pure-Python (Pillow 12.1.1, available on this machine) rasterizer that redraws the icon's geometry (rounded rects + 2 linear + 1 radial gradient) supersampled 8×, downscaled with Lanczos; a manual ICO packer emits 32-bpp BMP entries ≤ 64 px and PNG entries ≥ 128 px. **Feasibility spike passed** — 256/32/16 px outputs visually verified.

Variants per `temp/visual_style/README.txt` (files the README references but the package lacks are generated):
- full (≥ 48 px): radial disc light, plate edges, gradient plates, rows
- simplified (24–48 px): flat colors `#3B82F6`/`#F97316`, flat navy disc, rows kept
- favicon (16 px): flat, no rows, wider centre gap

**Deliverables**: `src/res/salamand.ico` replaced in place (file name kept — referenced by `salamand.rc2:19` and `salamand.vcxproj:906`) with entries 16/24/32/48/64/128/256 px; `sal_r/g/b.ico` replaced with state-tinted variants (16+32 px, both plates tinted red/green/blue per D16). Generator script + authored SVG variants committed under `tools/brand/` for reproducibility (Constitution I).

**Alternatives considered**: ImageMagick/Inkscape/resvg — not installed; browser-based rendering — needless indirection given the icon's simple geometry.

## R4 — About dialog & splash screen redesign

**Facts**: Rendering is nanosvg (`src/svg.cpp`) rasterizing RCDATA SVGs; **nanosvg ignores `<text>` elements**, so the lockup's live-text wordmark cannot ship as SVG text. About/splash are currently hardcoded light (white bg, fixed RGB text colors in `src/logo.cpp`), loaded with `SVGSTATE_ORIGINAL`; feature 028's `IsDarkThemeActive()` (`src/themes.h:23`) is not consulted.

**Decision**:
- `res/logo.svg` (salamander hand) → replaced by the new icon SVG (nanosvg-compatible primitives only; the provided icon uses rect/rx + linear/radial gradients, all supported).
- `res/os.svg` (wordmark) → retired from use; the product name is drawn by the app with GDI ("Newt" + "Commander" in the two lockup colors, Segoe UI bold — always present on Windows 11, satisfies D18's no-font-dependency rule with zero conversion tooling).
- `res/gradspl.svg` / `res/gradabt.svg` → replaced by a brand gradient band (blue `#3B82F6` → orange `#F97316`).
- `logo.cpp` gains `#include "themes.h"` and `IsDarkThemeActive()` branching: dark = navy `#0A1424` background with light text (`#EAF2FB` / tagline `#8FA6C4`), light = white background with `#0A1424` text and `#EA6A0B` accent — exact lockup palette (D17-A).
- About web link → `https://newtcommander.org`; copyright statics → year-split rule (FR-017).

**Alternatives considered**: converting Archivo text to SVG paths (needs font tooling not present; heavier); shipping PNG wordmark (not theme-adaptive, scaling artifacts on high DPI).

## R5 — Registry root & legacy import removal

**Decision**: `SalamanderConfigurationRoots` becomes a single entry `"Software\\Newt Commander\\0.1"` with parallel `SalamanderConfigurationVersions` `{"0.1"}`; `SALCFG_ROOTS_COUNT` (`src/consts.h:2096`) 83 → 1.

**Rationale/verified behavior**: All import machinery (`FindLatestConfiguration`, `FindLanguageFromPrevVerOfSal`, `DeleteOldConfigurations`, `CImportConfigDialog`) iterates roots 1..N-1 ("older than current"); with a single root these loops are empty, so the import UI can never appear — which implements FR-010's "mechanism removed" with the smallest possible diff (Constitution III). No fixed-size assumption breaks as long as both parallel arrays stay consistent (`ConfigurationExist[SALCFG_ROOTS_COUNT]` sizes itself from the constant).

**Other registry keys**: `Software\Open Salamander\Bug Reporter` → `Software\Newt Commander\Bug Reporter` (`src/salmoncl.cpp:39`, `src/salmon/config.cpp:11`). Dev-tool keys (Trace Server, Translator) stay per D24 (not shipped to end users).

## R6 — IPC & discovery namespace

**Decision** (new canonical names in `contracts/branding-identity.md`):
- `AltapSalamander3bProcessList` → `NewtCommander01ProcessList` (+ its `Mutex`/`Event`/`EventProcessed` variants where mirrored)
- `AltapSalamanderFirstInstance` → `NewtCommanderFirstInstance`
- `AltapSalamanderLoadSaveRegistry` → `NewtCommanderLoadSaveRegistry`
- `SalamanderMainWindowVer25` → `NewtCommanderMainWindowVer01` (FindWindow single-instance discovery)
- salmon: `AltapSalamanderSalmonMainDialog` → `NewtCommanderSalmonMainDialog`; `Global\AltapSalamanderBugReporterRegistryMutex` → `Global\NewtCommanderBugReporterRegistryMutex`

Process-local window classes (`SalamanderSaveBits`, `SalamanderItemsBox`, …) are not cross-process discoverable and stay unchanged (minimal diff). A mirror copy of the tasklist constants reportedly exists in a `salbreak` tool — verify at implementation time and keep mirrors in sync if the file exists.

## R7 — Shell extension identity

**Decision**: New CLSID **`{A6D5A8E2-D69F-4E03-8396-781909E7A3AE}`** (generated for this feature) replaces `{c78b614f-f3ea-11d2-94a1-00e0292a01e3}`; registration name `OpenSalamanderVer500` → `NewtCommanderVer010` (`SALSHEXT_SHAREDNAMESAPPENDIX` "500" → "010"); shared objects `SalExten_SharedMem4`/`SalExten_SharedMemMutex4`/`SalExten_DoPasteEvent4` → `NCExten_SharedMem1`/`NCExten_SharedMemMutex1`/`NCExten_DoPasteEvent1`; description and `shellext.rc` metadata rebranded. DLL file names (`salextx86/x64.dll`) stay per D05.

## R8 — Crash reporter (salmon)

**Decision**: Disable the upload path entirely (Q2): the reporter keeps writing minidumps locally under `%APPDATA%\Newt Commander` (`src/salmoncl.cpp:117` folder string rebranded) but the network upload to `reports.altap.cz` (`src/salmon/upload.cpp`) is compiled out/neutered and any "send report" UI affordance is removed or repointed to the GitHub issues URL. `APP_NAME` → "Newt Commander Bug Reporter"; `salmon.rc` + `manifest.xml` metadata rebranded. Exact neuter point (call-site vs. function body) chosen at implementation after reading salmon's dialog flow.

## R9 — Remaining user-visible strings & URLs

- `MAINWINDOW_NAME` → `"Newt Commander"` (`src/salamdr1.cpp:216`); `SALAMANDER_TEXT_VERSION` → `"Newt Commander " VERSINFO_VERSION` (`src/mainwnd1.cpp:27`). Title/tray composition needs no other change (verified composition sites).
- `src/lang/lang.rc`: 22 occurrences of "Open Salamander" → rebrand; About block gets year-split copyright + `newtcommander.org`.
- Main-app literals (~15 sites: `callstk.cpp:748`, `dialogs3.cpp:2601`, `salamdr5.cpp:1856/1866`, `salamdr1.cpp:108/4031/4639`, …) → contextual rebrand.
- Help-menu / dialog URLs (`mainwnd3.cpp:2565,2571`, `dialogs.cpp:2078-2080`, `dialogs2.cpp:1086`, `logo.cpp:424`) → `https://newtcommander.org` or `https://github.com/newtcommander/newtcommander` (Issues replaces forum); dead vendor pages removed with their menu items where no equivalent exists.
- Shipped-plugin literals: `pictview/pvtwain.cpp:43-44`, `ftp/ftp2.cpp:887` → rebrand. `checkver` (phones `altap.cz`) is already **off** in `plugins.cfg` — SC-006 holds without touching it; its strings are left to the deferred translations/marginal-plugins follow-up.
- `mdview` WebView2 cache dir `…\Open Salamander\mdview.WebView2` (`src/plugins/mdview/viewer.cpp:74`) → `…\Newt Commander\mdview.WebView2`.

## R10 — Version-resource metadata (app + plugins)

- `src/versinfo.rh2`: COMPANY → "Newt Commander Project", DESCRIPTION → "Newt Commander, File Manager", COPYRIGHT → year-split string, SLG_WEB → `newtcommander.org`, COMMENT rebranded.
- Shared `src/plugins/shared/versinfo.rc2:59`: hardcoded ProductName → `"Newt Commander\0"` (single edit rebrands every plugin's ProductName).
- Per-plugin `versinfo.rh2` overrides: **sftp**, **mdview** → `Copyright © 2026 Newt Commander Authors` (FR-021 sole attribution); **pictview** → `Copyright © 2000-2026 Open Salamander Authors, © 2026 Newt Commander Authors`; remaining shipped plugins → year-split with their original start year; CompanyName → "Newt Commander Project" everywhere.
- `src/shellext/shellext.rc`, `src/salmon/salmon.rc`, `src/salmon/manifest.xml` metadata rebranded (FR-005).

## R11 — Governance artifacts

- Constitution: rename to "Newt Commander Constitution"; Principle II re-anchored to Newt Commander 0.1.0 baseline with the deliberate Open Salamander 5.0 break recorded (MAJOR bump → 2.0.0 per its own versioning policy).
- `README.md`: FR-015-of-030 caveat replaced by completed-rebrand statement; `CLAUDE.md`: project identity updated (name, exe, registry root, version).
- `architecture/` docs: left as historical analysis (D26-A); a header note added only where they'd actively mislead (not a bulk rewrite).

## Confirmed non-issues

- `build.cmd` needs no changes for the rename; `full` ships no help/ content and no vendor-branded extra artifacts beyond those addressed above.
- Setup/remove sources are untouched (Q3 deferral); nothing from `src/setup` ships in the default build.
- No `salopen.exe`-style launcher exists; single-instance is in-process (`FindWindow` + mutexes covered by R6).
- Existing `salamand.ico` is 8-entry (16/32/48 ×8bpp+32bpp, 64, 256); the replacement's 32-bpp 16–256 set is a superset of what Windows 11 consumes.
