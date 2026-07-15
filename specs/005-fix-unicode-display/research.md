# Research: Correct Display of Unicode File Names in Dialogs and Text Fields

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)
**Date**: 2026-07-15

## Root cause (verified on live repro)

The two test directories both display as `č-dir` in the panel but
mojibake in the F2 Rename dialog:

| Stored form | UTF-8 bytes of the accent | Shown as (CP1250) |
|---|---|---|
| NFC `č` (U+010D) | `C4 8D` | `ÄŤ` |
| NFD `c`+U+030C | `63` + `CC 8C` | `cĚŚ` |

Mechanism: standard Win32 controls (Edit, ComboBox, ListBox, Static,
ListView, …) are **Unicode windows even inside ANSI-created dialogs**.
Narrow text sent through an ANSI message is converted bytes→UTF-16 by
USER32 using CP_ACP. Feature 004 made all internal narrow name/path
buffers UTF-8, and converted many UI crossings to W APIs
(`CTransferInfo::EditLine`, panel drawing, titles, Find results, …) —
but a long tail of crossings still uses ANSI messages, and every one of
them mojibakes UTF-8 names.

The trigger for the reported bug: `CCopyMoveDialog::Transfer`
(src/dialogs3.cpp:419-444) has two branches. The no-history branch
uses the *fixed* `ti.EditLine`. The history branch — which F2 Rename
and F5/F6 Copy/Move always take — bypasses it with raw
`SendMessage(hWnd, WM_SETTEXT/WM_GETTEXT, …, Path)` plus
`LoadComboFromStdHistoryValues` (`CB_ADDSTRING` ANSI). Feature 004's
verification could not drive the F2 dialog by automation (UIPI, see
004 validation-results.md), which is how this branch slipped through.

Read-back is equally broken and is the *dangerous* half: `WM_GETTEXT`
(ANSI) converts the edit text UTF-16 → CP_ACP. Typed characters
outside the ACP become `?`; in-ACP accented characters come back as
single ANSI bytes that are **invalid UTF-8** and corrupt the rename /
target path / history entry. (For the unedited pre-filled name the
corruption happens to round-trip back to the original bytes, so the
current build's no-op rename works by accident.)

**Secondary defect class**: byte-level name-validation loops predating
UTF-8 compare `*s >= 32` on **signed** `char`; every UTF-8 byte
≥ 0x80 is negative, so valid Unicode names are rejected as "invalid
name" even once display is fixed. Confirmed sites:
`CFilesWindow::RenameFileInternal` (src/fileswn5.cpp:2113-2116),
`CSalamanderGeneral::ViewFileInPluginViewer` (src/zip.cpp:2458-2460),
`MoveFileToCache`/`GetFileFromCache` (src/zip.cpp:3190-3192), plus
plugin copies (src/plugins/demoplug/fs2.cpp:933,
src/plugins/wmobile/fs2.cpp:524, src/plugins/pictview/render1.cpp:2807).

**Tertiary defect**: `CTruncatedString::TruncateText`
(src/salamdr4.cpp:157-…) measures and truncates the UTF-8 subject text
with ANSI GDI (`GetTextExtentExPoint` A) at byte granularity — wrong
widths, and a cut mid-sequence produces invalid UTF-8 that defeats the
already-converted subject display (SalU8ToWAlloc returns NULL → ANSI
fallback → mojibake).

## Decisions

### D1 — Fix at shared choke points, per-site edits only for direct messages

**Decision**: Repair the shared classes/helpers through which most
text flows (`CMessageBox`, `CStaticText`, `CStatusWindow`, custom menu
draw, `LoadComboFromStdHistoryValues`, `CTruncatedString`, plugin-side
`winliblt CTransferInfo::EditLine` and `lukas HistoryComboBox`), then
convert the remaining direct `WM_SETTEXT`/`WM_GETTEXT`/`CB_*`/`LVM_*`
sites individually using new central helpers.
**Rationale**: one fix in `CMessageBox` heals every message box; same
for the other framework classes. Mirrors 004's approach (fix
`EditLine`, panel draw, titles centrally).
**Alternatives considered**: (a) converting every dialog to
`UnicodeWnd`/`DialogBoxParamW` — rejected: much larger blast radius,
would force W procs everywhere while controls are already Unicode;
(b) switching the process ACP to UTF-8 via manifest — already rejected
in 004 (R3) for third-party plugin safety; unchanged.

### D2 — Central UTF-8 window-text helpers (core + plugin SDK)

