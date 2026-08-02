# Research — Dark Mode Stabilization (049)

Phase 0 output. Inputs: the six-track audit (`analysis/`), spec clarifications (2026-08-02),
theme-engine contracts from 028/036/044. All decisions verified against the live tree on
2026-08-02; audit claims that did not survive verification are recorded in R14.

## R1 — Public subtree re-apply helper (A1, A2, A3, A4, A6)

**Decision**: Add `void ThemeApplyToWindowTree(HWND hWnd)` to `src/themes.h|cpp`: runs
`ThemeApplyChildEnumProc(hWnd, dark)` on the window itself, then
`EnumChildWindows(hWnd, ThemeApplyChildEnumProc, dark)`. No DWM call, no `THEME_DARKENED_PROP`
stamp (those are top-level semantics of `ThemeApplyToDialog`). Works symmetrically in both
themes (the enum proc already restores light variants), idempotent (fixed subclass IDs;
`SetWindowSubclass` with same ID replaces).
**Rationale**: `SetWindowTheme` and the engine subclasses are per-HWND; every recreation path
needs one canonical re-apply primitive. `ThemeApplyToDialog` is unsuitable for child subtrees
(drags in title-bar semantics).
**Alternatives**: per-site `SetWindowTheme` calls (the `editwnd.cpp:1709` precedent) — rejected:
undiscoverable, already caused A1–A5; a CBT hook auto-theming every window — rejected in 028.

**Wiring** (each site guarded by nothing — helper is theme-aware internally):
- `CFilesBox::SetMode` (`src/filesbx1.cpp:82-90`) — after `ShowHideChilds/LayoutChilds`, call
  `ThemeApplyToWindowTree(HWindow)`. Covers every view-switch entry point (audit raw-03 B rows
  1a–1g, 2) including the header-line toggle.
- Quick rename (`src/fileswn5.cpp:~2757`) — after successful `CreateExW`.
- `CEditListBox` inline editor (`src/edtlbwnd.cpp:~485`) — after `EditLine->Create`.
- ListView label edits (`src/dialogs4.cpp` Views + Hot Paths pages) — in `LVN_BEGINLABELEDIT`
  handlers via `ListView_GetEditControl`.
- `CPackACDialog::WM_INITDIALOG` end — second `ThemeApplyToDialog(HWindow)` (Find precedent
  `finddlg1.cpp:3106-3109`; full dialog re-apply is correct here, status bar + checkbox LV).

**Colors for the panel edits**: `CFilesBox` gains `WM_CTLCOLOREDIT` → `ThemeHandleCtlColor`
routing (returns FALSE in light → `DefWindowProc`, unchanged). ListView label edits reflect
`WM_CTLCOLOREDIT` to the dialog, already routed.

## R2 — Palette rebalance: input surfaces lighter than face (C1, C2)

**Decision**: `src/common/themes_palette.h` `THEME_DARK_SYSCOLORS`:
`COLOR_WINDOW` **32,32,32 → 56,56,56** (`#383838`). No other palette entry changes; panel
(`THEME_DARK_PANEL_COLORS`) and viewer tables untouched.
**Rationale**: Implements the clarified "lighter than face" convention (face stays 45,45,45).
One entry fixes the whole class: edits, combos, list boxes, listviews, treeviews, message-box
backgrounds. Read-only/disabled edits keep the face brush (`WM_CTLCOLORSTATIC` path) — with
the field now *lighter* than the face, the classic Windows semantics (editable = field,
read-only = face) reads correctly and resolves the C2 inconsistency without code changes.
**Contrast verification** (WCAG ratios, computed):
- `COLOR_WINDOWTEXT` 240 vs 56 ≈ **10.2:1** (floor 4.5) ✓
- `COLOR_HOTLIGHT` (102,178,255) vs 56 ≈ **5.2:1** (floor 4.5) ✓
- `COLOR_GRAYTEXT` 150 vs 56 ≈ **3.9:1** (floor 3.0) ✓
All existing saltests assertions hold; add a new assertion `Luminance(WINDOW) > Luminance(BTNFACE)`
to lock the convention.
**Alternatives**: field = face + border (rejected: loses field affordance, clarification chose
lighter-than-face); keep 32 and only unify read-only (rejected in clarification).

