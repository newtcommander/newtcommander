# Data Model: Tandem Commander Rebrand

**Feature**: 046-tandem-commander-rebrand · **Date**: 2026-08-01

A rebrand has no runtime data entities; its "model" is the inventory of
identity-bearing elements and the files that carry them. Each element below is
a unit of change with its own verification. The definitive old→new value map
lives in [contracts/identity-map.md](contracts/identity-map.md).

## E1. Central version-info defines

| Attribute | Value / Files |
|---|---|
| Carrier files | `src/plugins/shared/spl_vers.h`, `src/versinfo.rh2`, `src/plugins/shared/versinfo.rc2`, 30 × `src/plugins/**/versinfo.rh2` |
| Fields | `VERSINFO_HOLDER_NEWT` (macro name → `VERSINFO_HOLDER_TANDEM`; value "Pavel Stupka" unchanged), `VERSINFO_COPYRIGHT_NEWT` → `_TANDEM`, `VERSINFO_COMPANY`, `VERSINFO_DESCRIPTION`, `VERSINFO_INTERNAL`, `VERSINFO_SLG_WEB`, `REQUIRE_LAST_VERSION_OF_SALAMANDER` string, shared `ProductName` |
| Invariants | Version numbers (0.1.0 / build 184 / interface 105) unchanged; holder defined in exactly one place; copyright year-split rule intact |
| Validation | Build succeeds; `.exe`/`.spl` version resources show new strings; macro-name sweep finds no `_NEWT` |

## E2. Hardcoded C++ identity strings

| Attribute | Value / Files |
|---|---|
| Display name & captions | `salamdr1.cpp` (`MAINWINDOW_NAME`, captions, reinstall texts), `mainwnd1.cpp` (`SALAMANDER_TEXT_VERSION`), `dialogs2/3.cpp`, `mainwnd2.cpp:716` |
| Registry roots | `mainwnd2.cpp:159` (`Software\Tandem Commander\0.1`), `salmoncl.cpp:39` + `salmon/config.cpp:11` (`...\Bug Reporter`), sniffing literals `dialogs2.cpp:652,684` |
| Kernel objects | `tasklist.cpp:30-36` (6 names) + mirror `tools/salbreak/tasklist.cpp:16-22`, `salmoncl.cpp:30`, `salmon/salmon.cpp:759` |
| Window class | `salamdr1.cpp:217` `TandemCommanderMainWindowVer01` |
| Shell-ext IPC | `shexreg.c:33-46` (`TCExten_*`, `TandemCommanderVer...`, descr); `shexreg.h` comments; CLSID **unchanged** |
| Per-user paths | `salamdr5.cpp:1856,1866`, `salmoncl.cpp:117`, `plugins/mdview/viewer.cpp:88` |
| Binary-name consumers | `salmon/salmon.cpp:324,331`, `plugins/filecomp/fcremote/fcremote.cpp:234`, `translator/restart.cpp:51` |
| URLs in code | `dialogs.cpp:2073`, `dialogs2.cpp:1094`, `mainwnd3.cpp:2571`, `salamdr1.cpp:86`, `logo.cpp:497`, 19 × `SetPluginHomePageURL`, `nethood/config.h`, `portables/fx.cpp`, `diskmap`, `translator/trldata.h` |
| Wordmark | `logo.cpp`: `part1 = "Tandem "`, `TCDrawWordmark`, `TC_COLOR_*` (RGB values unchanged — new design palette is identical) |
| Data-file headers | `callstk.cpp:748`, `ftp2.cpp:887` (+ new `STR_FILE_HEADER_OLD` accepted on import), `checksum/dialogs.cpp:1056`, `zip/add_del.cpp:368-369`, `pictview/pvtwain.cpp:43-44`, `zip2sfx/texts.h` |
| Plugin misc | `plugins2.cpp:1538-1545`, `diskmap/precomp.h:13`, `fcremote.cpp:65,68` |

## E3. Resource files

| Attribute | Value / Files |
|---|---|
| Manifests | `src/manifest.xml`, `src/salmon/manifest.xml` (assemblyIdentity + description) |
| English strings | `src/lang/texts.rc2` (65×), `src/lang/lang.rc` (26× incl. About `newtcommander.org` LTEXT) |
| Standalone VERSIONINFO | `salmon.rc`, `shellext.rc`, `zip2sfx.rc`, `SELFEXTR.RC`, `sfxmake.rc`, `salpvenv.rc`, `fcremote.rc`, `exif.rc2`, `7za/spl/VersionInfo.rc`, `tserver.rc` |
| Plugin lang resources | 18 × `plugins/*/lang/lang.rc(.rc2)` (`IDS_PLUGIN_HOME`, prose, issue URLs) |

