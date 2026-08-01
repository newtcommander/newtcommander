# Tasks: Tandem Commander Rebrand

**Input**: Design documents from `/specs/046-tandem-commander-rebrand/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/identity-map.md, quickstart.md

**Tests**: Not requested — verification is via the static gates (quickstart G1–G5), build checkpoints and manual smoke passes defined per story.

**Organization**: Tasks are grouped by user story. The authoritative old→new
values for every task are in `contracts/identity-map.md`; replacement order is
research.md R1 (longest token first). Historical `specs/0*` files are exempt
from every task. All touched files keep UTF-8-BOM encoding.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 = renamed identity, US2 = visual brand, US3 = build+installer, US4 = translations+docs

## Phase 1: Setup

**Purpose**: Baseline, inputs and one-off values needed later

- [ ] T001 Confirm clean working tree on branch `046-tandem-commander-rebrand`, then run quickstart gates G1–G3 to snapshot the *pre-rename* hit counts (expect: many hits; they become the burn-down baseline). Record counts in the PR description or a scratch note — no repo file.
- [ ] T002 [P] Verify inputs and tooling: all 18 PNG renders + 4 SVGs present in `temp/tandem_design/`; `python -c "import PIL"` succeeds; Inno Setup 6 (`iscc`) available for US3.
- [ ] T003 [P] Generate a fresh installer AppId GUID (`powershell [guid]::NewGuid()`) and replace the "fresh GUID generated at implementation" placeholder in `specs/046-tandem-commander-rebrand/contracts/identity-map.md` §5 with the literal value (keep it there as the record; used by T032).

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Central version-info identity (data-model E1) — the defines every
binary's version resource and copyright notice flow from. The macro rename
(`VERSINFO_HOLDER_NEWT` → `VERSINFO_HOLDER_TANDEM`) breaks compilation until
all reference sites are updated, so T004–T006 MUST land together as **one
commit** and the phase ends with a compile check.

**⚠️ CRITICAL**: US1 and US4 build on these strings; complete before starting them (US2 is independent).

- [ ] T004 Rename the identity defines in `src/plugins/shared/spl_vers.h` (`VERSINFO_HOLDER_NEWT`→`VERSINFO_HOLDER_TANDEM`, value "Pavel Stupka" unchanged; `REQUIRE_LAST_VERSION_OF_SALAMANDER` → "This plugin requires Tandem Commander 0.1.0 build 184 …"; update the holder/versioning comments incl. the build-184 history note), `src/versinfo.rh2` (`VERSINFO_COPYRIGHT`/`_NEWT`→`_TANDEM`, `VERSINFO_COMPANY` "Tandem Commander Project", `VERSINFO_DESCRIPTION` "Tandem Commander, File Manager", `VERSINFO_INTERNAL` "TANDEMCOMMANDER", `VERSINFO_SLG_WEB` "tandemcommander.org") and `src/plugins/shared/versinfo.rc2` (ProductName "Tandem Commander", SLGIncomplete github URL).
- [ ] T005 Update all 30 plugin version files `src/plugins/{7zip,automation,checksum,checkver,dbviewer,demomenu,demoplug,demoview,filecomp,folders,ftp,mdview,mmviewer,nethood,peviewer,pictview,portables,regedt,renamer,sftp,tar,uncab,unchm,undelete,uniso,unmime,unole,unrar,zip}/versinfo.rh2` + `src/plugins/diskmap/DiskMapPlugin/versinfo.rh2`: `VERSINFO_HOLDER_TANDEM` reference, `VERSINFO_COMPANY` "Tandem Commander Project", `VERSINFO_DESCRIPTION` "... for Tandem Commander", `VERSINFO_SLG_WEB` "www.tandemcommander.org" (pictview keeps its `_T(...)` wrappers; automation also has a header comment).
- [ ] T006 Update the remaining `VERSINFO_*_NEWT` reference sites in the same commit: `src/plugins/zip/sfxmake/sfxmake.rc`, `src/plugins/pictview/salpvenv.rc`, `src/plugins/filecomp/fcremote/fcremote.rc` (LegalCopyright macro refs + CompanyName/ProductName/FileDescription strings), `src/plugins2.cpp:1538-1545` (copyright macro refs + "… support for Tandem Commander." descriptions), `src/plugins/zip/add_del.cpp` (include comment + SFX `Vendor` macro ref), `src/logo.cpp:242,508` (macro refs only — full logo.cpp rebrand is T015), `src/lang/lang.rc:632` comment.

**Checkpoint**: `build.cmd` (Debug x64) compiles; `tandemcommander.exe` does not exist yet (TargetName still old) but version resources of built modules show "Tandem Commander Project".

---

## Phase 3: User Story 1 — Renamed application identity (Priority: P1) 🎯 MVP

**Goal**: Every user-visible and OS-visible identity surface says Tandem
Commander; the binary is `tandemcommander.exe`; registry, kernel objects,
manifests, per-user paths renamed. No config migration.

**Independent Test**: Build, run the exe: window title/About/splash/Task List
read "Tandem Commander"; Explorer Details show new product/company/original
filename; config lands under `HKCU\Software\Tandem Commander\0.1`; second
launch activates the first instance.

### Implementation for User Story 1

- [ ] T007 [US1] `src/salamdr1.cpp`: `MAINWINDOW_NAME` "Tandem Commander", `CMAINWINDOW_CLASSNAME` "TandemCommanderMainWindowVer01", MessageBox captions (:4029, :4041, :4649), "Please reinstall Tandem Commander." texts (:108, :3968, :4039), issues URL (:86).
- [ ] T008 [US1] Registry root and its sniffers in one commit: `src/mainwnd2.cpp:157-163` (`"Software\\Tandem Commander\\0.1"` + comment + :716 exit text), `src/dialogs2.cpp:652,684` (`StrIStr(..., "Tandem Commander")`), `:654,686` ("Tandem Commander %s"), `:1094` repo URL, `:1106` reinstall text, `src/consts.h:2116` comment.
- [ ] T009 [P] [US1] Remaining core-app strings: `src/mainwnd1.cpp:27` (`SALAMANDER_TEXT_VERSION` "Tandem Commander …"), `src/mainwnd3.cpp:2571` issues URL, `src/dialogs.cpp:2073` releases URL, `src/dialogs3.cpp:2609` default caption, `src/salamdr5.cpp:1856,1866` `%APPDATA%\Tandem Commander`, `src/callstk.cpp:748` "Tandem Commander Bug Report File".
- [ ] T010 [P] [US1] Kernel-object names: `src/tasklist.cpp:30-36` (six `TandemCommander*` names per identity-map §3) and keep `tools/salbreak/tasklist.cpp:16-22` byte-identical.
- [ ] T011 [P] [US1] Shell-extension identity: `src/shexreg.c:33-46` (`TCExten_SharedMemMutex1/SharedMem1/DoPasteEvent1`, registry value `TandemCommanderVer…`, descr "Shell Extension (%s) for Tandem Commander …", "TC = Tandem Commander" comment), `src/shexreg.h:60,108` comments. CLSID stays.
- [ ] T012 [P] [US1] Manifests: `src/manifest.xml` (`TandemCommander.TandemCommander` + description) and `src/salmon/manifest.xml` (`TandemCommander.BugReporter` + description).
- [ ] T013 [P] [US1] Crash reporter: `src/salmon/salmon.cpp` (:18 APP_NAME, :311 URL, :324 comment, :331 `\\tandemcommander.exe`, :759 mutex), `src/salmon/salmon.rc:24,25,31`, `src/salmon/config.cpp:11` `Software\\Tandem Commander\\Bug Reporter`, `src/salmon/upload.cpp:7,18`, `src/salmon/dialogs.cpp:456` comment, `src/salmoncl.cpp:30,39,117` (mutex/key/path).
- [ ] T014 [P] [US1] Translator tooling code: `src/translator/restart.cpp:51` match "tandemcommander.exe", `src/translator/trldata.h:575` `www.tandemcommander.org`.
- [ ] T015 [P] [US1] `src/logo.cpp`: `part1 = "Tandem "`, `NCDrawWordmark`→`TCDrawWordmark` (+ `newtClr`→`tandemClr`), `NC_COLOR_*`→`TC_COLOR_*` (RGB values unchanged), brand comments, :497 URL `https://tandemcommander.org`.
- [ ] T016 [P] [US1] English app resources: `src/lang/texts.rc2` (65 hits — About, `IDS_EXECUTE_SALDIR`, color scheme name, `TANDEMCOMMANDER.EXE` usage/config-export texts, SALMON block incl. 5 issue URLs, SLGINCOMPLETE) and `src/lang/lang.rc` (26 hits — captions, About `tandemcommander.org` + "Tandem Commander is free software", releases URL control).
- [ ] T017 [P] [US1] Shell-ext + tserver resources: `src/shellext/shellext.rc` (:14/:16 FILE_DESCR, :32 Comments, :39 CompanyName, :51 ProductName), `src/tserver/tserver.rc:111` www LTEXT.
- [ ] T018 [P] [US1] Plugin home pages & web literals: the 19 `SetPluginHomePageURL("www.tandemcommander.org")` calls (`7zip/7zip.cpp`, `automation/entry.cpp`, `checksum/checksum.cpp`, `checkver/checkver.cpp`, `dbviewer/dbviewer.cpp`, `filecomp/filecomp.cpp`, `folders/folders.cpp`, `ftp/ftp.cpp`, `mmviewer/mmviewer.cpp`, `peviewer/peviewer.cpp`, `regedt/regedt.cpp`, `renamer/renamer.cpp`, `tar/tardll.cpp`, `uncab/uncab.cpp`, `unchm/unchm.cpp`, `undelete/undelete.cpp`, `uniso/uniso.cpp`, `unmime/unmime.cpp`, `unole/unole2.cpp`, `zip/main.cpp`) + `nethood/config.h:15` + `portables/fx.cpp:400` + `diskmap/DiskMapPlugin/DiskMapPlugin.cpp:34` + `diskmap/DiskMapPlugin/precomp.h:13` REQUIRE string.
- [ ] T019 [P] [US1] Plugin English lang resources (18 files): `IDS_PLUGIN_HOME`/prose/issue URLs in `plugins/{7zip,checksum,demomenu,demoplug,demoview,filecomp,ftp,mdview,peviewer,pictview,regedt,tar,unchm,uniso,zip}/lang/lang.rc2` and `plugins/{demoplug,filecomp,pictview,zip}/lang/lang.rc`.
- [ ] T020 [US1] FTP `.str` header compatibility (FR-013): in `src/plugins/ftp/ftp2.cpp` set `STR_FILE_HEADER = "Tandem Commander - FTP Client - Exported Server Type"`, add `STR_FILE_HEADER_OLD = "Newt Commander - FTP Client - Exported Server Type"`, and extend the signature check at :1195-1202 to accept either header on import (export writes only the new one).
- [ ] T021 [P] [US1] Other data-file writers & misc plugin strings: `src/plugins/checksum/dialogs.cpp:1056` ("; Generated by Tandem Commander, https://tandemcommander.org"), `src/plugins/zip/add_del.cpp:369` SFX WWW, `src/plugins/pictview/pvtwain.cpp:43-44` TWAIN identity, `src/plugins/zip/zip2sfx/texts.h:11-12` CLI banner, `src/plugins/filecomp/fcremote/fcremote.cpp` (:65, :68 texts; :234 `tandemcommander.exe`), `src/plugins/mdview/viewer.cpp:88` `\\Tandem Commander\\mdview.WebView2`.
- [ ] T022 [P] [US1] Remaining standalone resource files with hardcoded names: `src/plugins/zip/zip2sfx/zip2sfx.rc` (:34, :41), `src/plugins/zip/selfextr/SELFEXTR.RC:38`, `src/plugins/7zip/7za/spl/VersionInfo.rc:3`, `src/plugins/pictview/exif/exif.rc2:45`.
- [ ] T023 [US1] `src/vcxproj/salamand.vcxproj:96-97`: `<TargetName>tandemcommander</TargetName>` + comment (output-directory renames stay in US3).
- [ ] T024 [US1] Checkpoint: `build.cmd full` (Debug), then smoke per quickstart §3 rows 1–7 (title, About incl. copyright lines, splash wordmark text, Task List caption, registry root, single-instance mutex, plugins load with interface 105).