## R3 — Hyperlink color (D1, F6)

**Decision**: `src/gui.cpp:1146`:
`SetTextColor(hDC, IsDarkThemeActive() ? ThemeSysColor(COLOR_HOTLIGHT) : RGB(0, 0, 255));`
**Rationale**: Single root cause for all 6 core + 8 plugin link sites (shared
`CStaticText::WindowProc` path). `COLOR_HOTLIGHT` dark = 102,178,255, already palette-mapped and
contrast-asserted; unread by any drawing code until now. Light theme byte-identical.
**Tests**: add saltests assertion `ContrastRatio(HOTLIGHT, RGB(0x0A,0x14,0x24)) >= 4.5` (About
navy, `TC_COLOR_NAVY` in `logo.cpp`; ≈ 8:1) with a comment tying the constant to `logo.cpp`.
**Alternatives**: new palette slot (unnecessary — HOTLIGHT is Windows' hyperlink index);
SysLink migration (no `WC_LINK` usage exists; larger change for no gain).

## R4 — Group boxes (B1)

**Decision**: New `ThemeGroupBoxSubclassProc` in `themes.cpp` (installed from the enum proc's
`Button`+`BS_GROUPBOX` branch, dark only, new subclass ID 6): full `WM_PAINT` owner draw —
etched frame via `ThemeDrawEdge(EDGE_ETCHED, BF_RECT)` on the client rect offset by half the
label height, label drawn opaque with `COLOR_BTNFACE` background and `COLOR_BTNTEXT`
(`COLOR_GRAYTEXT` when disabled) at the classic 8-px indent. Background not filled (classic
group boxes are transparent). Light theme: `DefSubclassProc` untouched.
**Rationale**: Same root cause and same remedy class as 044's `SS_ETCHED*` fix — classic chrome
paints with real system colors and never consults `WM_CTLCOLOR`. 26 controls / 15 dialogs fixed
centrally.
**Alternatives**: re-theme to `DarkMode_Explorer` (rejected — theme paints labels black, the
very defect 036 fixed by stripping); `WM_PRINTCLIENT` overlay (more fragile geometry than a
full redraw of this simple control).

## R5 — Radio buttons (B2)

**Decision**: New `ThemeRadioGlyphSubclassProc` (dark only, subclass ID 7) for
`BS_RADIOBUTTON`/`BS_AUTORADIOBUTTON`: `WM_PAINT` renders the default classic paint into a
memory DC via `WM_PRINTCLIENT` (fallback: direct `DefSubclassProc` paint-to-DC), then overlays
the glyph square (DPI-scaled 13 px, vertically centered, left edge — `BS_LEFTTEXT`/right-aligned
variants pass through untouched) with a dark-drawn radio: interior `COLOR_WINDOW`, ring
`COLOR_GRAYTEXT` (disabled) / `COLOR_BTNTEXT`, center dot `COLOR_BTNTEXT` when `BM_GETCHECK` is
`BST_CHECKED`; result blitted once (no flicker).
**Rationale**: Classic strip (036) already gives correct label text and background from
`WM_CTLCOLORSTATIC`; only the `DrawFrameControl` glyph uses hardcoded light colors. Overlaying
just the glyph preserves classic layout, focus rect, and prefix rendering exactly.
**Alternatives**: full custom paint (re-implements label layout, UI-state cues, multiline —
more risk); `DarkMode_Explorer` (black labels, see R4).

## R6 — Date/time pickers and hotkey fields (E1, E2)

**Decision**: New `ThemeGrayscaleRemapSubclassProc` (dark only, subclass ID 8) for
`SysDateTimePick32` and `msctls_hotkey32`: `WM_PAINT` renders the native control into a 32-bpp
memory DIB via `WM_PRINTCLIENT`, then remaps **grayscale** pixels (|R−G|,|G−B|,|R−B| ≤ 8)
linearly so white (255) → `COLOR_WINDOW` dark (56) and black (0) → `COLOR_WINDOWTEXT` (240);
chromatic pixels (the blue segment-selection highlight) pass through untouched; blit once.
`WM_ERASEBKGND` returns TRUE (paint covers the client).
**Rationale**: DTP/hotkey have no dark visual-style class and classic fallback still uses real
system colors; a full re-implementation would lose the DTP's per-segment edit highlight (a
functional cue — forbidden side effect). Grayscale remap preserves native behavior and the
highlight pixel-exactly, uses only documented GDI.
**Residual (recorded)**: the DTP's drop-down month calendar is an OS-drawn themed popup and
stays light — same acceptance class as common dialogs (028 boundary). Documented in spec's
edge-case expectations via quickstart.
**Alternatives**: full owner draw (loses segment highlight); `DTM_SETMCCOLOR` (ignored by the
themed control on Vista+).

