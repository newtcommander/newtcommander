# Visual Appearance Architecture Survey — Open Salamander

**Feature**: 028-visual-themes (switchable Default + Dark themes)
**Created**: 2026-07-21
**Purpose**: Detailed project analysis grounding the feature specification;
input for the planning phase (`/speckit.plan`). Findings verified against the
source tree on branch `028-visual-themes` (based on `main` @ 55bd406).

---

## 1. Existing color system

There is a mature, centralized **panel color-scheme system**, but it covers
only a subset of surfaces.

**Core model** (`src/consts.h:1209-1286`):

- `typedef DWORD SALCOLOR` — a `COLORREF` (low 24 bits) plus a flag byte in
  the high byte. Macros `RGBF(r,g,b,f)`, `GetCOLORREF()`, `GetFValue()`,
  `SetRGBPart()`.
- Flag `SCF_DEFAULT 0x01` (`consts.h:1265`) means "ignore the stored RGB;
  resolve from a system color at runtime."
- `NUMBER_OF_COLORS 34` named indices (`consts.h:1210-1250`): focus frame
  pens, item fg/bk (normal/selected/focused/focsel/highlight), icon-blend,
  progress bar, hot item, active/inactive panel caption fg/bk, thumbnail
  frames. Plus `NUMBER_OF_VIEWERCOLORS 4` (viewer fg/bk normal/selected) and
  `NUMBER_OF_CUSTOMCOLORS 16` (color-picker custom slots).

**Scheme arrays** (`src/salamdr1.cpp:437-659`): four built-in schemes as
static `COLORREF[NUMBER_OF_COLORS]` arrays — `SalamanderColors`,
`ExplorerColors`, `NortonColors` (Norton Commander), `NavigatorColors`
(DOS Navigator). `UserColors[]` is the editable "Custom" scheme.
`CurrentColors` is a pointer aliasing one of these (`salamdr1.cpp:437`,
default = `SalamanderColors`). `ViewerColors[]` at `salamdr1.cpp:441`.

**Resolution of SCF_DEFAULT → system colors**: `UpdateDefaultColors()`
(`src/salamdr1.cpp:1581-1713`) substitutes `GetSysColor(...)` for every
entry flagged `SCF_DEFAULT` — `COLOR_WINDOW`/`COLOR_WINDOWTEXT` for item
bg/fg, `COLOR_HIGHLIGHT(TEXT)` for progress, caption colors for panel
captions, `COLOR_HOTLIGHT` for hot items. It also derives highlight/full-row
tints heuristically (`GetFullRowHighlight`, `GetHilightColor`, grayscale
math at `salamdr1.cpp:1514-1579`).

> **Key implication for dark mode**: classic `GetSysColor` does NOT track
> the Windows 10/11 dark-apps setting, so "default" colors stay light even
> when Windows is dark. A dark theme must supply an explicit dark `SALCOLOR`
> array (or a different default-resolution source).

**User customization**: Configuration dialog "Colors" page =
`CCfgPageColors` (`src/cfgdlg.h:739`, impl `src/dialogs4.cpp:3290+`).
Scheme dropdown: Salamander/Explorer/Norton/Navigator/Custom
(`dialogs4.cpp:3327`, `IDS_COLORSCHEME_*`). Selecting a scheme repoints
`CurrentColors`; "Custom" edits `UserColors[]`. Also edits `ViewerColors`
and highlight masks (`CHighlightMasks`, default-resolved in
`UpdateDefaultColors`, `salamdr1.cpp:1684-1712`).

**Coverage**: panels, panel captions, progress, thumbnails, icon-blend,
internal viewer, file-highlight masks. **NOT covered**: menus, toolbars,
status/directory line bodies, command line, dialogs, window frames,
scrollbars — these use `GetSysColor(COLOR_BTNFACE/BTNTEXT/…)` or XP visual
styles directly.

## 2. Drawn surfaces

