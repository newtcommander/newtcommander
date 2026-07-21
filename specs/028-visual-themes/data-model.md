# Phase 1 Data Model: Switchable Visual Themes (Default + Dark)

**Feature**: 028-visual-themes | **Date**: 2026-07-21

## 1. Theme Selection (persisted setting)

| Aspect | Value |
|---|---|
| Field | `DWORD ThemeMode` in `CConfiguration` (`src/cfgdlg.h`) |
| Values | `THEME_MODE_DEFAULT = 0`, `THEME_MODE_DARK = 1` |
| Default | `0` (ctor init; absent registry value keeps 0 — FR-004) |
| Registry | `CONFIG_THEMEMODE_REG = "Theme Mode"` (DWORD) under `SALAMANDER_CONFIG_REG`, saved/loaded beside `UseRecycleBin` (`src/mainwnd2.cpp:1735` / `:3248` pattern) |
| Early read | Next to `CONFIG_SHOWSPLASHSCREEN_REG` quick read (`src/salamdr1.cpp:3958`) so the mode is known before any window exists |

Validation: any value ≠ 1 is treated as 0 (forward-compatible).

## 2. Effective theme state (runtime, not persisted)

```
IsDarkThemeActive() == (Configuration.ThemeMode == THEME_MODE_DARK)
                       && !SystemHighContrastActive   // FR-013
```

High-contrast flag cached; refreshed on `WM_SETTINGCHANGE` /
`WM_SYSCOLORCHANGE` (which already triggers full `ColorsChanged`).

State transitions (all via `SetThemeMode(mode)`):

```
Default ──menu CM_THEME_DARK──▶ Dark      (repoint palettes, rebuild, notify)
Dark ──menu CM_THEME_DEFAULT──▶ Default   (restore SchemeColors, rebuild, notify)
any ──HC on──▶ effective Default rendering (stored ThemeMode unchanged)
```

## 3. Panel palette pointers

| Variable | Role |
|---|---|
| `SALCOLOR* CurrentColors` (existing) | What all drawing/plugins read. Becomes derived: `Dark ? DarkColors : SchemeColors` |
| `COLORREF* SchemeColors` (NEW) | The user-selected scheme (Salamander/Explorer/Norton/Navigator/UserColors). All existing repoint sites (`mainwnd2.cpp:2572`, `dialogs4.cpp:3363`) and identity ladders (`mainwnd2.cpp:2269`, `dialogs4.cpp:3338`, `:3395`) move to this pointer — Colors page keeps editing the Default theme while Dark is active (FR-011) |
| `SALCOLOR DarkColors[34]` (NEW) | Built-in dark panel scheme, all-explicit RGB, **zero `SCF_DEFAULT` flags** (so `UpdateDefaultColors` never rewrites it) |
| `SALCOLOR* CurrentViewerColors` (NEW) | `Dark ? DarkViewerColors : ViewerColors`; viewer draw sites + plugin viewer-color mapping read it. User's `ViewerColors` never mutated (FR-008) |
| `SALCOLOR DarkViewerColors[4]` (NEW) | Dark viewer scheme |

## 4. `DarkColors[NUMBER_OF_COLORS]` — dark panel scheme

Semantics follow the "Salamander" scheme (selection = warm red accent),
transposed to dark surfaces. All flags = 0.

| Index | Value (R,G,B) | Note |
|---|---|---|
| FOCUS_ACTIVE_NORMAL | 240,240,240 | focus frame pen |
| FOCUS_ACTIVE_SELECTED | 255,160,160 | |
| FOCUS_FG_INACTIVE_NORMAL | 128,128,128 | |
| FOCUS_FG_INACTIVE_SELECTED | 200,120,120 | |
| FOCUS_BK_INACTIVE_NORMAL | 32,32,32 | |
| FOCUS_BK_INACTIVE_SELECTED | 32,32,32 | |
| ITEM_FG_NORMAL | 240,240,240 | 12.9:1 vs #202020 |
| ITEM_FG_SELECTED | 255,110,110 | ~7:1 |
| ITEM_FG_FOCUSED | 255,255,255 | |
| ITEM_FG_FOCSEL | 255,128,128 | |
| ITEM_FG_HIGHLIGHT | 240,240,240 | |
| ITEM_BK_NORMAL | 32,32,32 | #202020 |
| ITEM_BK_SELECTED | 32,32,32 | red-text selection keeps bg |
| ITEM_BK_FOCUSED | 58,58,58 | dark analog of #E8E8E8 |
| ITEM_BK_FOCSEL | 58,58,58 | |
| ITEM_BK_HIGHLIGHT | 48,48,48 | |
| ICON_BLEND_SELECTED | 255,128,128 | |
| ICON_BLEND_FOCUSED | 128,128,128 | |
| ICON_BLEND_FOCSEL | 255,96,96 | |
| PROGRESS_FG_NORMAL | 130,180,255 | |
| PROGRESS_FG_SELECTED | 255,255,255 | |
| PROGRESS_BK_NORMAL | 32,32,32 | |
| PROGRESS_BK_SELECTED | 38,79,120 | #264F78 |
| HOT_PANEL | 102,178,255 | |
| HOT_ACTIVE | 180,210,255 | on active caption bk |
| HOT_INACTIVE | 160,190,230 | |
| ACTIVE_CAPTION_FG | 255,255,255 | |
| ACTIVE_CAPTION_BK | 38,79,120 | |
| INACTIVE_CAPTION_FG | 170,170,170 | |
| INACTIVE_CAPTION_BK | 45,45,45 | |
| THUMBNAIL_FRAME_NORMAL | 96,96,96 | |
| THUMBNAIL_FRAME_FOCUSED | 240,240,240 | |
| THUMBNAIL_FRAME_SELECTED | 255,110,110 | |
| THUMBNAIL_FRAME_FOCSEL | 200,80,80 | |