## R7 — Progress bars and tooltips (E3, E4)

**Decision**:
- Enum-proc branch for `msctls_progress32`: mirror 044's `UpdateProgressBarTheme` exactly —
  dark: `SetWindowTheme(L"", L"")` + `PBM_SETBKCOLOR = ThemeSysColor(COLOR_BTNSHADOW)` +
  `PBM_SETBARCOLOR = ThemeSysColor(COLOR_HIGHLIGHT)`; light: `SetWindowTheme(NULL, NULL)` +
  both `CLR_DEFAULT`. Find's local helper remains (idempotent double-apply is harmless).
- New helper `ThemeApplyToTooltip(HWND)` in `themes.cpp` (tooltips are WS_POPUP — the child
  enum never reaches them): dark: `SetWindowTheme(L"", L"")`, `TTM_SETTIPBKCOLOR =
  ThemeSysColor(COLOR_INFOBK)`, `TTM_SETTIPTEXTCOLOR = ThemeSysColor(COLOR_INFOTEXT)`; light:
  restore (`NULL` theme + `GetSysColor` values). Called at both native-tooltip creation sites
  (`src/mainwnd3.cpp` splitter tooltip, `src/viewer3.cpp` viewer tooltip).
**Rationale**: proven color pairs (044; custom `CToolTip` already uses INFOBK/INFOTEXT).
Central branch closes checksum ×2 and pictview ×1 progress bars with zero plugin changes.

## R8 — Listview checkbox glyphs (E5)

**Decision**: In the enum proc's `SysListView32` branch: when `LVS_EX_CHECKBOXES` is set and
dark is active, install `CreateCheckboxImagelist(IconSizes[ICONSIZE_16])` (`src/gui.cpp:3788`,
already theme-aware — precedents `dialogs5.cpp:1674`, `finddlg2.cpp:1068`) as `LVSIL_STATE`,
guarded by a window prop so repeat sweeps don't re-create; on a light re-apply with the prop
present, detach + destroy the custom list and re-toggle `LVS_EX_CHECKBOXES` to regenerate the
native one. The listview destroys the state image list at window destruction (no
`LVS_SHAREIMAGELISTS`), so no leak on the normal path.
**Rationale**: central = all 5 core sites + ftp + dbviewer fixed with no per-site edits and no
new plugin API.
**Alternatives**: per-site wiring (7 edits, misses future sites); exporting the generator to
plugins (unneeded ABI growth).

## R9 — Remaining dialog-surface point fixes (C3, C4, C5, C7, D2, D3, D4, D5, B3, B5)

