# Tasks: Newt Commander Application Rebrand

**Input**: Design documents from `/specs/032-newt-commander-rebrand/`
**Prerequisites**: plan.md, spec.md, research.md (R1–R11), data-model.md, contracts/branding-identity.md, quickstart.md

**Tests**: No automated test framework in this repo; each phase ends with a build gate and the feature ends with the quickstart verification walkthrough (SC-001..SC-008). All identifier values MUST be taken verbatim from `contracts/branding-identity.md`.

**Organization**: Phases follow user stories US1 (identity) → US2 (separation) → US3 (visuals) → US4 (governance); asset generation is foundational (needed by US1's icon and US3's artwork).

## Phase 1: Setup — Brand asset toolkit

**Purpose**: Reproducible visual assets available for all later phases.

- [X] T001 Create `tools/brand/` with the icon generator: move the validated spike script to `tools/brand/gen_icons.py`, add `tools/brand/README.md` (usage, Pillow prerequisite, variant rules), copy `temp/visual_style/icon/newt-commander-icon.svg` to `tools/brand/newt-commander-icon.svg` and both lockups to `tools/brand/`
- [X] T002 Author nanosvg-compatible artwork SVGs in `tools/brand/`: `logo.svg` (the 96-viewBox icon, no `<text>`), `gradspl.svg` + `gradabt.svg` (blue `#3B82F6` → orange `#F97316` gradient bands sized like the originals in `src/res/`)
- [X] T003 Run `python tools/brand/gen_icons.py src/res` and overwrite `src/res/salamand.ico` (16 favicon / 24+32 simplified / 48+64+128+256 full, 32-bpp), `src/res/sal_r.ico`, `sal_g.ico`, `sal_b.ico` (16+32 px state-tinted); copy the authored `logo.svg`, `gradspl.svg`, `gradabt.svg` from T002 over `src/res/` (file names kept; `os.svg` left in place, retired from use in T015)

**Checkpoint**: `src/res` holds the new assets; `git diff --stat` shows only binary/SVG asset changes.

---

## Phase 2: Foundational — Version & product macros (blocks everything user-visible)

- [X] T004 In `src/plugins/shared/spl_vers.h`: set `VERSINFO_SALAMANDER_MAJOR 0`, `MINORA 1`, `MINORB 0`; fix the `#if (VERSINFO_SALAMANDER_MINORB == 0)` composition so the display string always carries all three components (`"0.1.0 (x64)"`); keep `VERSINFO_BUILDNUMBER 184` and `LAST_VERSION_OF_SALAMANDER 104`; reword `REQUIRE_LAST_VERSION_OF_SALAMANDER` to name `Newt Commander 0.1.0 (build 184)`
- [X] T005 In `src/versinfo.rh2`: `VERSINFO_COPYRIGHT` → year-split main-app string, `VERSINFO_COMPANY` → `Newt Commander Project`, `VERSINFO_DESCRIPTION` → `Newt Commander, File Manager`, `VERSINFO_COMMENT` rebrand, `VERSINFO_SLG_WEB` → `newtcommander.org`, `VERSINFO_INTERNAL` → `NEWTCOMMANDER`, `VERSINFO_ORIGINAL` → `NEWTCOMMANDER.EXE`
- [X] T006 [P] In `src/manifest.xml`: assemblyIdentity `name="NewtCommander.NewtCommander"`, `version="0.1.0.0"`, description `Newt Commander File Manager`
- [X] T007 [P] In `src/plugins/shared/versinfo.rc2`: hardcoded ProductName `"Open Salamander\0"` → `"Newt Commander\0"`; update the stale translation-forum comment URL (line ~68) to the GitHub repository

**Checkpoint**: `build.cmd` compiles (rc + core) — version identity in place.

---

## Phase 3: US1 — User sees a Newt Commander application (P1) 🎯 MVP

**Goal**: `newtcommander.exe` with new name/version everywhere the product names itself.
**Independent test**: build, launch, walk quickstart §1–§3 (minus About artwork, which lands in US3).

