# Contract: Theme Engine (`src/themes.h` / `src/themes.cpp`)

**Feature**: 028-visual-themes. Internal API contract — the only new
surface other modules depend on. C linkage-style free functions, WinAPI
types, no exceptions.

## Invariants (the safety contract)

1. **Default = passthrough**: when `IsDarkThemeActive()` is FALSE,
   `ThemeSysColor(i) ≡ GetSysColor(i)`, `ThemeSysColorBrush(i) ≡
   GetSysColorBrush(i)`, `ThemeDrawEdge ≡ DrawEdge`, and every
   `ThemeApply*` function is a no-op. This mechanically guarantees
   SC-003 (Default theme pixel-identical) at all converted call sites.
2. **No functional side effects**: no function touches panels' data,
   file operations, or configuration other than `Configuration.ThemeMode`
   (written only by `SetThemeMode`).
3. **Thread**: all functions main-thread only (same rule as all GDI in
   the app). `ThemeSysColor` is reentrant const lookup.
4. **Never returns invalid GDI handles**: brush getters return valid
   HBRUSH for any index (fallback `GetSysColorBrush`).
5. **User config untouched**: `UserColors`, `ViewerColors`,
   `SALAMANDER_CLRSCHEME_REG` are never written by the engine (FR-011).

## API

```c
enum { THEME_MODE_DEFAULT = 0, THEME_MODE_DARK = 1 };

BOOL     IsDarkThemeActive();            // ThemeMode==DARK && !high-contrast
void     RefreshThemeHighContrastState();// call on WM_SETTINGCHANGE/WM_SYSCOLORCHANGE

COLORREF ThemeSysColor(int index);       // GetSysColor replacement (draw sites only)
HBRUSH   ThemeSysColorBrush(int index);  // GetSysColorBrush / (HBRUSH)(COLOR_X+1) replacement
BOOL     ThemeDrawEdge(HDC, RECT*, UINT edge, UINT flags); // DrawEdge replacement

void     UpdateCurrentColorsForTheme();  // CurrentColors/CurrentViewerColors := per theme
void     SetThemeMode(DWORD mode);       // full live switch (see sequence below)

void     ThemeApplyToTopLevel(HWND);     // DWM immersive dark title bar (per current theme)
void     ThemeApplyToDialog(HWND dlg);   // title bar + EnumChildWindows control theming
BOOL     ThemeHandleCtlColor(UINT msg, WPARAM wParam, LPARAM lParam,
                             INT_PTR* result); // WM_CTLCOLOR* for central dialog procs

void     ReleaseThemeGraphics();         // free engine-owned brushes (app exit / rebuild)
```

## `SetThemeMode` sequence (normative)

1. `Configuration.ThemeMode = mode`; `RefreshThemeHighContrastState()`.
2. `UpdateCurrentColorsForTheme()`.
3. Swap WNDCLASS background brushes; invalidate engine brush cache.
4. `ColorsChanged(TRUE, FALSE, TRUE)` — existing pipeline rebuilds
   pens/brushes/toolbar bitmaps/imagelists and notifies panels, Find,
   viewers, plugins (`PLUGINEVENT_COLORSCHANGED`).
5. `ThemeApplyToTopLevel(MainWindow)`; open viewer/find windows re-apply
   in their existing broadcast handlers.

## Hook obligations on existing code (complete list of touch classes)

| Site | Obligation |
|---|---|
| `CDialog::DialogProc` (winlib.cpp) + `CPropSheetPage::DialogProc` (sheets) | call `ThemeHandleCtlColor` first; if TRUE return its result |
| `CCommonDialog/CCommonPropSheetPage::NotifDlgJustCreated` | call `ThemeApplyToDialog(HWindow)` |
| `CMainWindow`/`CViewerWindow` creation; WM_SYSCOLORCHANGE handler | `ThemeApplyToTopLevel` / `RefreshThemeHighContrastState` |
| Draw-site conversions | replace `GetSysColor`→`ThemeSysColor`, `(HBRUSH)(COLOR_X+1)`/`GetSysColorBrush`→`ThemeSysColorBrush`, `DrawEdge`→`ThemeDrawEdge` — **drawing sites only**, never seeding/luminance logic |
| `GetSVGSysColor` (svg.cpp) | route through `ThemeSysColor` |
| Scheme plumbing (mainwnd2/dialogs4) | assign/compare `SchemeColors`; call `UpdateCurrentColorsForTheme()` after changes |
| viewer.cpp | read `CurrentViewerColors`; `UpdateViewerColors(CurrentViewerColors)` on the ColorsChanged path |
| zip.cpp `GetCurrentColor` | viewer indices map to `CurrentViewerColors` (panel indices already follow `CurrentColors`) |

## Plugin-facing contract (unchanged ABI)

`CSalamanderGeneralAbstract::GetCurrentColor(SALCOL_*)` keeps its
signature; under Dark it returns dark values — that IS the intended
behavior (FR-012). `PLUGINEVENT_COLORSCHANGED` fires on switch via the
existing `ColorsChanged` pipeline. No new plugin API.