**Decision**: Add small helpers next to the existing conversion layer
and use them at every remaining site:
core (`src/common/winlib.{h,cpp}` or salamdr): `SalSetWindowTextU8`,
`SalGetWindowTextU8`, `SalSetDlgItemTextU8`, `SalGetDlgItemTextU8`,
`SalComboAddStringU8` (+ listview item text setter where needed);
plugin SDK (`src/plugins/shared/`): equivalents built on
`splunicode.h` (`SplU8ToWAlloc`/`SplWToU8`), replacing the ad-hoc
per-plugin `SetDlgItemTextU8` copies (zip, splitcbn, pictview keep
working; new code uses the shared ones).
**Rationale**: three plugins already invented private copies of the
same helper — the pattern is proven; centralizing prevents a fourth.
**Alternatives**: keep copying per-site conversion code — rejected
(the copy-paste is exactly how the History branch got missed).

### D3 — Read-back always via W (`GetWindowTextW` + `SalWToU8`)

**Decision**: every control→program crossing of a name field reads
wide and converts to UTF-8, sized for worst-case 3×WCHAR expansion;
follow the fixed `EditLine`'s established fallback semantics.
**Rationale**: fixes silent corruption (FR-003/FR-004); the pattern
already exists at common/winlib.cpp:1065-1082.

### D4 — Unsigned-byte rule for name-validation loops

**Decision**: rewrite the `*s >= 32` loops to treat bytes as
`unsigned char`; control-character rejection applies only to ASCII
0x01–0x1F; bytes ≥ 0x80 are always acceptable (UTF-8 lead/trail).
**Rationale**: minimal, behavior-preserving for ASCII; unblocks FR-004.
**Alternatives**: full UTF-8 decode + Unicode categories — rejected,
unnecessary for "reject control chars and reserved punctuation".

### D5 — Truncation/measurement on UTF-16

**Decision**: `CTruncatedString::TruncateText` converts to UTF-16
once, measures with `GetTextExtentExPointW`, truncates at WCHAR
boundaries (never splitting a surrogate pair), and either stores a
parallel wide result or re-encodes valid UTF-8.
**Rationale**: correct widths and never produces invalid UTF-8 (D5
protects the D1/D2 display fixes).

### D6 — History rings stay UTF-8; invalid entries dropped at load

**Decision**: dialogs write history entries from the W read-back
(valid UTF-8 by construction). Registry load drops entries that are
not valid UTF-8 (entries corrupted by builds carrying this bug).
**Rationale**: consistent with 004's protective handling of legacy
config strings; no migration heuristics for ambiguous ACP bytes.
**Alternatives**: ACP→UTF-8 re-encode on load — rejected: cannot be
distinguished reliably from valid UTF-8, risks corrupting good data.

### D7 — File-open/save browse dialogs go W

**Decision**: the `GetOpenFileNameA`/`GetSaveFileNameA` browse helpers
feeding name fields (dialogs.cpp:1843, execute.cpp:1705/2130 area)
switch to the W structs/calls with UTF-8 conversion at the boundary.
**Rationale**: they currently inject ACP bytes into UTF-8 fields —
same contract violation, and W common dialogs are also long-path
capable (aligns with 004 R2).

### D8 — Plugin shared UI layer fixed once, SDK contract clarified

**Decision**: fix `src/plugins/shared/winliblt.cpp`
`CTransferInfo::EditLine(char*)` (WM_SETTEXT :1063, WM_GETTEXT :1071)
and `src/plugins/shared/lukas/utildlg.cpp` `HistoryComboBox`
(:52/:88/:95) with Spl* conversions; fix core
`LoadComboFromStdHistoryValues` (salamdr6.cpp:390) which also serves
plugins via `CSalamanderGeneral`. Document in the SDK contract note
that narrow dialog text in interface 104 is UTF-8 and the shared
helpers now convert (spl_gen.h comment update).
**Rationale**: covers most DEFECTIVE plugins in one place; regedt's
`EditLineW` and zip/splitcbn's local wrappers prove both fix shapes.
**Note**: winliblt/lukas are compiled into each plugin — the fix takes
effect on rebuild of the bundled plugins (all in this solution);
already-shipped third-party binaries are unaffected (they keep their
own compiled copy — no ABI change).

## Audit inventory (FR-005)

Verdicts: DEFECTIVE = unconverted UTF-8 name crossing; OK = already
W-safe or cannot carry names; UNCERTAIN = to be resolved during
implementation (default: fix if it can carry a name).

### A. Core — shared framework classes (fix once, heals many surfaces)