- [X] T008 [US1] Add `<TargetName>newtcommander</TargetName>` to `src/vcxproj/salamand.vcxproj` (unconditioned PropertyGroup after the Configuration imports) and update `<SalPath>` in `src/plugins/shared/vcxproj/x86.props:8` and `x64.props:8` to `newtcommander.exe`
- [X] T009 [P] [US1] Rename the launch/detect literals: `src/salmon/salmon.cpp:331` (`"\\newtcommander.exe"`), `src/translator/restart.cpp:51` (`"newtcommander.exe"`), `src/plugins/filecomp/fcremote/fcremote.cpp:234` (`"newtcommander.exe"`)
- [X] T010 [P] [US1] In `src/salamdr1.cpp`: `MAINWINDOW_NAME` → `"Newt Commander"` (line 216) and rebrand the remaining product-name literals at lines 108, 4031, 4639; in `src/mainwnd1.cpp:27`: `SALAMANDER_TEXT_VERSION` → `"Newt Commander " VERSINFO_VERSION`
- [X] T011 [P] [US1] Rebrand main-app user-visible literals: `src/callstk.cpp:748`, `src/dialogs3.cpp:2601`, `src/salamdr5.cpp:1856,1866`, and the `dialogs2.cpp:652-657,684-689` import-label patterns (dead path, keep compiling; label pattern `Newt Commander %s`)
- [X] T012 [P] [US1] In `src/lang/lang.rc` replace the 22 "Open Salamander" occurrences with "Newt Commander" — EXCEPT the About block (lines ~637-641) which gets: year-split copyright line, web `newtcommander.org`, "Newt Commander is free software" + GPLv2 line kept; in `src/lang/texts.rc2` update usage strings at lines 1323, 1855, 1856 to `NEWTCOMMANDER.EXE`/`newtcommander.exe`
- [X] T013 [P] [US1] Rebrand shipped-plugin literals: `src/plugins/pictview/pvtwain.cpp:43-44`, `src/plugins/ftp/ftp2.cpp:887`
- [X] T014 [US1] Replace vendor URLs in UI code: `src/mainwnd3.cpp:2565` (keyboard help page → GitHub repo README anchor or remove menu item), `:2571` (forum → GitHub issues), `src/dialogs.cpp:2078-2080` (beta/EAP downloads → GitHub releases or remove), `src/dialogs2.cpp:1086` (translations forum → GitHub), `src/logo.cpp:424-426` (About link → `https://newtcommander.org`, drop the `/cz` variant)

**Checkpoint**: build passes; window title `Newt Commander 0.1.0 (x64)`; exe properties correct (quickstart §1, §3 minus About artwork).

---

## Phase 4: US2 — Separate product for the system (P2)

**Goal**: registry/IPC/shellext/crash-reporter separation; coexistence with OS 5.0.
**Independent test**: quickstart §4–§5 registry audit + no vendor network path.

- [X] T015 [US2] In `src/consts.h:2096`: `SALCFG_ROOTS_COUNT` 83 → 1; in `src/mainwnd2.cpp:157-242`: reduce `SalamanderConfigurationRoots` to `{"Software\\Newt Commander\\0.1", NULL}` and `SalamanderConfigurationVersions` to `{"0.1"}`; verify `FindLatestConfiguration`, `FindLanguageFromPrevVerOfSal`, `DeleteOldConfigurations` compile and are no-op with a single root
- [X] T016 [P] [US2] In `src/tasklist.cpp:30-36`: process-list/first-instance/load-save names per contract; sync the mirror constants in the salbreak tool if `salbreak` exists in the tree (search `AltapSalamander3bProcessList` repo-wide)
- [X] T017 [P] [US2] In `src/salamdr1.cpp:217`: `CMAINWINDOW_CLASSNAME` → `"NewtCommanderMainWindowVer01"` (single-instance FindWindow)
- [X] T018 [P] [US2] Shell extension identity in `src/shexreg.h`: CLSID → `{A6D5A8E2-D69F-4E03-8396-781909E7A3AE}` (line 61), `SALSHEXT_SHAREDNAMESAPPENDIX` → `"010"` (line 107), shared object names → `NCExten_*1` (lines 30-32); in `src/shexreg.c:39,42`: registration name base → `"NewtCommanderVer"`, description → `for Newt Commander`; rebrand `src/shellext/shellext.rc` metadata (ProductName/Company/Copyright/Comments per contract)
- [X] T019 [P] [US2] Crash reporter: `src/salmoncl.cpp` — registry key (line 39), crash folder `"\\Newt Commander"` (line 117), mutex name (line 30); `src/salmon/config.cpp:11` key; `src/salmon/salmon.cpp` — `APP_NAME` (line 18), main-dialog mutex (line 759); rebrand `src/salmon/salmon.rc` version block and `src/salmon/manifest.xml` description
- [X] T020 [US2] Disable salmon upload (FR-013/Q2): read `src/salmon/upload.cpp` + its call sites/UI flow, then compile out the network path (no POST to any server; keep local minidump write); remove/repoint the send-report affordance to `https://github.com/newtcommander/newtcommander/issues`
- [X] T021 [P] [US2] mdview WebView2 cache dir: `src/plugins/mdview/viewer.cpp:74` → `…\\Newt Commander\\mdview.WebView2`

**Checkpoint**: build passes; quickstart §4 registry audit clean; no reference to `reports.altap.cz` compiled in.

---

## Phase 5: US3 — New visual identity everywhere (P3)

**Goal**: About + splash redesign with theme support; icon assets already wired (Phase 1 replaced the .ico/SVG payloads in place).
**Independent test**: quickstart §2 + §3 About/splash in both themes.