| Surface | File(s) | Color source |
|---|---|---|
| File panels (owner-drawn, off-screen `ItemBitmap`) | `src/filesbx1.cpp`, `filesbx2.cpp`, `fileswn4.cpp`, `fileswn9.cpp` | `CurrentColors[]` + highlight masks (fully themeable already) |
| Directory line / info line / panel caption (`CStatusWindow`) | `src/stswnd.cpp:783-855, 1113-1278` | Mix: `CurrentColors[…CAPTION…, HOT_*]` for caption; **`GetSysColor(COLOR_BTNFACE/BTNTEXT)`** for status body |
| Main menu (owner-drawn `CMenuBar`/`CMenuPopup`) | `src/menubar.cpp`, `src/menu3.cpp:89-94, 431-734` | **`GetSysColor(COLOR_BTNFACE, COLOR_HIGHLIGHT, COLOR_BTNTEXT, COLOR_HIGHLIGHTTEXT, COLOR_3DHILIGHT, COLOR_3DSHADOW)`** — not configurable |
| Toolbars (custom `CToolBar`, off-screen) | `src/toolbar2.cpp:556-662`, `toolbar3.cpp`, rebar `mainwnd3.cpp:1063` | `GetSysColor(COLOR_BTNFACE/3DHILIGHT/BTNSHADOW/BTNTEXT)`; rebar bg `COLOR_BTNFACE` (`salamdr1.cpp:2997`) |
| Bottom function-key (F1–F12) bar | `src/filesbx2.cpp:228, 267`; image list `HBottomTBImageList` (`salamdr1.cpp:2318`) | `CurrentColors[HOT_PANEL]` for hot, else `GetSysColor(COLOR_BTNTEXT)` |
| Command-line edit (`CEditWindow`) | `src/editwnd.cpp` (uxtheme; `editwnd.cpp:1571`) | Standard EDIT subclass → system window colors |
| Internal text/hex viewer | `src/viewer.cpp:812-1534` | `ViewerColors[]` (themeable); `GetSysColor(COLOR_WINDOW/WINDOWTEXT)` fallback (`viewer.cpp:1528-1534`) |
| Dialogs (standard Win32 templates) + custom controls | `src/gui.cpp:2087-2167, 3796-3820`; `src/common/sheets.cpp` | System colors + `OpenThemeData`/`DrawThemeBackground` |
| Find dialog | `src/finddlg1.cpp:4017-4460`, `finddlg2.cpp` | `GetSysColor(COLOR_3DFACE/WINDOW)`; results list owner-drawn |
| Tooltips | `src/tooltip.cpp:646` | `GetSysColor(COLOR_INFOTEXT)` |
| Message boxes | `src/msgbox.cpp:1151-1153` | `GetSysColor(COLOR_WINDOW/WINDOWTEXT)` |

**Net**: panels + viewer are the only fully theme-aware surfaces today;
every chrome surface (menu, toolbar, status body, dialogs, command line,
tooltips) is hard-wired to `GetSysColor`/uxtheme.

## 3. Graphics / icons

- **Graphics init/teardown**: `InitializeGraphics(BOOL colorsOnly)` /
  `ReleaseGraphics` (`src/salamdr1.cpp:2134+`), re-run by `ColorsChanged()`
  on any color change (`salamdr1.cpp:2968-3005`).
- **Toolbar bitmaps**: depth variants via `Use256ColorsBitmap()`
  (`salamdr1.cpp:1721`): `IDB_TOOLBAR_256` vs `IDB_TOOLBAR_16`
  (`salamdr1.cpp:2298-2299`). `CreateToolbarBitmaps()` builds mask +
  grayscale (disabled) + color image lists, blending transparent magenta
  `RGB(255,0,255)` against `GetSysColor(COLOR_BTNFACE)` — **toolbar
  backgrounds are baked against the system button face at load time**.
  Image lists: `HHotToolBarImageList`, `HGrayToolBarImageList`,
  `HBottomTBImageList`, `HMenuMarkImageList`, `HFindSymbolsImageList`.
- **SVG icon system (modern, recolorable)**: `src/svg.h` / `src/svg.cpp` —
  `CSVGSprite` with states `SVGSTATE_ORIGINAL/ENABLED/DISABLED` and
  `ColorizeSVG()` that tints an SVG to a target color (`svg.h:19-70`).
  `RenderSVGImage()` takes a `bkColor`; `GetSVGSysColor(index)` returns a
  system color for the rasterizer (`svg.h:9-12`). Main-toolbar SVGs overlay
  the bitmap toolbar via `GetSVGIconsMainToolbar` (`salamdr1.cpp:2297`).
  **This is the existing image-recoloring machinery a dark theme can
  leverage.**