| # | Surface | Sites | Verdict |
|---|---------|-------|---------|
| A1 | `CMessageBox` (every SalMessageBox with a name in text/caption) | msgbox.cpp:384,387,588,598,613,809,837 | DEFECTIVE |
| A2 | `CStatusWindow` — panel directory line + info line (path display, hot-track segments, byte offsets) | stswnd.cpp:119,188,274,610,1004-1104,1802 | DEFECTIVE |
| A3 | `CStaticText` — progress-dialog Source/Target/Operation paths and other name statics | gui.cpp:548,616,1132,1153 (fed from dialogs.cpp:473-486) | DEFECTIVE |
| A4 | Custom menu system — dir-history, drive (Alt+F1/F2), hot paths, user menu, plugin menus | menu3.cpp:935-1001, menubar.cpp:258, menu1.cpp:331-345, menu2.cpp:1019-1020 | DEFECTIVE |
| A5 | Toolbars with name text (hot-path bar, drive labels) | toolbar2.cpp:299,633,639; toolbar3.cpp:498; toolbar4.cpp:1421 | DEFECTIVE |
| A6 | Tooltips (panel long-name tooltip, dir-line tooltip) | tooltip.cpp:323,640 (+ TTN_GETDISPINFO A: mainwnd3.cpp:5077, viewer3.cpp:561) | DEFECTIVE/UNCERTAIN |
| A7 | `LoadComboFromStdHistoryValues` (`CB_ADDSTRING` ANSI; also SDK-exposed) | salamdr6.cpp:383-391 (fwd: zip.cpp:4126) | DEFECTIVE |
| A8 | `CTruncatedString::TruncateText` (byte truncation/measurement) | salamdr4.cpp:157-… | DEFECTIVE |
| A9 | Command-line combo (`CEditWindow`): execute read-back, set-text, history fill, word-ops | editwnd.cpp:219,375,561,1332,1711,1986,1997 | DEFECTIVE |

### B. Core — dialog transfer/message sites

| # | Surface | Sites | Verdict |
|---|---------|-------|---------|
| B1 | Rename / Copy / Move (`CCopyMoveDialog::Transfer`, history branch) — **the reported bug** | dialogs3.cpp:429,431,435 | DEFECTIVE |
| B2 | Copy/Move with options (`CCopyMoveMoreDialog::Transfer`) | dialogs3.cpp:618,620,624 | DEFECTIVE |
| B3 | Edit New File default name read-back | dialogs3.cpp:555 | DEFECTIVE |
| B4 | Change Directory (Shift+F7) | dialogs3.cpp:1185,1187,1191 | DEFECTIVE |
| B5 | Make File List (path combo + read-backs) | dialogs.cpp:1873,1878,1883,1900,1919,2005,2008 | DEFECTIVE |
| B6 | Select/Deselect by mask | dialogs2.cpp:538,563,565,569 | DEFECTIVE |
| B7 | Find dialog: Named/Look-in/Containing read-backs + LoadControls | finddlg1.cpp:1685,1699,1749-1753 (2758,3551 UNCERTAIN) | DEFECTIVE |
| B8 | Convert/Filter mask editlines | dialogs3.cpp:113,117,316,320 (77 UNCERTAIN) | DEFECTIVE |
| B9 | User-menu config: command/arguments/init-dir | dialogs4.cpp:2148-2153,2178-2182; dialogs2.cpp:1229,1233 | DEFECTIVE |
| B10 | External viewer/editor + pack config: command/init-dir | dialogs5.cpp:2056-2084,2443-2470 | DEFECTIVE |
| B11 | Pack/associations config pages (commands, viewer/edit combos) | dialogsp.cpp:157-232,606-661,949-1048,1117-1150 | DEFECTIVE |
| B12 | Plugin-FS quick-rename read-back | dialogs3.cpp:1258 | DEFECTIVE |
| B13 | Network path static (UNC path) | dialogs3.cpp:1804 | DEFECTIVE |
| B14 | Remaining ANSI subject statics (pack/unpack dialogs) | dialogs3.cpp:1972,2148 | DEFECTIVE |
| B15 | ListViews with names/paths: Disconnect/Connections, Shared dirs, hot-path list, view templates + in-place edits, packers custom draw | dialogs6.cpp:506-510,1294,1298; dialogs4.cpp:1022,1508-1518,2768,3060-3072; packac.cpp:186 | DEFECTIVE |
| B16 | Browse helpers on ANSI common dialogs feeding name fields | dialogs.cpp:1843; execute.cpp:1705,1712,2130,2157 | DEFECTIVE (per D7) |
| B17 | Misc UNCERTAIN: salamdr3.cpp:3150,3635,3802,3804; salamdr2.cpp:223,225; dialogs2.cpp:837,840; dialogs5.cpp:1422,2917,2966; properties-dialog name control; find status bar finddlg1.cpp:2988; codetbl.cpp:668; shellsup.cpp:2028,2091; conversion-tables list dialogs3.cpp:2866-2870; icon-overlay/plugin lists dialogs6.cpp:2527-2530, plugins2.cpp:1055-1062 | UNCERTAIN — resolve in impl. |

