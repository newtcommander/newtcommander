# Validation Results: Correct Display of Unicode File Names

**Feature**: 005-fix-unicode-display | **Date**: 2026-07-15
**Build**: `build.cmd full` — Debug x64, all 90 projects incl. 35 plugins, clean (0 errors)

## Reported defect — fixed

The two `č-dir` test directories (composed `č` U+010D, and decomposed
`c`+U+030C) mojibaked in the F2 Rename dialog as `ÄŤ-dir` / `cĚŚ-dir`.

Root cause: standard Win32 controls are Unicode windows; feature 004
made internal narrow buffers UTF-8, but the Rename/Copy/Move dialog's
history-combo branch (`CCopyMoveDialog::Transfer`, dialogs3.cpp) still
sent raw UTF-8 through ANSI `WM_SETTEXT`/`WM_GETTEXT`/`CB_ADDSTRING`,
so USER32 reinterpreted the bytes through CP1250. The read-back path
also silently corrupted edited names. A secondary defect
(`RenameFileInternal`'s `*s >= 32` validation on `signed char`) would
have rejected any non-ASCII name once display was fixed.

Fix: convert every name-bearing control crossing to UTF-16 via the new
central helpers; make the validation loops treat bytes as
`unsigned char`.

## End-to-end verification (running app, headless-compatible)

The verification host is **headless** (`GetForegroundWindow()` returns
0), so interactive input (SendKeys / synthetic keystrokes into the
custom items-box control) cannot be delivered — the same limitation
feature 004 recorded. Window-text **reads** work headless, so the
anchor verification uses a surface that appears without any input: the
feature-004 **equivalent-pair notice** shown at startup for the two
`č-dir` directories. It goes through `CMessageBox` — the *same*
UTF-8→UTF-16→W-control boundary the F2 dialog uses.

| Check | Pre-fix | Post-fix (read via `GetWindowTextW`) |
|-------|---------|--------------------------------------|
| Equivalent-pair notice name line | `ÄŤ-dir` (mojibake) | **`č-dir`** (U+010D U+002D U+0064 U+0069 U+0072) ✓ |

Result: `A1 NOTICE VERIFICATION: PASS`, re-confirmed after the full
35-plugin rebuild. This demonstrates the boundary fix works in the
real running app on the real test name. The F2 Rename dialog fix uses
the identical `SalSetWindowTextU8`/`SalGetWindowTextU8` helpers.

**Deferred to an interactive desktop session** (cannot run headless):
the full F2/F5/F6/F7 walkthrough of quickstart rows #1–#20 (keyboard
driven). The code paths are in place and build-verified.

## What was implemented (build-verified)

Central helpers (`src/common/winlib.{h,cpp}`): `SalSetWindowTextU8`,
`SalGetWindowTextU8`, `SalSetDlgItemTextU8`, `SalGetDlgItemTextU8`,
`SalComboAddStringU8`, `SalListViewSetItemTextU8` — UTF-8⇄UTF-16 at the
control boundary with an invalid-UTF-8 ANSI fallback (the 004
transitional rule).