- **File icons**: Windows system image list / shell (`geticon.cpp`,
  `icncache.cpp`, `shiconov.cpp`); icon cache keyed on
  `CurrentColors[ITEM_BK_NORMAL]` in places. `GetImageListColorFlags()`
  returns `ILC_COLOR32` (`salamdr1.cpp:1727`).
- `iconColorsCount` / `LoadColorTable(IDC_COLORTABLE,…)`
  (`salamdr1.cpp:1736-1776`) — palette for image recoloring.

## 4. Dark-mode support today

**None in the main app.** Zero matches anywhere for
`DwmSetWindowAttribute`, `DWMWA_USE_IMMERSIVE_DARK_MODE`,
`AllowDarkModeForWindow`, `SetPreferredAppMode`, `ShouldAppsUseDarkMode`.

What exists is XP visual-styles (uxtheme) for light theming only:
`IsAppThemed`, `OpenThemeData`, `DrawThemeBackground` in
`src/gui.cpp:2087-3820`, `src/common/sheets.cpp:690-1358` (tree view
`SetWindowTheme(…, L"explorer")`), `src/editwnd.cpp:1910`,
`src/logo.cpp:454`; theming *stripped* from the rebar via
`SetWindowTheme(…, L" ", L" ")` (`mainwnd3.cpp:1063`, `plugins3.cpp:465`).
`#include <uxtheme.h>` + `#pragma comment(lib,"UxTheme.lib")`
(`salamdr1.cpp:28,51`).

**Existing in-repo reference**: the **mdview plugin** already implements
light/dark schemes — `src/plugins/mdview/viewer.cpp:355-375` reads
`HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme`
and picks `g_schemeLight`/`g_schemeDark`, with a runtime theme menu
(`viewer.cpp:395-419`). Useful pattern for a future follow-OS option
(out of scope for 028).

## 5. Menu structure (where the switch goes)

- Main menu is built in code from a static template, not an `.rc MENU`
  resource: `MainMenuTemplate[]` in `src/menu4.cpp:17-256`, instantiated by
  `BuildSalamanderMenus()` (`menu4.cpp:357-363`). Items are
  `{type, IDS_string, skill-flags, CM_command, toolbar-icon, …}`.
- **Options popup**: `menu4.cpp:180-256` — existing entries
  `IDS_MENU_OPT_CONFIGURATION → CM_CONFIGURATION` (handler
  `mainwnd3.cpp:2919`), `CML_OPTIONS_PLUGINS` submenu, `CM_SKILLLEVEL`,
  and the "Visible" submenu (`CML_OPTIONS_VISIBLE`). A theme selector fits
  here as a new `CM_*` item/submenu.
- **Config dialog page registration**: `CConfigurationDlg :
  CTreePropDialog` (`src/cfgdlg.h:1158-1201`) declares ~30 page members
  (`PageColors`, `PageView`, `PageAppear`, …). A "Themes" page would follow
  the `CCfgPageColors` pattern (`cfgdlg.h:739`); `CCfgPageAppearance`
  (`cfgdlg.h:1053`) is the natural sibling.

## 6. Configuration persistence

- Registry root: `HKEY_CURRENT_USER\Software\Open Salamander\5.0`
  (`SalamanderConfigurationRoots[]`, `src/mainwnd2.cpp:159`; older
  Altap/Servant roots imported).
- Colors persisted under subkey `SALAMANDER_COLORS_REG`
  (`mainwnd2.cpp:2267`): scheme DWORD `SALAMANDER_CLRSCHEME_REG`
  (0=Salamander/1=Explorer/2=Norton/3=Navigator/4=Custom,
  `mainwnd2.cpp:2269-2278`), all `UserColors[]`/`ViewerColors[]` via
  `SaveRGBF` (`mainwnd2.cpp:2280-2325`), highlight masks under
  `SALAMANDER_HLT`, 16 `CustomColors`. Load mirror at
  `mainwnd2.cpp:2572-2636`.
- Helpers: `SaveRGBF`/`LoadRGBF` (`consts.h:1788-1789`), `regwork.cpp`.
  The theme choice is one more DWORD (colors subkey or general config key
  `SALAMANDER_CONFIG_REG`, `consts.h:1521`).