- [X] T022 [US3] Rework `src/logo.cpp` splash (`CSplashScreen::PrepareBitmap`) and About background (`AboutAndEvalDlgCreateBkgnd`): render new `IDB_LOGO_HAND` (icon SVG) right-aligned, draw the "Newt " + "Commander" wordmark with GDI (Segoe UI bold, two lockup colors) in place of `IDB_LOGO_TEXT`/os.svg, place the brand gradient band, add `#include "themes.h"` + `IsDarkThemeActive()` branching for background/text colors per contract (dark `#0A1424`/`#EAF2FB`/`#F97316`; light white/`#0A1424`/`#EA6A0B`); update `WM_CTLCOLORSTATIC` colors (lines ~438-448) and `HGradientBkBrush` (line 323) for both themes
- [X] T023 [US3] Verify/adjust the About (`src/lang/lang.rc` IDD_ABOUT statics) and splash (`src/salamand.rc` IDD_SPLASH statics) layout rects for the new wordmark/version/copyright text so nothing clips (year-split line may need widening or a second line)
- [X] T024 [US3] Tray/status icon path check: confirm `MainWindowIcons[]` (`src/dialogs5.cpp:2687-2690`) and `SalLoadIcon`/`LoadIcon` sites render the new `sal_r/g/b`+main icons; update `IDS_SALAMANDERICON_*` display names in `src/lang/lang.rc` if they say "Salamander"

**Checkpoint**: build passes; About/splash correct in light AND dark theme; icons correct at all sizes.

---

## Phase 6: US4 — Project develops under the Newt Commander identity (P4)

- [X] T025 [P] [US4] Amend `.specify/memory/constitution.md` → v2.0.0: title "Newt Commander Constitution", Principle II re-anchored (baseline Newt Commander 0.1.0; deliberate documented break with Open Salamander 5.0 referencing spec 032), Sync Impact Report updated
- [X] T026 [P] [US4] Update `README.md`: replace the feature-030 caveat (app still branded Open Salamander) with completed-rebrand statement (`newtcommander.exe`, version 0.1.0, own registry root); add `https://newtcommander.org`
- [X] T027 [P] [US4] Update `CLAUDE.md` project context: product name Newt Commander, exe `newtcommander.exe`, version 0.1.0, registry root, note that source/solution names intentionally keep `salamand*` (FR-016); update "Current Phase" wording
- [X] T028 [P] [US4] Add a short historical-context note at the top of `architecture/01-project-overview.md` only (docs describe the Open Salamander codebase; project now ships as Newt Commander — D26)

---

## Phase 7: Polish & full verification

- [X] T029 Per-plugin version metadata sweep (FR-021): in every shipped plugin's `versinfo.rh2` set `VERSINFO_COMPANY` → `Newt Commander Project` and `VERSINFO_COPYRIGHT` per contract (sftp+mdview sole NC Authors; pictview dual `2000-2026`; others year-split keeping original start year); verify with a repo-wide grep that no shipped binary's version block still says company "Open Salamander"
- [X] T030 Repo-wide audit: `grep -ri "altap.cz"` and `grep -r "Open Salamander"` over `src/` — every remaining hit must be (a) a comment, (b) attribution required by FR-017, (c) deferred scope (setup/, help/, translations/, disabled plugins), or (d) dead code; fix stragglers that violate FR-002/FR-014
- [X] T031 `build.cmd full` clean pass; run quickstart walkthrough §1–§6: exe identity, icon surfaces (16–256), About/splash light+dark screenshots, registry audit (plant marker in `HKCU\Software\Open Salamander\5.0`, verify untouched), plugin load check (18/18), salmon identity; record results in `specs/032-newt-commander-rebrand/validation-results.md` mapped to SC-001..SC-008

---

## Dependencies & execution order

- Phase 1 (T001-T003) → prerequisite for T022 (About artwork) and final icon checks; independent of Phases 2-4 otherwise
- Phase 2 (T004-T007) → blocks every phase that builds resources (all later phases)
- US1 (T008-T014): T008 first (exe name), then T009-T013 in parallel, T014 last (touches logo.cpp lightly — coordinate with T022)
- US2 (T015-T021): independent of US1; T020 after T019 (same files)
- US3 (T022-T024): after Phase 1 + Phase 2; T022 after T014 (both touch logo.cpp)
- US4 (T025-T028): anytime after US1 lands (docs must describe reality)
- Polish (T029-T031): last; T031 is the release gate

**MVP scope**: Phases 1+2+US1 (branded, versioned `newtcommander.exe`). US2 completes the separation mandate; US3 the visual promise; US4 governance.

**Parallel opportunities**: T006+T007; T009-T013; T016-T019+T021; T025-T028.