### C. Core — name-validation loops (signed-char defect)

| # | Site | Effect | Verdict |
|---|------|--------|---------|
| C1 | fileswn5.cpp:2113-2116 `RenameFileInternal` | F2 rename to non-ASCII name → "invalid name" error | DEFECTIVE |
| C2 | zip.cpp:2458-2460 `ViewFileInPluginViewer` | viewing Unicode-named archive entries fails | DEFECTIVE |
| C3 | zip.cpp:3190-3192 `MoveFileToCache`/`GetFileFromCache` | plugin file-cache rejects Unicode names | DEFECTIVE |

### D. Plugins — shared infrastructure

| # | Surface | Sites | Verdict |
|---|---------|-------|---------|
| D1 | `winliblt CTransferInfo::EditLine(char*)` (used by ftp, renamer, undelete, filecomp, pictview, regedt path fields, wmobile, uniso, checksum, dbviewer, nethood, demoplug, …) | plugins/shared/winliblt.cpp:1063,1071 | DEFECTIVE |
| D2 | `lukas HistoryComboBox` (renamer, ftp, dbviewer) | plugins/shared/lukas/utildlg.cpp:52,88,95 | DEFECTIVE |
| D3 | SDK docs: no note that dialog `char*` text is UTF-8 | spl_gen.h (helper docs) | Doc gap |

### E. Plugins — individual sites (beyond D1/D2)

| # | Plugin | Sites | Verdict |
|---|--------|-------|---------|
| E1 | ftp | bookmark listbox LB_INSERTSTRING/LB_ADDSTRING dialogs1.cpp:894,1199,1231; narrow SetDlgItemText fields | DEFECTIVE |
| E2 | undelete | dialogs.cpp:631,681 (via D1) | DEFECTIVE |
| E3 | filecomp | dialogs.cpp:105-106 (via D1) | DEFECTIVE |
| E4 | pictview | dialogs.cpp:1128,1835,1889 (via D1); render1.cpp:2807 (C-loop) | DEFECTIVE |
| E5 | regedt | dialogs.cpp:950-983,1222,1234 file-path fields still narrow | DEFECTIVE (partial) |
| E6 | wmobile | narrow SetDlgItemText (7×) + fs2.cpp:524 (C-loop) | DEFECTIVE (likely) |
| E7 | renamer / dbviewer | via D1+D2 (mask/new-name/history) | DEFECTIVE |
| E8 | demoplug | dialogs.cpp:250, fs2.cpp:933 | DEFECTIVE (SDK sample — fix as documentation) |
| E9 | uniso, checksum, nethood, folders, uncab, unarj, unrar, tar, pak, unlha, unchm, unfat, unole, unmime | name fields not U8-wrapped where present | UNCERTAIN — sweep in impl. |
| E10 | zip, splitcbn (local `SetDlgItemTextU8`), regedt key names (`EditLineW`, `LVN_GETDISPINFOW`), 7zip (names via core) | — | OK (reference patterns) |

### F. Already-correct reference surfaces (regression guard)

`CTransferInfo::EditLine`/`EditLineW` (winlib.cpp:1033-1113), panel
name drawing (fileswn4.cpp), in-place quick rename (fileswn5.cpp:2581+),
main-window title (mainwnd1.cpp:1907), viewer title (viewer3.cpp:52),
CopyMove subject (dialogs3.cpp:466), Drive Info dialog
(dialogs3.cpp:1315+), Change-Icon + hot-path editlines (dialogs3/4),
Find results + Find log lists (finddlg1/2). These must stay intact
(SC-004) and serve as the coding pattern for the fixes.

## Verification research

- The F2 dialog cannot be driven by `SendKeys` automation under UIPI
  (004 finding). Verification uses the quickstart manual matrix plus,
  where possible, `PostMessageW`-driven automation of the panel as in
  004; the no-op rename check (SC-001) is scriptable by comparing
  directory code points before/after.
- Build check: `build.cmd full` (all 90 projects incl. plugins, since
  winliblt/lukas are compiled into plugins).
