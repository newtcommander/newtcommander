# FR-009 Inventory: UTF-8 values reaching legacy display calls

**Feature**: `043-fix-ui-text-encoding` · **Date**: 2026-07-27
**Instrument**: `tools/check_encoding.py` (5 rules) + three independent code surveys

## Baseline vs final

| Rule | Before | After |
|---|---|---|
| `cp-acp-display` | 0 | 0 |
| `mixed-composition` | 0 | 0 |
| `dead-dispinfow` | 0 | 0 |
| `utf8-to-legacy-sink` (new) | **42** | 0 unannotated |
| `ansi-template-caption` (new) | **11** | 0 unannotated |

## Fixed

**Composed captions** (ANSI `LoadStr` template + UTF-8 name → `CTruncatedString`),
repaired by moving template and ingredients to `LoadStrU8` / `u8=TRUE`:

| Site | Surface |
|---|---|
| `fileswn5.cpp:2383` | **F2 Quick Rename** (reported) |
| `fileswn8.cpp:474` | **F5 Copy / F6 Move / F8 Delete** (reported) |
| `fileswna.cpp:92` | F5 / F6 / Delete on a plugin file system |
| `fileswn7.cpp:469` | copy out of / delete from an archive |
| `fileswn7.cpp:1382` | Pack files (Alt+F5) |
| `fileswn7.cpp:1580` | add-to-existing-archive confirmation |
| `fileswn7.cpp:1724` | Unpack archive (Alt+F6) |
| `fileswn5.cpp:406` | NTFS compress / encrypt confirmation |
| `finddlg2.cpp:1949` | Find log → Ignore |
| `fileswn5.cpp` ×3 | template-only captions (were correct only by accident of the fallback) |

**Legacy sinks**, repaired with the existing `Sal*U8` helpers:

| Site | Surface | Helper |
|---|---|---|
| `dialogs2.cpp:889` | language selector list (reported) | `SalListViewSetItemTextU8` |
| `dialogs4.cpp:871` | configuration language field (reported) | `SalSetDlgItemTextU8` |
| `dialogs.cpp` ×3 | overwrite-confirmation size/date/time; beta date | `SalSetDlgItemTextU8` |
| `dialogs2.cpp` ×9 | occupied-space numbers | `SalSetDlgItemTextU8` |
| `dialogs3.cpp` ×13 | volume information, directory sizes | `SalSetDlgItemTextU8` |
| `dialogs3.cpp` ×4 | pack dialog path combo | `SalComboAddStringU8` |
| `dialogs6.cpp` ×2 | reparse-point details, drive-not-ready | `SalSetDlgItemTextU8` |
| `finddlg1.cpp` ×3 | Find window caption | `SalSetWindowTextU8` |
| `finddlg1.cpp` ×1, `finddlg2.cpp` ×1 | Find status bar | `SalStatusSetTextU8` (new) |
| `finddlg2.cpp` ×3 | Find options fields | `SalSetDlgItemTextU8` |
| `dialogs3.cpp` ×2 | copy/move/rename dialog titles | `SalSetWindowTextU8` |
| `plugins3.cpp` ×2 | plugin subject label — had **no** wide path at all | `SalSetWindowTextU8` |
| `fileswn9.cpp` ×3 | drag image — had **no** wide path at all | `DrawTextW` + fallback |

**New helper**: `SalStatusSetTextU8` (`src/common/winlib.*`) — `SB_SETTEXTW` with a
legacy fallback, filling the one gap in the `Sal*U8` family.

**Reverted**: `dialogs5.cpp` — a feature 042 conversion that was wrong (the
substituted value is a local copy of an ANSI plugin name).

## Recorded, deliberately not changed (annotated in place)

| Sites | Value | Reason |
|---|---|---|
| `dialogs5.cpp` ×4, `fileswn7.cpp` ×2, `plugins1.cpp` ×8 | plugin name / DLL name | plugin-supplied metadata; this application does not control its encoding. Revisit when the plugin metadata encoding is defined. |
| `dialogs6.cpp` ×1 | network share name | comes from the OS share enumeration |
| `mainwnd3.cpp` ×1 | configuration name | chosen in-app, not a file name |
| `dialogs2.cpp` ×1 | `.slg` file name | ANSI, from `FindFirstFile` — must **not** be treated as UTF-8 |
| `fileswn5.cpp` ×1 | quick-rename window fallback | documented legacy fallback of a wide path |

## Out of scope, recorded

- `src/tserver/tablist.cpp` — separate helper executable, built Unicode.
- `src/plugins/regedt/finddlg2.cpp:932` — an `LVN_GETDISPINFOW` handler with no
  `NF_REQUERY` anywhere in that plugin, i.e. dead code. Plugin, so not modified.
- `msgbox.cpp` checkbox/hint/url text is ANSI while the body is UTF-8 — an
  asymmetry in the public `MSGBOXEX_PARAMS`, worth normalising in a future feature.