**Checkpoint**: US1 fully testable — the product is Tandem Commander end to end (with old build-tree paths and old artwork still in place).

---

## Phase 4: User Story 2 — New visual brand applied (Priority: P2)

**Goal**: New orange folder icon (full-bleed renders) in all 4 shipped `.ico`
+ About/splash artwork, regenerated via the feature-035 pipeline.

**Independent Test**: `python tools/brand/gen_icons.py --verify` all OK; after
a build, Explorer shows the new icon at 16–256 px and About/splash show the
new artwork. (Runs independently of US1/US3 — only T028's wordmark check
needs US1's T015.)

### Implementation for User Story 2

- [ ] T025 [P] [US2] Stage brand sources in `tools/brand/`: copy `temp/tandem_design/png/tandem-commander-icon-full-1024.png` → `icon-master.png`; `tandem-commander-icon-full-{16,24,32,48,64,128,256}.png` → `icon-{16,24,32,48,64,128,256}.png`; `tandem-commander-icon-1024.png` (margin+shadow variant) → `about.png`; delete `tools/brand/newt-commander-icon.svg`; add `tandem-commander-icon.svg`, `tandem-commander-icon-full.svg`, `tandem-commander-lockup-light.svg`, `tandem-commander-lockup-dark.svg` from `temp/tandem_design/`.
- [ ] T026 [US2] Run `python tools/brand/gen_icons.py` (regenerates `src/res/salamand.ico`, `src/salmon/res/salmon.ico`, `src/setup/res/setup.ico`, `src/setup/remove/icon1.ico`, `src/res/logo.png` — file names unchanged by design), copy the regenerated `src/setup/res/setup.ico` over `setup/setup.ico`, then run `python tools/brand/gen_icons.py --verify` (gate G4, all OK).
- [ ] T027 [P] [US2] Docs of the pipeline: rewrite `tools/brand/README.md` for Tandem Commander (swap-table, wordmark note → `TCDrawWordmark`, reference-files table with the 4 new SVGs, fold in the palette/usage rules from `temp/tandem_design/DESIGN_README.md`); update the `tools/brand/gen_icons.py` docstring.
- [ ] T028 [US2] Visual checkpoint: `build.cmd`, then check Explorer icon at 16/32/48/256 px, window/taskbar icon, splash + About artwork (and, if T015 already landed, the "Tandem Commander" two-tone wordmark on both).

**Checkpoint**: US1 + US2 = renamed product with new artwork, still in old build tree.

---

## Phase 5: User Story 3 — Build & installer outputs (Priority: P3)

**Goal**: Build outputs under `<OPENSAL_BUILD_DIR>tandemcommander\`, all
scripts aligned, installer renamed with new AppId installing to
`Program Files\Tandem Commander`.

**Independent Test**: quickstart §1 G3 (zero `newtcommander` in
props/targets/vcxproj) + §2 (clean Release build: exe in renamed tree,
intermediates in `obj\`, incremental build stays incremental) + §4
(installer compile/install/uninstall).

### Implementation for User Story 3

- [ ] T029 [US3] **Atomic output-root rename — one commit** (Directory.Build.targets:22-29 hazard): replace segment `newtcommander\` → `tandemcommander\` in `src/vcxproj/sal_base.props`, `lang_base.props`, `sqlite/sqlite_base.props`, `salmon/salmon_base.props`, `salopen/salopen_base.props`, `salopen/salopen_debug.props`, `salspawn/salspawn_base.props`, `salspawn/salspawn_debug.props`, `shellext/shellext_base.props`, `shellext/shellext_debug.props`, `saltests/saltests.vcxproj:50`, `src/plugins/shared/vcxproj/plugin_base.props`, `lang_base.props`, `x86.props:8` + `x64.props:8` (`SalPath` — directory **and** `tandemcommander.exe`), `src/plugins/7zip/vcxproj/7ZA/7za_base.props`, `7zwrapper/7zwrapper_base.props`, `src/plugins/filecomp/vcxproj/fcremote/fcremote_base.props`, `src/plugins/pictview/vcxproj/salpvenv_base.props`, `exif/exif_base.props`, `src/plugins/unchm/vcxproj/chmlib/chmlib_base.props`, `src/plugins/zip/vcxproj/zip2sfx/zip2sfx_base.props`, and `src/Directory.Build.targets:36` IntDir token + its KEEP-IN-SYNC comment.
- [ ] T030 [US3] Build/helper scripts: `build.cmd` (:122, :216, :296, :317 OUT_DIR + banners), `!clean_all_interm.cmd` (:24-32), `src/vcxproj/!populate_build_dir.cmd` (12 sites incl. shortcut `tandemcommander.lnk`), `src/vcxproj/build_langs.cmd:70`, `build_langs.ps1:71-72`, `signslgs.cmd:17-24`, `verify_slg.ps1:52` (`tandemcommander.exe`), `src/plugins/zip/vcxproj/selfextr/makeall.bat` (5 sites), `help/src/compileall.bat` (5), `help/src/copy_to_salbin.bat` (3), `translations/!update_langs_from_translator.bat:17`.
- [ ] T031 [US3] Build gate: run quickstart G3 grep (`git grep -n "newtcommander" -- "*.props" "*.targets" "*.vcxproj"` → zero); `build.cmd rebuild` then `build.cmd full release`; verify `%OPENSAL_BUILD_DIR%tandemcommander\Release_x64\tandemcommander.exe`, no `newtcommander\` dir recreated, Release intermediates under `obj\Release_x64\`, and a follow-up incremental build is fast (PCH cache intact).
- [ ] T032 [US3] Installer: `git mv setup/newtcommander.iss setup/tandemcommander.iss`; update `MyAppName` "Tandem Commander", `MyAppURL` `https://tandemcommander.org/`, `MyAppExeName` `tandemcommander.exe`, `AppId` = GUID from T003, `Source:` paths `..\build\tandemcommander\Release_x64\...`, `OutputBaseFilename=tandemcommander-{#MyAppVersion}-x64-setup` (Publisher "Pavel Stupka" and `DefaultDirName={autopf}\{#MyAppName}` derive correctly).
- [ ] T033 [US3] Installer checkpoint: `iscc setup\tandemcommander.iss` → `setup\output\tandemcommander-0.1.0-x64-setup.exe`; install on a clean profile → `Program Files\Tandem Commander`, Start-menu/desktop shortcuts with new icon, app runs; uninstall removes them; a pre-existing Newt Commander install (if any) is untouched.

**Checkpoint**: Full release pipeline produces Tandem Commander artifacts end to end.

---

## Phase 6: User Story 4 — Translations & documentation (Priority: P4)

**Goal**: All 11 languages carry the inflected new name; tooling, language
registry, repo docs and constitution rebranded. Depends on US1 (T016/T019 —
English strings final before the residue gate is meaningful).

### Implementation for User Story 4

- [ ] T034 [US4] Translation tooling: in `tools/translate/rebrand.py` set `PRODUCT`/`PROJECT`/`WEB` to Tandem forms, add previous-identity rules ahead of the Salamander rules (`www.newtcommander.org`→WEB; `NEWTCOMMANDER.EXE`/`newtcommander.exe`/`NewtCommander.exe`→Tandem forms; `\bNEWT COMMANDER(\w*)`/`\bNewt Commander(\w*)`→suffix-preserving PRODUCT forms), extend `find_residue()` with `newt`-identity patterns, update the module docstring; also `tools/translate/config.py:91` (`tandemcommander.exe`), `merge.py:36` (`build/tandemcommander/translator/templates`), `deepl.py:153` (User-Agent `TandemCommander-translate/0.1`), `__init__.py:1`, `README.md:58`.
- [ ] T035 [US4] Run `python -m tools.translate.rebrand --apply` (rewrites the 220 `.slt` files incl. disabled languages), review the printed samples for inflection correctness (cs "Tandem Commanderu", sk/de/nl/hu suffixes), then re-run without `--apply` → gate G5: "no predecessor identity remains", exit 0.
- [ ] T036 [P] [US4] `translations/languages.cfg`: header comment, 11 × `web = www.tandemcommander.org`, ukrainian `author = Tandem Commander Project (machine translation)`.
- [ ] T037 [P] [US4] Repository docs: `README.md` (title, naming note, website/issues, example build dir, license paragraph), `CLAUDE.md` (title, What Is This, Product Identity section — new macro name `VERSINFO_HOLDER_TANDEM`, binary, registry root, websites, recent-changes note), `AUTHORS:1`, `architecture/01-project-overview.md:5`, `architecture/03-build-pipeline.md:158` output-tree diagram, `architecture/06-plugin-architecture.md:90` OutDir pattern.
- [ ] T038 [US4] Constitution amendment `.specify/memory/constitution.md`: title "Tandem Commander Constitution"; Principle II re-anchored to Tandem Commander 0.1.0 (`tandemcommander.exe`, `HKCU\Software\Tandem Commander`, IPC names, shell-ext identity) citing `specs/046-tandem-commander-rebrand/` as the second deliberate one-time identity change; plugin ABI 105 carried unchanged; fresh Sync Impact Report; version → **3.0.0** with corrected footer.
- [ ] T039 [US4] Language checkpoint: `build.cmd full` (produces enabled `.slg`), switch UI to Czech + one Germanic language, verify About/menus show correctly inflected "Tandem Commander" and `www.tandemcommander.org`.

**Checkpoint**: All four stories complete; every shipped language and all docs carry the new identity.

---

## Phase 7: Polish & Final Verification

**Purpose**: Repo-wide gates and release-shape validation

- [ ] T040 Final grep gates from quickstart §1: G1 (`git grep -iIl -e "newt commander" -e "newtcommander" -- ':!specs'` → empty) and G2 (`git grep -nE "VERSINFO_(HOLDER|COPYRIGHT)_NEWT|NCDrawWordmark|NC_COLOR_|NCExten_" -- ':!specs'` → empty); fix any stragglers found.
- [ ] T041 Full quickstart pass: §2 clean `build.cmd full release` + exe properties/icon; §3 runtime smoke incl. second-launch activation, shell-extension paste IPC, and SC-007 (`.str` exported by a pre-rename build — e.g. `temp/baseline_release_newtcommander.exe` — imports into the new build); §4 installer install/uninstall.
- [ ] T042 Formatting & encoding sweep over all touched files: `src/vcxproj/normalize.ps1` / clang-format check, confirm UTF-8-BOM preserved (spot-check `.rc`/`.rh2`/`.slt`), then commit any normalization diffs.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: none — start immediately; T002/T003 parallel.
- **Foundational (Phase 2)**: after Setup; T004→T005/T006 (macro name first); **lands as one commit**; blocks US1 & US4.
- **US1 (Phase 3)**: after Phase 2. T007–T022 are largely parallel [P] (disjoint files); T008 is one commit (root + sniffers); T020 contains real logic (dual-header import); T023 anytime; T024 last.
- **US2 (Phase 4)**: independent of Phases 2–3 (asset files only) — can run in parallel with US1; T028's wordmark assertion needs T015.
- **US3 (Phase 5)**: independent of US1/US2 content-wise, but T031's exe-name check expects T023; T029 **must be a single commit**; T032 needs T003; T033 needs T026 (setup icon) + T031.
- **US4 (Phase 6)**: T034/T036/T037 anytime; T035 after T034 and after US1 (T016/T019); T038 anytime; T039 after T035 + US3 (renamed build tree for `build_langs`).
- **Polish (Phase 7)**: after all stories; T040 before T041.

### Parallel Opportunities

- After Phase 2: T007–T022 (US1) fan out across disjoint files; simultaneously T025–T027 (US2) and T034/T036/T037 (US4 prep).
- T029+T030 (US3) touch only build files — parallel with US1 string work if commits stay separated.

## Implementation Strategy

**MVP = Phase 1 + 2 + US1** (renamed product, old artwork/paths) — ship-checkable via T024. Then US2 (visuals), US3 (release pipeline), US4 (languages + governance), Polish. Suggested commit granularity: one commit per task, except T004–T006 (one commit) and T029 (one commit); message prefix `[046]` per repo convention.
