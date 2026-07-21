# Phase 0 Research: Switchable Visual Themes (Default + Dark)

**Feature**: 028-visual-themes | **Date**: 2026-07-21
**Sources**: three targeted code surveys (dialog/window infrastructure,
system-color usage sweep, color-scheme plumbing trace) + the spec-phase
survey in `analysis/visual-architecture-survey.md`. All file:line refs
verified against branch `028-visual-themes` (base `main` @ 55bd406).

**Guiding constraint (user directive)**: change ONLY the visual layer.
No changes to file operations, panel logic, plugin binary interface, or
the configuration engine beyond one new setting. Every change is either
(a) draw-time color resolution, (b) window styling at creation, (c) one
config DWORD + one menu entry.

---

## D1. Theme state: one new `Configuration.ThemeMode` field

**Decision**: Add `DWORD ThemeMode` (0 = Default, 1 = Dark) to
`CConfiguration` (`src/cfgdlg.h:176+`), persisted as a new
`CONFIG_THEMEMODE_REG` DWORD under the general config key
(`SALAMANDER_CONFIG_REG`), following the `UseRecycleBin` pattern
(save `src/mainwnd2.cpp:1735`, load `src/mainwnd2.cpp:3248`). Missing
value ⇒ 0 (Default) — satisfies FR-004 with no migration.

**Rationale**: The panel scheme is stored as *pointer identity* of
`CurrentColors` (no index field exists; trace §1), so the theme axis
cannot piggyback on it — clarification session decided theme is a
separate setting anyway.

**Alternatives considered**: (a) 5th color scheme in `SALAMANDER_CLRSCHEME_REG`
— rejected by clarification (conflates two axes); (b) separate registry
subkey — overkill for one DWORD.

## D2. New theme engine module `src/themes.h` + `src/themes.cpp`

**Decision**: One new module owning all theme knowledge:

- `BOOL IsDarkThemeActive()` — TRUE iff `Configuration.ThemeMode == 1`
  AND Windows High Contrast is not active (FR-013).
- `COLORREF ThemeSysColor(int index)` — Default/high-contrast: exact
  passthrough to `GetSysColor(index)` (guarantees SC-003); Dark: lookup
  in a static dark chrome palette (D3).
- `HBRUSH ThemeSysColorBrush(int index)` — Default: `GetSysColorBrush`;
  Dark: cached app-owned solid brushes (rebuilt on switch, freed at exit).
- `void ThemeApplyToTopLevel(HWND)` — DWM dark title bar (D5).
- `void ThemeApplyToDialog(HWND dlg)` — child enumeration: per-class
  `SetWindowTheme` dark variants + listview/treeview color setup (D6).
- `BOOL ThemeHandleCtlColor(UINT msg, WPARAM, LPARAM, INT_PTR* result)`
  — shared WM_CTLCOLOR* implementation for the two central dialog procs.
- `void SetThemeMode(DWORD mode)` — the switch entry point (D9).

**Rationale**: mirrors the only existing wrapper precedent
(`GetSVGSysColor`, `src/svg.cpp:35`); keeps the Default path a pure
passthrough so zero behavior changes when Dark is off.

**Alternatives**: scattering `if (dark)` at call sites — unmaintainable;
hooking `GetSysColor` via IAT patching — rejected (invasive, affects
plugins and OS-owned drawing unpredictably).

## D3. Dark chrome palette (COLOR_* → dark RGB)