- **Change propagation**: `ColorsChanged(refresh, colorsOnly,
  reloadUMIcons)` (`salamdr1.cpp:2968`) → `ReleaseGraphics` +
  `InitializeGraphics`, `UpdateViewerColors`, `MainWindow->OnColorsChanged`,
  broadcasts `WM_USER_COLORCHANGEFIND`, fires `PLUGINEVENT_COLORSCHANGED`
  to plugins. **This is the existing hook a live theme switch should call.**

## 7. Scope size

- **Dialogs**: `src/lang/lang.rc` contains **165 `DIALOG`/`DIALOGEX`
  templates** (main app), all standard Win32 → each needs `WM_CTLCOLOR*`
  handling or a shared subclass/hook layer to go dark (no global theming
  layer exists today). Plus ~30 config property-sheet pages (`cfgdlg.h`).
- **Owner-drawn custom controls** (each a distinct code path): panels
  (`filesbx*`), status/dir line (`stswnd`), menu bar+popups
  (`menubar`/`menu3`), toolbars (`toolbar2/3`), bottom F-key bar
  (`filesbx2`), tab bar (`tabwnd.cpp`), edit-listbox (`edtlbwnd.cpp`),
  command line (`editwnd`), tooltips (`tooltip`), message box (`msgbox`),
  viewer (`viewer`), find list (`finddlg1/2`), GUI button/checkbox/color
  controls (`gui.cpp`), property sheets (`common/sheets.cpp`). Roughly
  **12–15 owner-drawn surfaces** + OS chrome (title bar/scrollbars — needs
  the immersive-dark-mode APIs, currently absent).
- **Plugin API color exposure** (plugins already receive host colors):
  `CSalamanderGeneralAbstract::GetCurrentColor(int)` declared
  `src/plugins/shared/spl_gen.h:1550`, implemented `src/zip.cpp:1648`; maps
  **38 `SALCOL_*` constants** (`spl_gen.h:314-351`) onto
  `CurrentColors`/`ViewerColors`. Used in ~15 plugin files (filecomp,
  regedt, pictview, folders, …). Plugins also query
  `CanUse256ColorsBitmap()` (`spl_gen.h:2794`). **A new theme flows to
  plugins automatically via `CurrentColors` + `PLUGINEVENT_COLORSCHANGED`**
  — provided the theme populates `CurrentColors`/`ViewerColors` rather than
  bypassing them. No dark-specific plugin API exists.

## Bottom line (implementation outlook for planning)

The cleanest lever is the existing `SALCOLOR`/`CurrentColors` scheme:
adding a **dark scheme array** (parallel to `SalamanderColors`,
`salamdr1.cpp:449`) plus a dark `ViewerColors` variant covers panels,
captions, thumbnails, viewer, and all color-consuming plugins for free
through `ColorsChanged`/`GetCurrentColor`.

The **hard, large part** is the chrome that ignores `CurrentColors`:

1. Owner-drawn menu/toolbar/status/bottom-bar — all read
   `GetSysColor(COLOR_BTNFACE/BTNTEXT/…)` directly. A theme-aware color
   accessor (e.g. `GetThemeSysColor()` shim replacing raw `GetSysColor` in
   app-drawn code) is the likely approach.
2. The **165 standard dialogs** — need a centralized `WM_CTLCOLOR*` /
   subclass layer plus dark handling for common controls (lists, trees,
   tabs, combos, scrollbars via `SetWindowTheme(…, L"DarkMode_Explorer")`).
3. **Title bar** — `DwmSetWindowAttribute(DWMWA_USE_IMMERSIVE_DARK_MODE)`
   per top-level window (currently absent from the codebase).
4. **Toolbar imagery** — bitmaps are baked against `COLOR_BTNFACE` at load;
   must be re-baked against the theme background on switch
   (`CreateToolbarBitmaps` already re-runs via
   `ReleaseGraphics`/`InitializeGraphics`); SVG sprites recolor via
   `CSVGSprite::ColorizeSVG`.

Existing assets to reuse: the SVG recoloring machinery (`svg.h`), the
`ColorsChanged` propagation pipeline, the scheme-array pattern, and the
mdview plugin's OS-dark detection (future follow-OS enhancement).