`DarkViewerColors`: FG_NORMAL 220,220,220 · BK_NORMAL 30,30,30 ·
FG_SELECTED 255,255,255 · BK_SELECTED 38,79,120.

## 5. Dark chrome palette (`ThemeSysColor` mapping)

Covers every COLOR_* index the app draws with (sweep §1); anything
outside the table passes through to `GetSysColor`.

| COLOR_* | Dark value | Used by |
|---|---|---|
| WINDOW | 32,32,32 | edit/list fields, viewer fallback, class bg |
| WINDOWTEXT | 240,240,240 | text on WINDOW |
| WINDOWFRAME | 85,85,85 | WndFramePen |
| BTNFACE / 3DFACE | 45,45,45 | dialogs, menus, toolbars, status body |
| BTNTEXT | 240,240,240 | chrome text, glyph masks |
| BTNSHADOW / 3DSHADOW | 26,26,26 | bevels |
| BTNHIGHLIGHT / 3DHILIGHT | 70,70,70 | bevels |
| 3DLIGHT | 58,58,58 | bevels |
| 3DDKSHADOW | 16,16,16 | bevels |
| HIGHLIGHT | 38,79,120 | menu/toolbar/list selection |
| HIGHLIGHTTEXT | 255,255,255 | |
| GRAYTEXT | 150,150,150 | disabled text (≥3:1 vs 45,45,45) |
| HOTLIGHT | 102,178,255 | links/hot |
| INFOTEXT / INFOBK | 240,240,240 / 50,50,50 | tooltips |
| ACTIVECAPTION / CAPTIONTEXT | 38,79,120 / 255,255,255 | caption-derived chrome |
| INACTIVECAPTION / INACTIVECAPTIONTEXT | 45,45,45 / 170,170,170 | |
| MENU / MENUTEXT | 45,45,45 / 240,240,240 | |
| SCROLLBAR | 45,45,45 | |
| APPWORKSPACE | 38,38,38 | |

Contract: every FG/BK pair used for standard text meets WCAG ≥ 4.5:1;
GRAYTEXT ≥ 3:1 (unit-tested — SC-005).

## 6. Theme-owned GDI caches (rebuilt on switch)

| Cache | Today | Dark-aware form |
|---|---|---|
| `HDialogBrush, HButtonTextBrush, HMenuSelectedBkBrush, HMenuSelectedTextBrush, HMenuHilightBrush, HMenuGrayTextBrush` (`salamdr1.cpp:1808-1813`) | `GetSysColorBrush` (system-owned, never freed) | `CreateSolidBrush(ThemeSysColor(...))`, rebuilt on theme switch + WM_SYSCOLORCHANGE, freed on release |
| `BtnShadowPen … WndPen` (`salamdr1.cpp:2460-2465`) | `CreatePen(GetSysColor)` in `InitializeGraphics` | same lifecycle, `ThemeSysColor` |
| Theme dialog/field brushes (NEW, in themes.cpp) | — | dialog bg, field bg, created lazily, freed at exit |
| WNDCLASS bg brushes (`COLOR_WINDOW+1`/`COLOR_3DFACE+1` classes) | static class brush | `SetClassLongPtr(GCLP_HBRBACKGROUND)` swap on startup + switch |

## 7. Menu / command surface

| Item | Value |
|---|---|
| Commands | `CM_THEME_DEFAULT`, `CM_THEME_DARK` (new, `src/resource.rh2`) |
| Menu rows | Options popup (`src/menu4.cpp:180+`): `MNTT_PB` "Theme" submenu → 2 `MNTT_IT` items, skill level `MNTS_B\|MNTS_I\|MNTS_A` |
| Strings | `IDS_MENU_OPT_THEME`, `IDS_MENU_OPT_THEME_DEFAULT`, `IDS_MENU_OPT_THEME_DARK` (`src/texts.rh2` + `src/lang/texts.rc2`) |
| Check state | `CheckRadioItem` on popup init keyed on `Configuration.ThemeMode` (`src/mainwnd3.cpp` popup-init handler) |
| Dispatch | `case CM_THEME_*` beside `CM_SKILLLEVEL` (`src/mainwnd3.cpp:2911`) → `SetThemeMode(...)` |