**Decision**: static table covering exactly the COLOR_* indices the app
draws with (sweep §1): WINDOW, WINDOWTEXT, WINDOWFRAME, BTNFACE/3DFACE,
BTNTEXT, BTNSHADOW/3DSHADOW, BTNHILIGHT/3DHILIGHT, 3DLIGHT, HIGHLIGHT,
HIGHLIGHTTEXT, GRAYTEXT, HOTLIGHT, INFOTEXT, INFOBK, ACTIVECAPTION,
CAPTIONTEXT, INACTIVECAPTION, INACTIVECAPTIONTEXT, MENU-related. Values
modeled on Windows 11 dark app conventions (surfaces #202020/#2D2D2D,
text #F0F0F0, selection #264F78); exact table in `data-model.md`, with
contrast ratios ≥ 4.5:1 for text pairs (SC-005) verified by unit test.
Unlisted indices fall through to `GetSysColor` (never darker-broken than
today).

## D4. Dark panel scheme + dark viewer colors

**Decision**: `DarkColors[NUMBER_OF_COLORS]` — a 6th static SALCOLOR
array (structure parallel to `SalamanderColors`, `src/salamdr1.cpp:449`),
all entries explicit RGB, **no `SCF_DEFAULT` flags** (so
`UpdateDefaultColors` never overwrites them with light system colors —
trace §4 gotcha). Plus `DarkViewerColors[NUMBER_OF_VIEWERCOLORS]`.

**Scheme-pointer decoupling** (per clarification "separate setting"):
introduce `COLORREF* SchemeColors` holding what the Colors page/config
selected; the two existing repoint sites (`src/mainwnd2.cpp:2572-2586`,
`src/dialogs4.cpp:3363-3378`) assign `SchemeColors`, the three
pointer-identity ladders (`mainwnd2.cpp:2269`, `dialogs4.cpp:3338`,
`dialogs4.cpp:3395`) compare `SchemeColors`, and a new
`UpdateCurrentColorsForTheme()` sets
`CurrentColors = IsDarkThemeActive() ? DarkColors : SchemeColors`.
Colors-page edits while Dark is active therefore keep affecting the
Default theme only, and the user's scheme/custom colors are never
touched (FR-011). Panels, captions, thumbnails, highlight masks, and
all plugins (via `GetCurrentColor`) follow automatically.

**Viewer**: add `SALCOLOR* CurrentViewerColors = ViewerColors;` and
switch the ~12 in-file draw/brush references in `src/viewer.cpp`
(sweep §6) plus the plugin viewer-color mapping (`src/zip.cpp:1648`)
to it. User's `ViewerColors` config values are never overwritten
(they are saved back to registry — mutating them would corrupt the
user's configuration; FR-008/FR-011).

## D5. Dark title bar via documented DWM attribute

**Decision**: `DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE
(=20), &BOOL, sizeof)` — documented API, Windows 11 target guaranteed
(constitution: Win11+). Applied at creation via the two funnels
(`CDialog::CDialogProc` WM_INITDIALOG `src/common/winlib.cpp:726`,
`CPropSheetPage::CPropSheetPageProc` `src/common/sheets.cpp:388`) and
for the two frame windows (`CMainWindow` create `src/salamdr1.cpp:4224`,
`CViewerWindow` `src/viewer.cpp:1570`); re-applied (TRUE/FALSE) on
theme switch to the main window and to open viewer/find windows through
the existing broadcast handlers. Link `dwmapi.lib`.

**Rejected**: undocumented uxtheme ordinals (`SetPreferredAppMode`,
`AllowDarkModeForWindow`) — not needed because Salamander's menus are
owner-drawn (`CMenuPopup`), and undocumented APIs violate the project's
stability posture. OS scrollbar/common-control darkening uses the
semi-documented but stable `SetWindowTheme(hwnd, L"DarkMode_Explorer",
NULL)` / `L"DarkMode_CFD"` (combos) with graceful fallback — if the
theme name is unknown the call fails harmlessly and colors still come
from WM_CTLCOLOR.

## D6. Dialog dark layer — two central procs, no per-dialog edits

**Decision** (from infrastructure survey): 100% of app dialogs flow
through `CDialog::CDialogProc` (`src/common/winlib.cpp:704`) and all
config pages through `CPropSheetPage::CPropSheetPageProc`
(`src/common/sheets.cpp:382`). Hook points:

1. `WM_CTLCOLORDLG/STATIC/EDIT/LISTBOX/BTN` handled centrally in
   `CDialog::DialogProc` (`winlib.cpp:635`) and
   `CPropSheetPage::DialogProc` via shared `ThemeHandleCtlColor` —
   returns dark brushes + light text; no-op (FALSE) in Default theme.
2. `ThemeApplyToDialog` called from the WM_INITDIALOG paths
   (`NotifDlgJustCreated` hooks, `src/dialogs2.cpp:251` and `:333`):
   DWM title bar + `EnumChildWindows` applying `SetWindowTheme` dark
   variants and `ListView/TreeView_SetBk/TextColor` (centralizing what
   today exists as ~12 scattered per-dialog `ListView_SetBkColor` calls).
3. Property-sheet *frame* (config dialog chrome around pages): subclass
   installed from the first page's creation for WM_CTLCOLOR + DWM.
4. Reconcile the 3 existing per-dialog CTLCOLOR precedents:
   `src/msgbox.cpp:1120-1154`, `src/logo.cpp:433-454` (about box keeps
   its bitmap look — exempt), `src/gui.cpp:1144` (forwards to parent —
   already compatible).

**Risk accepted**: a small tail of exotic controls may stay imperfect;
SC-004 allows ≥95% dialog coverage, walkthrough will catch stragglers.

## D7. Chrome sweep — replace draw-time GetSysColor with ThemeSysColor

**Decision**: mechanical, file-by-file replacement of the **D-tagged**
draw sites from the sweep (menu3.cpp, menubar.cpp, toolbar2/3.cpp,
stswnd.cpp, filesbx2.cpp, gui.cpp, editwnd.cpp, tooltip.cpp, msgbox.cpp,
finddlg1/2.cpp, dialogs2-6.cpp, sheets.cpp, edtlbwnd.cpp, packac.cpp,
pack3.cpp), plus:

- Six `GetSysColorBrush` globals (`src/salamdr1.cpp:1808-1813`,
  `HDialogBrush`…`HMenuGrayTextBrush`) become app-owned
  `CreateSolidBrush(ThemeSysColor(...))`, rebuilt on every
  `InitializeGraphics`/theme switch, freed in release path (they are
  currently system brushes that must NOT be deleted — after conversion
  they MUST be).
- Six system-color pens (`src/salamdr1.cpp:2460-2465`) — already
  rebuilt in `InitializeGraphics`; just swap the accessor.
- `FillRect(hdc, r, (HBRUSH)(COLOR_X+1))` sites (sweep §4) →
  `ThemeSysColorBrush(COLOR_X)`.
- `DrawEdge` bevels (menu3 ×3, toolbar2 ×2, gui 2861, filesbx1 1359,
  stswnd 896) → `ThemeDrawEdge` helper (manual two-pen bevel in Dark,
  passthrough `DrawEdge` in Default).
- WNDCLASS background brushes (`COLOR_WINDOW+1` / `COLOR_3DFACE+1`
  classes: winlib.cpp:487/510, salamdr1.cpp:4151/4160/4215,
  viewer.cpp:1576) — `SetClassLongPtr(GCLP_HBRBACKGROUND)` swapped on
  theme switch/startup (theme is global, class-wide swap is correct).
- Rebar bg (`salamdr1.cpp:2997`), `ImageList_SetBkColor` sites, toolbar
  bitmap baking (`gui.cpp:2731`, `salamdr1.cpp:2285+`) → accessor; all
  re-run automatically via `ColorsChanged` → `Release/InitializeGraphics`.
- SVG icons: route `GetSVGSysColor` (`src/svg.cpp:35`) through
  `ThemeSysColor` — toolbar/menu glyph sprites re-rasterize against the
  dark background (FR-009).

**Non-drawing uses stay untouched**: `UpdateDefaultColors` seeding
(`P`), luminance checks (`X`), the find high-contrast test — they must
keep reflecting real system colors.

## D8. High contrast precedence

**Decision**: `IsDarkThemeActive()` returns FALSE whenever
`SystemParametersInfo(SPI_GETHIGHCONTRAST)` reports HC on (cached,
refreshed on `WM_SETTINGCHANGE`/`WM_SYSCOLORCHANGE` — the latter
already triggers full `ColorsChanged`, `src/mainwnd3.cpp:1237`).
The menu keeps showing the stored selection; effective rendering is
system HC (FR-013).

## D9. Live switch flow (`SetThemeMode`)

**Decision**: reuse the existing, proven `WM_SYSCOLORCHANGE` rebuild
path (`ColorsChanged(TRUE, FALSE, TRUE)`, `src/mainwnd3.cpp:1246`):

1. `Configuration.ThemeMode = mode`; refresh HC cache.
2. `UpdateCurrentColorsForTheme()` (repoint `CurrentColors` /
   `CurrentViewerColors`).
3. Swap WNDCLASS background brushes; rebuild theme brush cache.
4. `ColorsChanged(TRUE, FALSE, TRUE)` — rebuilds pens/brushes/toolbar
   bitmaps/imagelists, notifies panels, Find (live), viewers (live),
   plugins (`PLUGINEVENT_COLORSCHANGED`), invalidates main window.
5. Re-apply DWM title-bar attribute to main window + open viewer/find
   windows (inside their existing broadcast handlers).

Menu radio state: `CMenuPopup::CheckRadioItem` keyed on
`Configuration.ThemeMode` in the Options-popup init handler
(`src/mainwnd3.cpp:~4772` pattern). New `CM_THEME_DEFAULT` /
`CM_THEME_DARK` IDs in `src/resource.rh2`; WM_COMMAND cases next to
`CM_SKILLLEVEL` (`src/mainwnd3.cpp:2911`); menu rows in the Options
popup template (`src/menu4.cpp:180-256`) as an `MNTT_PB` submenu
"Theme"; strings in `src/texts.rh2` + `src/lang/texts.rc2`.

**Ongoing-work safety (FR-014)**: `ColorsChanged` already runs during
normal operation (system color changes) without disturbing operations;
no operation state is touched.

## D10. Startup — no light flash

**Decision**: colors are loaded and `CurrentColors` repointed BEFORE the
main window is shown (`LoadConfig` at `src/salamdr1.cpp:4242` <
`ShowWindow` at `:4268`) — main window cannot flash. The only
pre-config surface is the splash (`SplashScreenOpen`,
`salamdr1.cpp:3966`); mitigation: early-read `CONFIG_THEMEMODE_REG`
next to the existing early `ShowSplashScreen` read
(`salamdr1.cpp:3958`) and set `Configuration.ThemeMode` before any
window exists. The splash's branded bitmap stays as-is (assumption:
imagery unchanged).

## D11. Testing

**Decision**:
- **Build gates**: Debug x64 + Release x64 via `build.cmd` /
  `build.cmd release` must compile clean.
- **Unit tests** (saltests, currently 427 green): new theme tests —
  (a) dark chrome palette covers every COLOR_* index the app draws
  with; (b) WCAG contrast ratio ≥ 4.5:1 for all dark text/background
  pairs (SC-005), ≥ 3:1 for disabled text; (c) `DarkColors` has no
  `SCF_DEFAULT` flags (would be clobbered); (d) Default passthrough:
  `ThemeSysColor(i) == GetSysColor(i)` for all indices when Default
  active; (e) `ThemeMode` default value = 0.
- **Smoke**: launch built Debug exe, verify clean start/exit.
- **Visual walkthrough** (SC-003/SC-004 checklist in quickstart.md):
  performed by the user in the GUI (established project practice);
  SC-006 (20 switches) included in that checklist.

## Resolved unknowns

All Technical Context items are resolved; no NEEDS CLARIFICATION
remains. Key risks + mitigations:

| Risk | Mitigation |
|---|---|
| `DarkMode_Explorer` SetWindowTheme is semi-documented | Fallback: colors still come from central WM_CTLCOLOR; call failure is harmless |
| Property-sheet frame (comctl-owned) resists theming | Subclass from first page; SC-004 tolerates imperfection tail |
| Deleting converted brushes that were once system brushes | Conversion is atomic in one commit; release path updated in the same change |
| Owner-drawn buttons (gui.cpp) draw uxtheme light parts in Dark | Dark branch draws flat themed chrome instead of `DrawThemeBackground` |
| Regression risk to Default look | `ThemeSysColor`/`ThemeSysColorBrush`/`ThemeDrawEdge` are exact passthroughs when Dark is off; SC-003 walkthrough verifies |
