# Phase 1 Data Model: Fix Find Window Dark-Mode Rendering

**Feature**: 044-fix-find-dark-mode · **Date**: 2026-07-28

This feature stores no data and adds no configuration. The "model" is
the inventory of themed surfaces, the palette mappings each fix must
use, and the state rules every paint path must obey.

## Entity 1 — Find-Window Defect Surface Inventory

The acceptance checklist (spec SC-001). Each row must render dark and
readable in the Dark theme and byte-identical to today in Default.

| Surface | Control / owner | Fix | Dark rendering rule |
|---|---|---|---|
| Separator under menu bar | `IDC_FIND_LINE1` (`SS_ETCHEDHORZ`) | R1 central | etched bevel drawn as `COLOR_3DDKSHADOW` (16,16,16) / `COLOR_3DLIGHT` (58,58,58) |
| Separator beside "Search file content" | `IDC_FIND_LINE2` (`SS_ETCHEDHORZ`) | R1 central | same |
| Edge above results list | `CFindTBHeader` `WS_EX_STATICEDGE` NC frame | R2 | `ThemeDrawEdge` sunken pair, same palette |
| Advanced-options box frame | `IDC_FIND_ADVANCED_TEXT` (read-only `EDITTEXT`) | R3 central | `DarkMode_CFD` border (dark) |
| Advanced-options text, disabled state | same control, disabled when no options set | R4 central | text `COLOR_GRAYTEXT` (150,150,150) on `COLOR_BTNFACE` (45,45,45) |
| "Found Items: (N)" label | `CFindTBHeader` `WM_ERASEBKGND` `DrawText` | local | text `COLOR_BTNTEXT` (240,240,240) |
| Disabled toolbar caption ("Focus") | `CToolBar` disabled-text path | R8 central | single-pass `COLOR_GRAYTEXT`, no emboss |
| Results header labels ("Name", "Path") | `SysHeader32` child of `IDC_FIND_RESULTS` | R7 | text `COLOR_BTNTEXT` via custom draw |
| Status bar: background, part borders, size grip | `msctls_statusbar32` (`IDC_FIND_STATUS`) | R5 central | fill `COLOR_BTNFACE`; grip/bevels from dark bevel pair |
| Status bar: idle hint / results / selection texts | parts 0–2, plain text | R5 central | text `COLOR_BTNTEXT` |
| Status bar: searched-path part (owner-draw during search) | `WM_DRAWITEM` in `CFindDialog` | local | `SetTextColor(ThemeSysColor(COLOR_BTNTEXT))` |
| Progress bar during search | `PROGRESS_CLASS` child on status bar | R9 | classic renderer; track `COLOR_BTNSHADOW` (26,26,26), bar `COLOR_HIGHLIGHT` (38,79,120) |

Out-of-scope surfaces that must not regress (already correct): menu bar
and drop-downs, combo boxes, push/drop-down/arrow buttons, results-list
items and Path-column custom draw, checkboxes and labels, dialog
background, DWM dark title bar.

## Entity 2 — Palette Mappings Used (no new colors)

All colors come from the existing dark palette
(`src/common/themes_palette.h:19-46`) through the 028 accessors —
this feature defines **zero** new color values.

| Role | Index | Dark value | Contrast on its background |
|---|---|---|---|
| Surface fill | `COLOR_BTNFACE` | 45,45,45 | — |
| Primary text | `COLOR_BTNTEXT` | 240,240,240 | 11.6:1 on BTNFACE (≥4.5:1 ✓) |
| Disabled text | `COLOR_GRAYTEXT` | 150,150,150 | 4.6:1 on BTNFACE (≥3:1 ✓) |
| Bevel dark half | `COLOR_3DDKSHADOW` | 16,16,16 | n/a (decorative) |
| Bevel light half | `COLOR_3DLIGHT` | 58,58,58 | n/a (decorative) |
| Progress track | `COLOR_BTNSHADOW` | 26,26,26 | n/a |
| Progress bar | `COLOR_HIGHLIGHT` | 38,79,120 | n/a |

saltests additions assert the two text-contrast rows above for the new
surface pairs (mirroring the existing WCAG suite,
`saltests.cpp:573-700`).

## Entity 3 — Paint-Path State Rules (invariants)

Every new subclass/branch obeys the same three-state contract:

| State | Behavior |
|---|---|
| Default theme active | `DefSubclassProc` / legacy branch — native painting, bit-for-bit unchanged (spec FR-005, SC-003) |
| Dark theme active | dark paint path using `ThemeSysColor*` accessors only — never raw `GetSysColor` at a draw site |
| High Contrast on | `IsDarkThemeActive()` returns FALSE → identical to Default state (spec FR-007) |

State transitions (theme switch while the Find window is open, spec
FR-006) require no new plumbing: the decision is re-evaluated inside
every `WM_PAINT`/`WM_ERASEBKGND`/`WM_NCPAINT`/`WM_DRAWITEM`, and the
existing `WM_USER_COLORCHANGEFIND` broadcast →
`CFindDialog::OnColorsChange()` → `ThemeApplyToDialog` +
`InvalidateRect` (`finddlg2.cpp:295-318`) repaints everything. The only
transition-sensitive object is the transient progress bar (R9),
re-colored in `OnColorsChange` if alive during a switch.

## Entity 4 — Central vs. Local Fix Ledger

Traceability from spec FRs to fix locus (drives tasks.md):

| Spec FR | Fixes | Locus |
|---|---|---|
| FR-001 separators | R1 | central `themes.cpp` |
| FR-002 readable fields | R4 (disabled edit), R7 (header), local text-color fixes | mixed |
| FR-003 frames/borders | R2 (NC edge), R3 (edit border) | local + central |
| FR-004 status bar | R5 + R6 + R9, owner-draw color | central + local wiring |
| FR-005 light unchanged | passthrough invariant of every fix | all |
| FR-006 live switch | existing broadcast + per-paint re-evaluation | none new |
| FR-007 High Contrast | existing `IsDarkThemeActive()` guard | none new |
| FR-008 multi-instance | subclasses installed per-window by `ThemeApplyToDialog` | automatic |
