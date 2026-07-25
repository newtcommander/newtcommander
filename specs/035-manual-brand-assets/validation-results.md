# Validation Results: Manual Brand Asset Replacement (035)

**Date**: 2026-07-25 · **Build**: Debug x64 (`build.cmd`), all builds 0 errors
**Environment**: Windows 11, VS2022, Python 3 + Pillow 12.1.1

## Success criteria

### SC-001 — Icon swap via guide only, image files only ✅

T019 dry run followed `tools/brand/README.md` verbatim: replaced
`icon-master.png` with a visibly different test image (blue rounded square),
deleted the seven `icon-<N>.png` overrides, ran
`python tools\brand\gen_icons.py`, rebuilt. The new icon appeared in the
main-window top-left corner (screenshot `t019-window-swapped.png`). Zero
source/rc/vcxproj edits; wall-clock well under 15 minutes.

### SC-002 — About/splash artwork swap = one file ✅

Same dry run swapped `about.png` (green-circle test image) alone; after
regenerate+rebuild the splash showed the new artwork
(`t019-splash-swapped.png`), scaled with aspect preserved, no renderer
knowledge needed (plain PNG export).

### SC-003 — New artwork in 100% of identity locations ✅

All four shipped ICOs (`salamand`, `salmon`, `setup`, `icon1`) are written
by the single packer run from the same frame set (T004 log), so exe/window/
taskbar/crash-reporter/installer/uninstaller all carry the same artwork;
window + taskbar + Explorer verified live (`t009-mainwindow.png`,
`t019-window-swapped.png`).

### SC-004 — Splash copyright fully readable on two lines ✅

Splash capture (`t017-splash.png`): line 1 "Copyright © 1997-2026 Open
Salamander Authors", line 2 "© 2026 Newt Commander Authors", both complete,
bold, status line below, no overlap with artwork or accent line (dialog
grown 94→104 dlgunits).

### SC-005 — Invalid input → actionable error, nothing broken ✅

- Missing master: `error: E:\...\tools\brand\icon-master.png is missing`, exit 1.
- Wrong-size override: `error: E:\...\icon-16.png is 20x20, expected exactly 16x16`, exit 1.
- Validation runs before any write — no partial outputs (verified: ICO
  hashes unchanged after failed runs).

### SC-006 — Zero AI-assistance steps ✅

`tools/brand/README.md` documents every replaceable file (path, surface,
format, size), the 3-step procedure, error behavior and the non-replaceable
list; the T019 dry run used only the README.

## Functional requirements spot-checks

- **FR-001/001a**: master-only regeneration works (overrides deleted in
  T019); with the migrated overrides present the first run reproduced the
  previous ICOs **bit-identically** (SHA-256 `AE6D36F9…2469` before and
  after, T004) — no visual regression from the pipeline change.
- **FR-003**: variants removed everywhere (`sal_r/g/b.ico`, resource IDs,
  strings, config combo). Stale registry `Main window icon index = 2` →
  app starts with default icon, no error, value re-saved as 0 on exit
  (T009 live test). Repo-wide sweep (T020) found no live references.
- **FR-004/005/006**: About dialog (`t013-about.png`, light background) and
  splash (`t017-splash.png`, dark navy background) render the same
  `logo.png` via WIC premultiplied alpha, undistorted — both compositing
  paths covered.
- **FR-007**: see SC-005.
- **FR-009/010/011**: two-line splash display; exe → Properties → Details →
  Copyright still the full single-line string (`VersionInfo.LegalCopyright`
  check); layout accommodates the extra line (status shifted, dialog taller).

## Post-restore state

Original assets restored via git, regenerated, `--verify` all OK,
`salamand.ico` hash equals the pre-feature baseline; final rebuild
succeeded. Working tree contains only the intended feature changes.

## Notes / limitations

- The Configuration → Main Window page was verified at template+code level
  (combo removed from `IDD_CFGPAGE_MAINWINDOW`, transfer code deleted,
  build+run clean); OS focus-stealing protection blocked scripted opening
  of the dialog for a screenshot. The dialog contains only standard
  controls that were untouched otherwise.
- Screenshots live in the session scratchpad (not committed):
  `t009-mainwindow.png`, `t013-about.png`, `t017-splash.png`,
  `t019-splash-swapped.png`, `t019-window-swapped.png`.