| Defect | Decision |
|---|---|
| C3 disabled multiline edits | Extend `ThemeFlatDisabledEditSubclassProc` (`themes.cpp:375-429`): remove the `ES_MULTILINE` bail; multiline branch paints `COLOR_BTNFACE` fill + `DrawTextW(DT_WORDBREAK\|DT_NOPREFIX\|DT_EDITCONTROL)` in `EM_GETRECT` rect with `COLOR_GRAYTEXT`. Affected controls are short informational texts (no scrolling). |
| C4 Change Icon list | `dialogs3.cpp:~2392`: `(HBRUSH)(bkColor+1)` → `ThemeSysColorBrush(bkColor)`; text colors via `ThemeSysColor`. Mirrors every other converted owner-draw. |
| C5 `CExecuteWindow` | `pack3.cpp` `WM_ERASEBKGND`: fill with `ThemeSysColorBrush(COLOR_BTNFACE)` and return TRUE (exact `CWaitWindow` precedent `dialogs3.cpp:2760`). C6 class brushes stay (each window responsible; sole broken consumer was C5). |
| C7 | `dialogs4.cpp:1362-1363` `GetSysColor` → `ThemeSysColor` (matches all 9 sibling sites). |
| D2 header text | `gui.cpp:~2875` (`CToolbarHeader::OnPaint`): add `SetTextColor(ThemeSysColor(COLOR_BTNTEXT))` + `SetBkMode(TRANSPARENT)` before `DrawText` — the fix 044 recorded as "candidate for a future sweep". |
| D3 swatches | `dialogs4.cpp:3682-3694`: blank state uses `IsDarkThemeActive() ? ThemeSysColor(COLOR_BTNFACE) : RGB(255,255,255)` for both fg/bg (light byte-identical). |
| D4 Drive Info gauge | `gui.cpp:1574` outline pen: dark → `ThemeSysColor(COLOR_GRAYTEXT)`, light → black (unchanged). `dialogs3.cpp:1715-1725`: in dark force the truecolor constants for both color-depth branches (kills the 256-color pure-blue). Truecolor values stay (saturated, readable on dark). |
| D5 markers | `toolbar3.cpp:522`: pair `DrawFocusRect` with `ThemeSysColor(COLOR_BTNTEXT)/(COLOR_BTNFACE)` (light: BTNTEXT==black → identical). `toolbar2.cpp:825` insert-mark pen → `ThemeSysColor(COLOR_BTNTEXT)` (light black, identical). `filesmap.cpp:679-680` verified visible in both themes (XOR pattern) — no change, recorded. |
| B3 arrow button | `edtlbwnd.cpp:~555`: dark branch replaces `DrawFrameControl(DFC_SCROLL)` with face fill + `ThemeDrawEdge` raised border + `COLOR_BTNTEXT` arrow triangle; light unchanged. |
| B5 config tree | New hook `BOOL (*SheetsIsDarkHook)()` beside `SheetsGetSysColorHook` (`common/sheets.h|cpp`), installed to `IsDarkThemeActive` in `salamdr1.cpp:~1884`; `sheets.cpp:702` picks `L"DarkMode_Explorer"` vs `L"explorer"`. Removes the order dependency. |

## R10 — Plugin coverage (F1–F5)

- **F1 peviewer**: call `SetupWinLibTheme(SalamanderGeneral)` in `SalamanderPluginEntry`
  (`peviewer.cpp`; winliblt.h already included via precomp). One line — the 036 pattern.
- **F2 propsheet frames** (ftp, pictview, filecomp): append
  `virtual void WINAPI ThemeSubclassPropSheetFrame(HWND hwnd) = 0;` to
  `CSalamanderGeneralAbstract` after `ThemeHandleCtlColor` (`spl_gen.h`), implement as one-line
  delegation in `CSalamanderGeneral` (`src/zip.cpp`, beside the 036 six), bump
  `LAST_VERSION_OF_SALAMANDER` 105 → **106** with a `spl_vers.h` history row. Call it centrally
  from winliblt's `CPropSheetPageProc` on `WM_INITDIALOG`
  (`ThemeApplyToDialog` + `ThemeSubclassPropSheetFrame(GetParent(hwnd))` — the core
  `dialogs2.cpp:344` pattern). The core-side subclass is idempotent (fixed subclass ID). All
  three plugins are covered without per-plugin edits; plugins built for ≤ 105 keep loading.
- **F3 mdview Find**: add the two-touchpoint raw-proc pattern (SFTP/ZIP precedent) to
  `FindDlgProc` (`mdview/viewer.cpp:338`): `WM_INITDIALOG` → `ThemeApplyToDialog`;
  `WM_CTLCOLOR*` → `ThemeHandleCtlColor`.
- **F4 diskmap About**: same two-touchpoint pattern in the About dlgproc
  (`GUI.AboutDialog.h:34`); diskmap already holds the general interface
  (uses `ThemeApplyToTopLevel`).
