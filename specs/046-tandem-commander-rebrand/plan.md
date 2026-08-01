# Implementation Plan: Tandem Commander Rebrand

**Branch**: `046-tandem-commander-rebrand` | **Date**: 2026-08-01 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/046-tandem-commander-rebrand/spec.md`

## Summary

Rename the whole product identity from **Newt Commander** (`newtcommander.exe`)
to **Tandem Commander** (`tandemcommander.exe`) and apply the new visual brand
from `temp/tandem_design/`. The change is mechanical but wide (~392 tracked
files) and touches seven distinct subsystems: central version-info defines,
hardcoded C++ identity strings (registry root, kernel-object names, window
class, per-user paths), resource scripts, the MSBuild output tree (with a
lockstep `Directory.Build.targets` string-match hazard), the Inno Setup
installer, 220 translation archives (inflection-preserving rewrite via the
existing `tools/translate/rebrand.py` machinery), and the brand-asset pipeline
(`tools/brand/gen_icons.py` regenerates all four shipped `.ico` files and the
About/splash artwork from the delivered full-bleed renders). Upstream-derived
names (`salamand.sln`, `SALAMANDER_*`, salmon, shexreg CLSID) stay, exactly as
in feature 032. The constitution's Principle II identity anchor is amended in
the same feature (Newt Commander 0.1.0 → Tandem Commander 0.1.0, MAJOR bump).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022); Win32
resource scripts (`.rc`/`.rc2`/`.rh2`); MSBuild (`.vcxproj`/`.props`/`.targets`);
Windows Batch + PowerShell build scripts; Python 3.13 (`tools/`, developer-side
only); Inno Setup 6 (`setup/*.iss`)
**Primary Dependencies**: none new — Pillow (already required for
`tools/brand/gen_icons.py`), existing `tools/translate` package
**Storage**: Windows Registry — root moves to `HKCU\Software\Tandem
Commander\0.1` (+ `...\Bug Reporter`); per-user folders `%APPDATA%\Tandem
Commander`, `%LOCALAPPDATA%\Tandem Commander\mdview.WebView2`
**Testing**: `build.cmd full release` clean build; `python
tools/brand/gen_icons.py --verify`; `python -m tools.translate.rebrand`
residue gate; repository-wide grep gate (FR-015); manual smoke checklist in
`quickstart.md`; Inno Setup compile + install/uninstall on a clean profile
**Target Platform**: Windows 11+ (x64 primary, x86 shellext also built)
**Project Type**: Windows desktop application monorepo (1 exe, 28 plugins,
lang modules, helpers, installer)
**Performance Goals**: N/A — identity rename; zero behavioral change intended
**Constraints**: output-dir rename must change in lockstep across 20
`*_base.props`, 2 exe-path props, `Directory.Build.targets` (silent-no-op
string match), `saltests.vcxproj` and 10 scripts — verified by grep + clean
rebuild; UTF-8-BOM encoding preserved in every touched file; translation
rewrite must preserve inflectional suffixes and accelerator markers
**Scale/Scope**: ~392 tracked files; 1,717 occurrences in 220 `.slt` files;
30 plugin `versinfo.rh2`; 65 occurrences in `texts.rc2`; 4 shipped `.ico`
regenerated; 2 file renames (`setup/newtcommander.iss`,
`tools/brand/newt-commander-icon.svg`)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Verdict | Notes |
|---|---|---|
| I. Build Reproducibility | PASS | Single-command build preserved; output root renamed atomically in one commit across all props/targets/scripts; `OPENSAL_BUILD_DIR` mechanism untouched. Clean full rebuild is an explicit gate (SC-002). |
| II. Backward Compatibility | PASS **with amendment** | Principle II anchors the product identity to Newt Commander (binary, registry root, IPC names) and forbids piecemeal reintroduction of identity changes. This feature changes that identity **wholesale, once, by design** — the same shape as the feature 032 break it codifies. The constitution is amended in this feature (baseline re-anchored to Tandem Commander 0.1.0, MAJOR bump), so post-merge the constitution and product agree. Functionality, plugin ABI (105) and data formats do not regress; old exported FTP `.str` files remain importable (FR-013). |
| III. Incremental Modernization | PASS | Pure mechanical rename; no adjacent refactoring. Brand-derived identifier renames (NEWT→TANDEM, NC→TC) were explicitly decided in clarification and stay within touched lines. |
| IV. Windows Platform Commitment | PASS | No platform/toolchain change. |
| V. Plugin Architecture Preservation | PASS | Plugin interface stays 105; only version-resource strings and home-page URLs change. Pre-rename plugin binaries still load (FR-016). |
| VI. UI Consistency | PASS | Artwork swap goes through the sanctioned feature-035 pipeline; no dialog/control changes; wordmark drawing code keeps its GDI approach with identical palette values. |

**Post-design re-check**: no new violations introduced by Phase 1 artifacts;
the Principle II amendment is scoped in `contracts/identity-map.md` and
executed as part of the documentation work.

## Project Structure

### Documentation (this feature)

```text
specs/046-tandem-commander-rebrand/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output — identity inventory by subsystem
├── quickstart.md        # Phase 1 output — build & verification walkthrough
├── contracts/
│   └── identity-map.md  # Phase 1 output — definitive old→new identity map
│                        #   + replacement token order + what must NOT change
├── checklists/
│   └── requirements.md  # Spec quality checklist (complete)
└── tasks.md             # Phase 2 output (/speckit.tasks — not created here)
```

### Source Code (repository root)

```text
src/
├── versinfo.rh2                      # app identity defines (COMPANY/DESCRIPTION/INTERNAL/SLG_WEB)
├── consts.h                          # SALCFG_ROOTS_COUNT comment
├── mainwnd2.cpp                      # registry root "Software\Tandem Commander\0.1"
├── salamdr1.cpp                      # window title + class name + captions
├── mainwnd1.cpp, mainwnd3.cpp        # SALAMANDER_TEXT_VERSION, issues URL
├── dialogs.cpp, dialogs2.cpp, dialogs3.cpp  # URLs, root sniffing, captions
├── salamdr5.cpp                      # %APPDATA% folder name
├── tasklist.cpp                      # kernel-object names (6)
├── salmoncl.cpp                      # bug-reporter mutex/key/path
├── shexreg.c, shexreg.h              # shell-ext IPC names + registry value
├── callstk.cpp                       # bug-report file header
├── logo.cpp                          # wordmark text + TC_COLOR_* palette + URL
├── plugins2.cpp                      # default plugin descriptions
├── manifest.xml                      # assemblyIdentity
├── lang/texts.rc2, lang/lang.rc      # English strings & dialogs
├── salmon/                           # crash reporter (cpp, rc, manifest, config)
├── shellext/shellext.rc              # x86/x64 shell-ext version info
├── translator/restart.cpp, trldata.h # process-name match, default web
├── Directory.Build.targets           # IntDir rewrite token (lockstep!)
├── vcxproj/                          # salamand.vcxproj TargetName, *_base.props,
│   └── ...                           #   build_langs, signslgs, verify_slg, populate
└── plugins/
    ├── shared/spl_vers.h             # VERSINFO_HOLDER_* macro + REQUIRE string
    ├── shared/versinfo.rc2           # shared ProductName template
    ├── shared/vcxproj/{plugin,lang}_base.props, x86.props, x64.props
    └── <28 plugins>/                 # versinfo.rh2 ×30, SetPluginHomePageURL ×19,
                                      #   lang.rc/rc2 strings, ftp2.cpp header,
                                      #   zip SFX defaults, mdview WebView2 path
setup/
└── newtcommander.iss → tandemcommander.iss   # renamed + rebranded + new AppId
tools/
├── brand/                            # icon-master/icon-<N>/about sources swapped,
│   └── ...                           #   gen_icons.py docstring, README, ref SVGs
├── translate/                        # rebrand.py rules+constants, config.py,
│   └── ...                           #   merge.py, deepl.py, README
└── salbreak/tasklist.cpp             # kernel-object names mirror
translations/
├── <11 languages>/*.slt              # 220 files via rebrand.py --apply
├── languages.cfg                     # web= fields + author + header
└── !update_langs_from_translator.bat # build-dir path
build.cmd, !clean_all_interm.cmd, help/src/*.bat   # output-dir paths
README.md, CLAUDE.md, AUTHORS, architecture/*.md   # docs
.specify/memory/constitution.md                    # Principle II re-anchor
```

**Structure Decision**: No structural change — this feature edits identity
strings and build metadata in place. Exactly two tracked files are renamed
(`setup/newtcommander.iss` → `setup/tandemcommander.iss`;
`tools/brand/newt-commander-icon.svg` replaced by the new reference vectors).
Historical `specs/` content is exempt from all edits.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Principle II identity anchor changes (constitution amendment, MAJOR) | The feature's entire purpose is a new product identity; the constitution names the old one and must be re-anchored in the same change, exactly as feature 032 did for the previous identity. | Keeping the Newt Commander anchor while shipping Tandem Commander binaries would leave the constitution factually wrong and would force every future PR through a false compliance gate. |
