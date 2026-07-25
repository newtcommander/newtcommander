# Tasks: Manual Brand Asset Replacement

**Input**: Design documents from `/specs/035-manual-brand-assets/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Tests**: Not requested — verification is per-story run verification (quickstart.md) plus `gen_icons.py --verify`.

**Organization**: Grouped by user story; US1 additionally carries the variant removal (FR-003) because the deleted `sal_r/g/b.ico` are outputs of the same icon pipeline and the build breaks if resources reference them after regeneration.

## Format: `[ID] [P?] [Story] Description`

---

## Phase 1: Setup

- [X] T001 Record SHA-256 baselines of the 4 shipped ICOs (`src/res/salamand.ico`, `src/salmon/res/salmon.ico`, `src/setup/res/setup.ico`, `src/setup/remove/icon1.ico`) into the scratchpad for the T004 bit-identity check; confirm `python -c "import PIL"` works on this machine

---

## Phase 2: Foundational (blocks US1 + US2)

**Purpose**: The new source-asset layout and the reworked packer are prerequisites for both the icon story and the artwork story.

- [X] T002 Migrate `tools/brand/` source layout: `git mv` `png/newt-commander-icon-{16,24,32,48,64,128,256}.png` → `tools/brand/icon-<N>.png`, `png/newt-commander-icon-1024.png` → `tools/brand/icon-master.png`, `png/newt-commander-icon-512.png` → `tools/brand/about.png`; remove the emptied `png/` directory and delete `tools/brand/logo.svg`
- [X] T003 Rewrite `tools/brand/gen_icons.py` per `contracts/gen-icons-cli.md`: master+override input model with Lanczos downscale, 4 ICO targets only (drop `sal_r/g/b`, `recolor()`, `ORANGE_BAND`/`SAT_THRESHOLD`/`HUE_*`, `STATE_SIZES`), copy `about.png` → `src/res/logo.png`, input validation with file-naming error messages, `--verify` extended to check `logo.png`
- [X] T004 Run `python tools/brand/gen_icons.py` then `--verify`: the 4 ICOs must be bit-identical to the T001 baselines (same source pixels, same encoder), `src/res/logo.png` must exist and be PNG-signed

**Checkpoint**: New asset pipeline works and reproduces today's shipped icons exactly.

---

## Phase 3: User Story 1 — Replace the application icon by swapping image files (P1) 🎯 MVP

**Goal**: Icon swap = replace `icon-master.png`, run packer, rebuild; red/green/blue variants removed (FR-003).

**Independent Test**: Quickstart §3 (window/taskbar/Explorer icon, config page, stale-registry fallback) + §4 swap dry run for the icon.

- [X] T005 [P] [US1] Delete `src/res/sal_r.ico`, `src/res/sal_g.ico`, `src/res/sal_b.ico` (`git rm`) and remove their `<Image>` items from `src/vcxproj/salamand.vcxproj` and `src/vcxproj/salamand.vcxproj.filters`
- [X] T006 [P] [US1] Remove `IDI_SALAMANDER_RED/GREEN/BLUE` defines from `src/resource.rh2` and their `ICON` statements from `src/salamand.rc2`
- [X] T007 [US1] Shrink the variant machinery: `src/cfgdlg.h` `MAINWINDOWICONS_COUNT` 4 → 1; `src/dialogs5.cpp` `MainWindowIcons[]` down to the default entry and delete the `IDC_TITLEBAR_ICON_INDEX` combo fill/transfer/HDPI-rebuild code in `CCfgPageMainWindow`; verify `src/dialogs4.cpp` `GetMainWindowIconIndex()` clamp still covers stale registry/`-i`/tasklist values
- [X] T008 [US1] Remove the " Main Window Icon " groupbox, "Ic&on color:" label, `IDC_TITLEBAR_ICON_INDEX` combobox and shortcut-hint static from `IDD_CFGPAGE_MAINWINDOW` in `src/lang/lang.rc`; drop `IDC_TITLEBAR_ICON_INDEX` from `src/lang/lang.rh` and the now-unused `IDS_SALAMANDERICON_RED/GREEN/BLUE` strings from `src/lang/lang.rc`/`lang.rh`
- [X] T009 [US1] Build (`build.cmd`) and run-verify per quickstart §3: new-pipeline icon in window top-left, taskbar and Explorer; config page lays out cleanly without the combo; `reg add … "Main window icon index" /d 2` → app starts with default icon, no error

**Checkpoint**: US1 delivers the MVP — icon fully hand-swappable, variants gone.

---

## Phase 4: User Story 2 — Replace About/splash artwork by swapping one image (P2)

**Goal**: `about.png` swap → PNG drawn (WIC, aspect-fit) in About dialog and splash screen; nanosvg artwork path retired.

**Independent Test**: Quickstart §3 About/splash checks + §4 artwork swap dry run.

- [X] T010 [P] [US2] Create `src/pngimage.h` + `src/pngimage.cpp`: `CPngImage` with `Load(int resID, int maxWidth, int maxHeight)` (WIC: resource memory stream → decode → `32bppPBGRA` convert → Fant-scaler to aspect-fit target → top-down premultiplied DIB), `GetSize()`, `AlphaBlend(HDC, x, y, w, h)`; add both files to `src/vcxproj/salamand.vcxproj`(+`.filters`) and ensure `windowscodecs.lib` is linked
- [X] T011 [P] [US2] Resource swap: in `src/resource.rh2` replace `IDB_LOGO_HAND` with `IDB_LOGO_IMAGE`; in `src/salamand.rc2` replace the `logo.svg` RCDATA line with `IDB_LOGO_IMAGE RCDATA "res\\logo.png"`; `git rm src/res/logo.svg`; register `src/res/logo.png` in `salamand.vcxproj`(+`.filters`) resource items
- [X] T012 [US2] Rework `src/logo.cpp`: splash `PrepareBitmap()` and `AboutAndEvalDlgCreateBkgnd()` load `CPngImage` (`IDB_LOGO_IMAGE`) instead of `svgHand`/`IDB_LOGO_HAND`, preserving current placement rects and aspect-fit; gradient strips (`IDB_LOGO_GRAD`/`IDB_ABOUT_GRAD`) stay SVG
- [X] T013 [US2] Build and run-verify: About artwork (light + dark theme) and splash artwork render undistorted from PNG; extreme-aspect test image scales without overlapping texts (edge case)

**Checkpoint**: Artwork swap is a one-file operation with zero renderer knowledge.

---

## Phase 5: User Story 3 — Splash copyright on two lines (P3)

**Goal**: Splash shows "Copyright © 1997-2026 Open Salamander Authors" / "© 2026 Newt Commander Authors" on two full-visible lines; VERSIONINFO string untouched (FR-010).

**Independent Test**: Quickstart §3 splash checks + exe file-properties check.

- [X] T014 [P] [US3] Add `VERSINFO_COPYRIGHT1` and `VERSINFO_COPYRIGHT2` to `src/versinfo.rh2` with a comment stating the invariant `COPYRIGHT1 + ", " + COPYRIGHT2 == VERSINFO_COPYRIGHT` (year rule updates all three together); leave `VERSINFO_COPYRIGHT` unchanged
- [X] T015 [P] [US3] `src/salamand.rc` `IDD_SPLASH`: keep copyright line 1 at (8,73), add `LTEXT "",IDC_SPLASH_COPYRIGHT2,8,83,237,8`, move `IDC_SPLASH_STATUS` to (8,93), grow dialog height 94 → 104; define `IDC_SPLASH_COPYRIGHT2` in `src/salamand.rh`
- [X] T016 [US3] `src/logo.cpp` `CSplashScreen`: add `Copyright2R` member (mainwnd.h/cfgdlg.h — wherever `CopyrightR` is declared), fetch it via `GetDlgItemRectAndDestroy`, paint `VERSINFO_COPYRIGHT1` and `VERSINFO_COPYRIGHT2` bold on the two lines instead of the single `VERSINFO_COPYRIGHT`
- [X] T017 [US3] Build and run-verify: both splash lines fully visible, status text below, no overlap with artwork/gradient; `newtcommander.exe` Properties → Details → Copyright still the full single-line string

**Checkpoint**: All three functional stories verifiable independently.

---

## Phase 6: User Story 4 — Self-service asset guide (P2; depends on US1 + US2 final shape)

**Goal**: `tools/brand/README.md` alone suffices for any future graphics swap (FR-008, SC-006).

**Independent Test**: Perform a swap end-to-end using only the README.

- [ ] T018 [US4] Rewrite `tools/brand/README.md` as the asset guide per `contracts/asset-layout.md`: replaceable-file table (path, where it appears, format/size), the 3-step swap procedure, validation/error behavior, explicit non-replaceable list (wordmark, gradient strips); drop all obsolete nanosvg/hue-remap/png/-set content
- [ ] T019 [US4] Guide dry run (quickstart §4): following README only — swap `icon-master.png` + `about.png` with test images (overrides deleted), regenerate, rebuild, confirm new artwork in all six identity surfaces + About + splash; break-test missing `icon-master.png` → `error:` naming the file, exit ≠ 0; restore original assets, regenerate, rebuild, re-verify

**Checkpoint**: "No AI needed" is demonstrated, not assumed.

---

## Phase 7: Polish & Cross-Cutting

- [ ] T020 [P] Stale-reference sweep: repo-wide grep for `logo.svg`, `IDB_LOGO_HAND`, `sal_r`, `sal_g`, `sal_b`, `IDI_SALAMANDER_RED|GREEN|BLUE`, `IDC_TITLEBAR_ICON_INDEX`, `IDS_SALAMANDERICON_(RED|GREEN|BLUE)`, `ORANGE_BAND`, `newt-commander-icon-` must return no live hits (specs/ history excluded); update the CLAUDE.md "Brand assets" bullet to the new layout; clang-format touched C++ sources
- [ ] T021 Write `specs/035-manual-brand-assets/validation-results.md` mapping SC-001…SC-006 to the evidence gathered in T004/T009/T013/T017/T019 (repo convention from features 033/034)

---

## Dependencies & Execution Order

- **Phase 1 → Phase 2**: baseline before regeneration.
- **Phase 2 blocks US1 and US2** (both consume the new packer/layout); it does NOT block US3.
- **US1 (Phase 3)**: after Phase 2. T005/T006 parallel; T007 → T008 → T009.
- **US2 (Phase 4)**: after Phase 2; independent of US1. T010/T011 parallel → T012 → T013.
- **US3 (Phase 5)**: independent of Phases 2–4 in content, but T012 and T016 both edit `src/logo.cpp` — execute US3 after US2 (or accept a merge in that file). T014/T015 parallel → T016 → T017.
- **US4 (Phase 6)**: after US1 + US2 (documents their final workflow).
- **Phase 7**: last.

### Parallel opportunities

- T005 + T006 (different files); T010 + T011; T014 + T015; T020 alongside T021.
- Single-developer flow is effectively sequential; build+verify tasks (T009, T013, T017, T019) each gate their story.

## Implementation Strategy

MVP = Phases 1–3 (icon swap working, variants removed) — ship-worthy alone.
Then US2 → US3 → US4 as independent increments, each ending in a run-verified
checkpoint; Polish closes with the stale-reference sweep and SC evidence.