- **F5 message boxes (safe sites per clarification)**: convert `fileswn2.cpp:2768`,
  `packac.cpp:276`, `dialogs2.cpp:1105` to `SalMessageBox`. Keep `regwork.cpp` (registry
  failure paths) and `salamdr1.cpp` (startup/allocation) on system `MessageBox`. The
  winlib/winliblt numeric-validation boxes get a function-pointer hook
  (`WinLibMessageBoxHook`, default `::MessageBox`) installed by the core to the themed
  `SalMessageBox` and by winliblt to `SalamanderGeneral->SalMessageBox` — same pattern as
  `SheetsGetSysColorHook`; if implementation reveals signature friction, the winliblt side may
  be recorded as an accepted residual (core side is required).

## R11 — Lifecycle robustness (G1–G5)

- **G1**: `CMainWindow` `WM_SETTINGCHANGE` (`mainwnd3.cpp:1271`): when
  `wParam == SPI_SETHIGHCONTRAST`, run the `WM_SYSCOLORCHANGE` sequence
  (`RefreshThemeHighContrastState` → `UpdateCurrentColorsForTheme` → class bg → top-level →
  `ColorsChanged(TRUE,FALSE,TRUE)`). Idempotent if the OS also sends `WM_SYSCOLORCHANGE`.
- **G2 + G5**: add `WM_THEMECHANGED` → `ThemeApplyToDialog(HWindow)` in
  `CCommonDialog::DialogProc`, `CCommonPropSheetPage::DialogProc`, and `CMainWindow`
  (re-apply after comctl32 resets `SetWindowTheme` state); add the same re-apply to
  `CCommonDialog`'s `WM_SYSCOLORCHANGE` path. The viewer already re-applies on
  `WM_USER_CFGCHANGED`; add `WM_THEMECHANGED` → same block in `viewer3.cpp`.
- **G3**: `UpdateCurrentColorsForTheme` pre-creates **all** mapped dark brushes (not just
  BTNFACE/WINDOW) so `ThemeSysColorBrush` never lazily creates from the viewer thread; light
  passthrough uses system-owned `GetSysColorBrush` (thread-safe). The `SetClassLongPtr` cross-
  thread call is process-global by design and stays (documented).
- **G4**: `salamdr1.cpp:3143` → `UpdateViewerColors(CurrentViewerColors)` per
  `028/contracts/theme-engine.md:68`.

## R12 — Subclass ID registry (extends 044)

| ID | Owner |
|---|---|
| 1 | propsheet frame (028) |
| 2 | disabled static (036) |
| 3 | etched static (044) |
| 4 | disabled edit (044; extended to multiline by 049) |
| 5 | status bar (044) |
| 6 | **group box (049)** |
| 7 | **radio glyph (049)** |
| 8 | **grayscale remap — DTP/hotkey (049)** |

All new subclasses: paint-class messages only (`WM_PAINT`, `WM_ERASEBKGND`), everything else
`DefSubclassProc`; removed/no-op in light; installed only from the central enum proc.

## R13 — Test strategy

- saltests: update nothing for existing floors (all hold with WINDOW=56, verified in R2);
  **add**: `Luminance(WINDOW) > Luminance(BTNFACE)` (field-lighter-than-face invariant),
  `ContrastRatio(HOTLIGHT, TC_NAVY 0x0A1424) ≥ 4.5` (About link), and keep count monotonicity.
- Build: `build.cmd full` (Debug x64) via PowerShell (044 gotcha: never `cmd /c` from Git
  Bash). saltests run from build output.
- Manual GUI walkthrough per `quickstart.md` (includes the two checks 044 left open).

## R14 — Audit claims corrected during planning (do not implement)

- **B4 rebar background**: `RB_SETBKCOLOR` **does** exist — `salamdr1.cpp:3164`
  (`ColorsChanged` path). raw-06's "no RB_SETBKCOLOR anywhere" is wrong; B4 is dropped.
- **D5 `filesmap.cpp` rubber band**: white/black XOR pair is visible on both themes; changing
  it would alter light mode. Dropped (recorded in R9).
- Progress-bar dark colors are `BTNSHADOW`/`HIGHLIGHT` (not BTNFACE as the consolidated audit
  paraphrased) — mirror `finddlg1.cpp:1522-1543` exactly.