## E4. Build system

| Attribute | Value / Files |
|---|---|
| Target name | `src/vcxproj/salamand.vcxproj` `<TargetName>tandemcommander</TargetName>` |
| Output root segment | `tandemcommander\` in 20 `*_base.props` + 3 `*_debug.props` + `saltests.vcxproj` |
| Exe path | `shared/vcxproj/x86.props:8`, `x64.props:8` (`SalPath` = dir + exe) |
| IntDir rewrite | `src/Directory.Build.targets:36` — MUST match props segment (silent no-op hazard) |
| Scripts | `build.cmd`, `!clean_all_interm.cmd`, `!populate_build_dir.cmd` (+ `tandemcommander.lnk`), `build_langs.cmd/.ps1`, `signslgs.cmd`, `verify_slg.ps1`, `zip/vcxproj/selfextr/makeall.bat`, `help/src/compileall.bat`, `help/src/copy_to_salbin.bat`, `translations/!update_langs_from_translator.bat` |
| Invariant | `OPENSAL_BUILD_DIR` env-var name unchanged (build-infrastructure, not brand) |

## E5. Installer

| Attribute | Value / Files |
|---|---|
| File | `setup/newtcommander.iss` → `setup/tandemcommander.iss` (git mv) |
| Directives | `MyAppName`, `MyAppURL`, `MyAppExeName`, `AppId` (new GUID), `DefaultDirName={autopf}\Tandem Commander`, `OutputBaseFilename=tandemcommander-0.1.0-x64-setup`, `Source:` paths, icons/run entries |
| Icon | `setup/setup.ico` regenerated from new frames |

## E6. Translations

| Attribute | Value / Files |
|---|---|
| Archives | 220 `.slt` (11 languages × 20 modules), 1,717 occurrences incl. 374 URLs; inflected forms (`Commanderu`, `Commandera`, `Commanders`, `Commandert`) |
| Registry | `translations/languages.cfg` — 11 × `web =`, 1 × `author =`, header comment |
| Tooling | `tools/translate/rebrand.py` (constants + new previous-identity rules + residue patterns), `config.py:91`, `merge.py:36`, `deepl.py:153` (User-Agent), `__init__.py`, `README.md` |
| Validation | `python -m tools.translate.rebrand` exits 0 with no residue (old + predecessor identities) |

## E7. Brand assets

| Attribute | Value / Files |
|---|---|
| Sources (tools/brand/) | `icon-master.png` ← full-1024; `icon-{16,24,32,48,64,128,256}.png` ← full per-size renders; `about.png` ← plain (shadowed) 1024 |
| Reference vectors | remove `newt-commander-icon.svg`; add `tandem-commander-icon.svg`, `tandem-commander-icon-full.svg`, 2 lockup SVGs |
| Generated outputs | `src/res/salamand.ico`, `src/salmon/res/salmon.ico`, `src/setup/res/setup.ico`, `src/setup/remove/icon1.ico`, `src/res/logo.png` (upstream file names kept — no project edits) |
| Docs | `tools/brand/README.md` (rebrand + fold in DESIGN_README palette/usage), `gen_icons.py` docstring |
| Validation | `python tools/brand/gen_icons.py --verify` all OK |

## E8. Documentation & governance

| Attribute | Value / Files |
|---|---|
| Repo docs | `README.md`, `CLAUDE.md`, `AUTHORS`, `architecture/01-project-overview.md`, `03-build-pipeline.md`, `06-plugin-architecture.md` |
| Governance | `.specify/memory/constitution.md` — Principle II re-anchor, MAJOR bump → 3.0.0, Sync Impact Report, footer fix |
| Exempt | everything under `specs/0*` (historical record), git branches/history, `doc/` third-party notices (no brand hits) |

## Relationships & change-ordering constraints

1. **E1 before E2/E3 compile**: macro renames (`VERSINFO_HOLDER_TANDEM`) must
   land together with all 45+ reference sites (single commit).
2. **E4 is atomic**: props + targets + vcxproj + scripts in one commit, then
   grep gate + clean rebuild (silent-no-op hazard).
3. **E7 before visual verification**: icons/artwork regenerate before the
   quickstart smoke pass.
4. **E2 registry root + sniffing literals together**: `mainwnd2.cpp` and
   `dialogs2.cpp` must agree within one commit.
5. **E6 last-ish**: run rebrand.py after English resources are final so its
   residue check reflects the shipped state.
6. **E8 constitution amendment** merges with the feature (not before), so the
   constitution never describes an identity the tree doesn't have.