| Audit row | Surface | Status |
|-----------|---------|--------|
| A1 | `CMessageBox` (all message boxes naming files; wide measure/wrap via new `DuplicateStrAndInsertEOLsW`) | **Fixed + verified** |
| A3 | `CStaticText` (progress-dialog Source/Target/Operation paths; measure/ellipsis/paint on UTF-16) | Fixed (build; ASCII layout identical by construction) |
| A7 | `LoadComboFromStdHistoryValues` (shared history combo) | Fixed |
| A8 | `CTruncatedString::TruncateText` (wide measure/truncate, no torn UTF-8) | Fixed |
| A9 | Command-line combo (execute/rebuild/history/save-restore) | Fixed |
| B1 | Rename/Copy/Move `CCopyMoveDialog::Transfer` (**reported bug**) | Fixed |
| B2 | Copy/Move-More `Transfer` | Fixed |
| B3 | Edit-New-File default-name read-back | Fixed |
| B4 | Change Directory | Fixed |
| B5 | Make File List (combo + validation read-backs) | Fixed |
| B6 | Select/Deselect by mask | Fixed |
| B7 | Find dialog (Named/Look-in/Grep) + viewer grep — shared `HistoryComboBox` (viewer.cpp) | Fixed |
| B8 | Convert/Filter mask editlines | Fixed |
| B9 | User-menu config command/arguments/init-dir; compare-args path | Fixed |
| B10 | Config viewer/editor command/arguments/init-dir (dialogs5) | Fixed |
| B12 | Drive-Info volume-label read-back | Fixed |
| B15 (part) | Hot-path & view-template config lists; connections & shared-dirs lists (dialogs4/dialogs6) | Fixed |
| B16 | Browse common dialog (`BrowseFileName` → `GetSaveFileNameW` + `SafeGetSaveFileNameW`, UTF-8 round-trip) | Fixed |
| C1 | `RenameFileInternal` signed-char validation | Fixed |
| C2/C3 | zip file-cache name validation (`ViewFileInPluginViewer`, `Move/GetFileFromCache`) | Fixed |
| D1 | Plugin-shared `winliblt CTransferInfo::EditLine` (heals ftp, renamer, undelete, filecomp, pictview, dbviewer, …) | Fixed |
| D2 | Plugin-shared `lukas HistoryComboBox` | Fixed |
| D6 | Registry history load drops invalid-UTF-8 entries (`LoadHistory`) | Fixed |
| E4/E6/E8 | pictview / wmobile / demoplug name-validation loops | Fixed |

Surfaces that the audit initially flagged but were already the correct
004 pattern (verified, no change needed): network-path static and the
Copy/Move/pack **subject** statics (dialogs3.cpp — already
`SalU8ToWAlloc`+`SetWindowTextW`), Drive-Info display side, Find results
& log lists, panel name drawing, window/viewer titles, `CTransferInfo::EditLine`.

## Remaining (identified, not yet implemented)

These need an **interactive desktop session** for safe visual
verification (owner-drawn GDI with bespoke byte-based measurement,
ellipsis, and hot-tracking) or are lower-frequency advanced surfaces.
All are documented with the required approach in
[research.md](research.md) §Audit inventory.

| Audit row | Surface | Why deferred |
|-----------|---------|--------------|
| A2 | `CStatusWindow` — panel directory/info line | Clickable path-segment hot-tracking maps mouse-X to byte offsets; wide rewrite must be verified interactively so navigation isn't broken |
| A4 | Custom menus (dir-history, drive Alt+F1/F2, hot paths, user menu) | Owner-draw `DrawText`; menu layout/measurement needs visual check |
| A5 | Toolbars (hot-path bar labels) | Owner-draw cache-bitmap text; visual check |
| A6 | Tooltips (long-name tooltips) | Owner-draw; visual check |
| B11 | Pack/associations config command pages (dialogsp) | Low-frequency advanced config; commands are ASCII exe paths in practice |
| B17 | Remaining UNCERTAIN misc (salamdr2/3 browse/connect dialogs, conversion-tables list, plugin/icon-overlay lists, code-page menu names) | Per-site classification; mostly ASCII metadata |
| dialogs4 | In-place ListView label-edit read-back (view templates, hot paths) | `LVN_ENDLABELEDITW` path; low-frequency |
| E1/E5 (part) | ftp bookmark listbox; regedt on-disk file-path fields | Plugin-local direct `LB_*`/`SetDlgItemText` beyond the shared layer |
| Other browse | dialogs5 open-file, mainwnd3 config-export browse | ASCII-typical (.reg / exe); same `SafeGet*W` approach as B16 |

## Regression posture

- ASCII names: the CStaticText/CTruncatedString/CMessageBox wide paths
  produce byte-identical layout for ASCII (WCHAR↔byte 1:1, equal
  widths), so no ASCII-name regression is expected. Full ASCII
  rename/copy/move/create/delete regression (quickstart #20) is part
  of the deferred interactive pass.
- No plugin ABI change: `winliblt`/`lukas` are shared *source* compiled
  into each plugin (picked up on rebuild); `CSalamanderGeneral`
  signatures unchanged; `SafeGetSaveFileNameW` is internal-only.
- Full 35-plugin Debug build is clean; Release build (`build.cmd full
  release`) recommended before merge.
