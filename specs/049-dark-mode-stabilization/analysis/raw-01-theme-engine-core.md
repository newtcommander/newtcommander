# Raw findings — Agent 1: Theme engine mechanism & architectural gaps

> Unedited output of the theme-engine core exploration agent (feature 049 initial audit).
> Note (consolidator): the closing suggestion to use `SetPreferredAppMode` via uxtheme ordinal
> contradicts the standing "documented APIs only" invariant (028 D5, 037 R4, 044 R7) and is
> recorded here verbatim but must NOT be adopted.

# (A) MECHANISM

## A1. State + selection

| What | Where |
|---|---|
| Mode storage | `Configuration.ThemeMode` (`int`), declared `src/cfgdlg.h:328`; values `THEME_MODE_DEFAULT=0` / `THEME_MODE_DARK=1` (`src/themes.h:18-19`) |
| Default init | `src/dialogs4.cpp:279` |
| Registry load | `src/salamdr1.cpp:4129-4133` — read early (before splash), then `UpdateCurrentColorsForTheme()` |
| Registry save | `src/mainwnd2.cpp:1575`, `src/mainwnd2.cpp:3091-3093` |
| Master predicate | `IsDarkThemeActive()` — `src/themes.cpp:49`. `ThemeMode==DARK && !HighContrast`. **Every** theme path keys off this one call. |
| High-contrast cache | `RefreshThemeHighContrastState()` — `src/themes.cpp:38` (also eagerly builds the LUT because the viewer thread reads it) |
| UI entry | `CM_THEME_DEFAULT` / `CM_THEME_DARK` → `SetThemeMode()` at `src/mainwnd3.cpp:2944` / `:2950`; menu radio state `src/mainwnd3.cpp:4972` |

## A2. Palettes (data)

`src/common/themes_palette.h` is pure X-macro data, three tables:
- `THEME_DARK_SYSCOLORS` (line 19) — 27 `COLOR_*` overrides → expanded into `ThemeDarkSysColors[31]` LUT by `InitThemeDarkSysColors()` (`src/themes.cpp:68`).
- `THEME_DARK_PANEL_COLORS` (line 55) → `DarkColors[]` in `salamdr1.cpp` (order static_asserted against `consts.h` indices).
- `THEME_DARK_VIEWER_COLORS` (line 93) → `DarkViewerColors[]`.
- `ThemeDarkAdaptColor()` (line 117) — shared glyph-recolor math, used by both the bitmap transform and `svg.cpp`.

`UpdateCurrentColorsForTheme()` (`src/themes.cpp:193`) just repoints the two globals: `CurrentColors = DarkColors|SchemeColors`, `CurrentViewerColors = DarkViewerColors|ViewerColors`. All panel/viewer painting reads `CurrentColors[...]` — so panels are themed by pointer swap, not by hooks.

## A3. How colors reach pixels — five distinct channels

**1. Draw-site substitution (the bulk).** `ThemeSysColor(int)` `src/themes.cpp:81`, `ThemeSysColorBrush(int)` `:99`, `ThemeDrawEdge(...)` `:130`. Contractual invariant: in Default theme these are byte-identical passthroughs to `GetSysColor`/`GetSysColorBrush`/`DrawEdge`. Brushes are engine-owned, lazily created, cached in `ThemeDarkBrushes[]`, freed by `ReleaseThemeGraphics()` `:112`. Conversion is essentially complete — **`DrawEdge` has exactly one raw caller left (the passthrough inside `themes.cpp:133`)**.

**2. Global GDI object pool.** `src/salamdr1.cpp:1935-1940` and `:2581-2614` build `HDialogBrush`, `HButtonTextBrush`, `HMenuSelectedBkBrush`, `HMenu*Brush`, `BtnShadowPen`, `BtnHilightPen`, `Btn3DLightPen`, `BtnFacePen`, `WndFramePen`, `WndPen`, panel/thumbnail pens — all from `ThemeSysColor`/`CurrentColors`. Owner-drawn menus (`menu2.cpp:2872`, `menu3.cpp:*`), menubar (`menubar.cpp:249,850`), panel header (`filesbx2.cpp:217`) just `FillRect(..., HDialogBrush)` and are themed for free.

**3. `WM_CTLCOLOR*` central handler.** `ThemeHandleCtlColor(uMsg, wParam, lParam, INT_PTR* result)` `src/themes.cpp:696`. Returns FALSE in Default theme. Wired at exactly three places in the main app:
- `CCommonDialog::DialogProc` — `src/dialogs2.cpp:258-263` (first statement, before its own switch)
- `CCommonPropSheetPage::DialogProc` — `src/dialogs2.cpp:348-353`
- `CMainWindow::WindowProc` for the command line — `src/mainwnd3.cpp:1240-1249`
- plus the propsheet-frame subclass `src/themes.cpp:828-839`

Because derived dialogs call the base **last**, a dialog's own `WM_CTLCOLOR*` handling still wins. That ordering is correct.

**4. Per-child `SetWindowTheme` sweep.** `ThemeApplyToDialog(HWND)` `src/themes.cpp:679` → `EnumChildWindows(ThemeApplyChildEnumProc)` `src/themes.cpp:564-677`. Per class:

| Class | Action |
|---|---|
| `Button` | `DarkMode_Explorer`; **except** `BS_RADIOBUTTON`/`BS_AUTORADIOBUTTON`/`BS_GROUPBOX` → `SetWindowTheme(L"",L"")` (classic) so `WM_CTLCOLORSTATIC` text color applies (`:578-582`) |
| `Edit` | `DarkMode_CFD`, or `DarkMode_Explorer` if it has `WS_VSCROLL/WS_HSCROLL`; + `ThemeFlatDisabledEditSubclassProc` (`:375`) |
| `Static` | `SS_ETCHED*` → `ThemeEtchedLineSubclassProc` (`:333`); otherwise → `ThemeFlatDisabledTextSubclassProc` (`:266`) |
| `msctls_statusbar32` | `ThemeStatusBarSubclassProc` (`:436`) — full repaint incl. parts, borders, forwarded `WM_DRAWITEM`, size grip |
| `ComboBox`/`ComboBoxEx32` | `DarkMode_CFD` |
| `SysListView32` | `DarkMode_Explorer` + `ListView_SetBkColor/TextBkColor/TextColor` + header → `DarkMode_ItemsView` |
| `SysTreeView32` | `DarkMode_Explorer` + `TreeView_SetBkColor/SetTextColor` |
| `SysHeader32` | `DarkMode_ItemsView` |
| anything else | if `WS_VSCROLL\|WS_HSCROLL` → `DarkMode_Explorer` (this is the **only** scrollbar-darkening mechanism) |

**5. Title bars.** `ThemeApplyToTopLevel(HWND)` `src/themes.cpp:211` → `DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE=20, &BOOL, 4)`. Guarded by the window property `"SalThemeDark"` (`src/themes.cpp:26`) so Default theme is a strict no-op on windows never darkened.

Supporting helpers: `ThemeUpdateWindowClassBackground(HWND, int lightSysColor)` `src/themes.cpp:227` (swaps `GCLP_HBRBACKGROUND` between the engine brush and `(HBRUSH)(idx+1)`), `ThemeUpdateRebarStyle(HWND)` `:237` (strips `WS_BORDER|RBS_BANDBORDERS` — classic rebar chrome is light-only), `ThemeSubclassPropSheetFrame(HWND)` `:862`, `ThemeAdjustBitmapForDarkMode(HBITMAP, COLORREF)` `:737`.

**There is no CBT hook / no `WH_CBT` / no global dialog hook.** Theming is strictly opt-in per window, and the opt-in point is `CCommonDialog`/`CCommonPropSheetPage`. Also no `SetPreferredAppMode`/`AllowDarkModeForWindow` (undocumented uxtheme ordinals) are used — hence the manual per-control `SetWindowTheme` sweep.

## A4. Menus and scrollbars

- **Menus**: the app does *not* use Win32 menus for its own UI. `CMenuPopup` is a fully owner-drawn popup on class `"PopupMenuClass"` (`menu1.cpp:9`, created `menu2.cpp:2656`), erased with `HDialogBrush` (`menu2.cpp:2869-2875`), items drawn in `menu3.cpp` (15 `ThemeSysColor` sites). `CMenuBar` likewise. So the main window's whole menu system is themed by construction.
- **Scrollbars**: only via `SetWindowTheme(..., L"DarkMode_Explorer")` in the enum proc — nothing else. Any window that gets a scrollbar without passing the enum keeps a light scrollbar.

## A5. Non-obvious extension point

`src/common/sheets.cpp:29-34` — `SheetsGetSysColorHook`, a function pointer installed once at `src/salamdr1.cpp:1884` (`SheetsGetSysColorHook = ThemeSysColor;`). This is how the winlib-level tree property-sheet holder gets themed without `common/` depending on `themes.h`. `src/common/` has **zero** direct references to the theme engine.

## A6. Public API surface (themes.h)

```
BOOL     IsDarkThemeActive()                                     themes.h:23  / themes.cpp:49
void     RefreshThemeHighContrastState()                         themes.h:27  / themes.cpp:38
COLORREF ThemeSysColor(int index)                                themes.h:32  / themes.cpp:81
HBRUSH   ThemeSysColorBrush(int index)                           themes.h:36  / themes.cpp:99
BOOL     ThemeDrawEdge(HDC,RECT*,UINT edge,UINT flags)           themes.h:39  / themes.cpp:130
void     UpdateCurrentColorsForTheme()                           themes.h:44  / themes.cpp:193
void     SetThemeMode(DWORD mode)                                themes.h:49  / themes.cpp:877
void     ThemeApplyToTopLevel(HWND)                              themes.h:54  / themes.cpp:211
void     ThemeApplyToDialog(HWND)                                themes.h:59  / themes.cpp:679
void     ThemeUpdateWindowClassBackground(HWND,int)              themes.h:64  / themes.cpp:227
void     ThemeUpdateRebarStyle(HWND)                             themes.h:69  / themes.cpp:237
BOOL     ThemeHandleCtlColor(UINT,WPARAM,LPARAM,INT_PTR*)        themes.h:73  / themes.cpp:696
void     ThemeSubclassPropSheetFrame(HWND)                       themes.h:78  / themes.cpp:862
void     ThemeAdjustBitmapForDarkMode(HBITMAP,COLORREF)          themes.h:85  / themes.cpp:737
void     ReleaseThemeGraphics()                                  themes.h:88  / themes.cpp:112
```
Plugin-facing mirrors (`src/zip.cpp:5277,5282,5287`, declared `plugins/shared/spl_gen.h:3488,3494`): `ThemeApplyToDialog`, `ThemeApplyToTopLevel`, `ThemeHandleCtlColor`.

---

# (B) GAPS — ranked by user-visible impact

### B1. Internal viewer's menu bar is a raw Win32 `HMENU` → permanently light
`src/viewer.cpp:1548` `ViewerMenu = LoadMenu(HLanguage, MAKEINTRESOURCE(IDM_VIEWERMENU))`, attached as the window menu at `src/viewer2.cpp:267`. No `MENUINFO`/`MIM_BACKGROUND`, no `MFT_OWNERDRAW`, no `WM_MEASUREITEM`/`WM_DRAWITEM` anywhere in `viewer*.cpp`. Windows draws menu bars with real system colors regardless of app dark mode → a permanent white strip across the top of every viewer window, plus light dropdowns. Its popups (`src/viewer3.cpp:2877` `TrackPopupMenuEx` on a `ViewerMenu` submenu) are light too. Highest-impact remaining gap: the main window solved this with `CMenuBar`/`CMenuPopup`; the viewer never got that treatment.

### B2. Panel quick-rename edit box created untouched at runtime → white box on the dark panel
`src/fileswn5.cpp:2749-2757` — `QuickRenameWindow.CreateExW(0, L"edit", ..., GetListBoxHWND(), ...)`. No `SetWindowTheme(L"DarkMode_CFD")`, and its parent `CFilesBox` (`filesbx1/2.cpp`) has no `WM_CTLCOLOREDIT` handler, so `DefWindowProc` hands back the light `COLOR_WINDOW` brush. F2 / slow-double-click rename is a hot path — white edit with black text over `#202020`.

### B3. `CExecuteWindow` paints near-white text on a light-gray system background
`src/pack3.cpp:2025-2026` — `WM_ERASEBKGND` calls `CWindow::WindowProc` (→ `DefWindowProc`) which fills with the class brush, then line 2034 draws with `ThemeSysColor(COLOR_BTNTEXT)` (= `RGB(240,240,240)`). The class `SAVEBITS_CLASSNAME` is registered `(HBRUSH)(COLOR_3DFACE + 1)` at `src/salamdr1.cpp:4325`, never swapped by `ThemeUpdateWindowClassBackground`. Result: **light background + light text = unreadable**. (Contrast: `CWaitWindow` on the same class is correct because `src/dialogs3.cpp:2760` fills with `ThemeSysColorBrush(COLOR_BTNFACE)` itself and returns TRUE.)

### B4. Both universal window classes keep light system class brushes
`src/salamdr1.cpp:4325` (`SAVEBITS_CLASSNAME`) and `:4334` (`SHELLEXECUTE_CLASSNAME`) use `(HBRUSH)(COLOR_3DFACE + 1)`. Only `CMAINWINDOW_CLASSNAME`, `CFILESBOX_CLASSNAME*` and `CVIEWERWINDOW_CLASSNAME` are ever swapped (`salamdr1.cpp:4414`, `mainwnd1.cpp:3277`, `mainwnd3.cpp:1259`, `viewer2.cpp:279`, `viewer3.cpp:709`). Feeds B3 and any future user of those classes.

### B5. `EnumChildWindows` is a one-shot snapshot — controls born later are never themed
`src/themes.cpp:688`. Anything created after the sweep escapes permanently:
- ListView in-place label edits: `src/dialogs4.cpp:1372-1373`, `:1464`, `:3129-3130`, `:3164` (Configuration → Views, Hot Paths) — white edit inside a dark listview.
- Any control created in a handler after `WM_INITDIALOG`. The Find dialog is the known instance and it works around this by calling `ThemeApplyToDialog` a **second** time (`src/finddlg1.cpp:3106-3109`) — evidence the pattern is fragile and undiscoverable.

### B6. Dialogs bypassing `CCommonDialog` get zero theming
Only two in the main app derive straight from `CDialog`:
- `CSplashScreen` — `src/dialogs.h:815`, proc `src/logo.cpp:293`. Fully self-painted branded bitmap; stays light on startup in dark mode (arguably intentional, but it is an unthemed top-level window with a light title-bar-less frame).
- `CTreePropHolderDlg` — `src/common/sheets.h:175`, proc `src/common/sheets.cpp:677`. Saved only **indirectly**: the first `CCommonPropSheetPage::NotifDlgJustCreated` (`src/dialogs2.cpp:344`) subclasses it via `ThemeSubclassPropSheetFrame(::GetParent(HWindow))`. If a sheet ever had zero `CCommonPropSheetPage` pages, the whole config frame would stay light.

Also `src/common/sheets.cpp:702` `SetWindowTheme(HTreeView, L"explorer", NULL)` unconditionally forces the **light** Explorer theme on the config page tree; it is only corrected afterwards by the page-driven `ThemeApplyToDialog`. Order-dependent, no dark-aware branch of its own.

### B7. Native `HMENU` context menus in dialogs stay light
`CreatePopupMenu` + `TrackPopupMenuEx` with no owner-draw at: `src/dialogs3.cpp:187/209/547` (drive info, browse menus), `src/dialogs4.cpp:1819/1876/3785` (font/color pickers, hot paths), `src/dialogs5.cpp:2964`, `src/execute.cpp:2002-2037`, `src/finddlg2.cpp:1512`, `src/gui.cpp:1349-1351` (hyperlink "Copy to clipboard"), `src/mainwnd3.cpp:7003-7009` (tray icon menu), `src/drivelst.cpp:2956`. Each is a small light popup over dark UI. Shell-owned context menus (`src/shellsup.cpp:42/101`, `src/shellib.cpp:2743`) are the same problem but are externally drawn.

### B8. `DrawFrameControl` sites that ignore the palette
- `src/edtlbwnd.cpp:555` — `DFC_SCROLL | DFCS_SCROLLRIGHT`: the arrow button on every `CEditListBox` (Configuration → User Menu / Hot Paths / Viewers / Editors, Find). Classic light 3D button glued onto a dark list.
- `src/gui.cpp:3873/3879` — `DFC_BUTTON` checkbox fallback in `CreateCheckboxImagelist`. Guarded: there **is** a dark branch at `src/gui.cpp:3800-3840`, so this is reached only when unthemed. Low risk.
- `src/menu3.cpp:138` — renders into a 1bpp mask bitmap. Not a gap.

### B9. Direct `GetSysColor` survivors
Full census in the main app (excluding `themes.*`): **34 hits, 3 files, only one is a real bug.**
- **`src/dialogs4.cpp:1362-1363`** — `CCfgPageView::DialogProc` `WM_SYSCOLORCHANGE`: `ListView_SetBkColor(HListView, GetSysColor(COLOR_WINDOW))`. The single unconverted one — **every** sibling uses `ThemeSysColor` (`dialogs2.cpp:1154`, `dialogs3.cpp:2941`, `dialogs5.cpp:918/1270`, `dialogs6.cpp:720/1394/2615`, `finddlg1.cpp:4621`, `finddlg2.cpp:307`, `packac.cpp:330`). Configuration → Views listviews turn white after any system color change / HC toggle.
- `src/salamdr1.cpp:1664-1737` (21), `src/viewer.cpp:1528-1534` (4) — `UpdateDefaultColors`/`UpdateViewerColors` **seeding** logic for `SCF_DEFAULT` entries. Correct per contract line 65 ("never seeding/luminance logic").
- `src/salamdr1.cpp:2630-2631`, `src/finddlg1.cpp:4158` — explicit light-branch comparisons. Correct.

### B10. Hardcoded `RGB()` literals in live painting
Census: 33 in `salamdr1.cpp`, 16 `menu3.cpp`, 9 `gui.cpp`, 9 `fileswn4.cpp`, 8 `dialogs3.cpp`, 6 `logo.cpp`, 5 `dialogs4.cpp`, 4 each `toolbar4/iconlist/fileswn9`, rest single digits. **Most are mask/transparency keys**, not real colors: `RGB(255,0,255)` magenta key (`salamdr1.cpp:2432/2447`, `gui.cpp:2734`, `toolbar4.cpp:901`), monochrome mask black/white (`fileswn4.cpp:672-673/1552-1553/1959-1960`, `fileswn9.cpp:1730-1811`, `filesmap.cpp:679-680`, `toolbar4.cpp:896-897`). Real ones:
- `src/gui.cpp:1146` — `STF_HYPERLINK_COLOR` → `SetTextColor(hDC, RGB(0,0,255))`. Pure blue on `#2d2d2d` ≈ 2.3:1, fails the feature's own 4.5:1 target (`COLOR_HOTLIGHT` is already mapped to `RGB(102,178,255)` in the palette and is the obvious substitute). Appears in About / config hyperlinks.
- `src/toolbar3.cpp:522` — `SetTextColor(hDC, RGB(0,0,0))` before `DrawFocusRect` in the toolbar customize dialog.
- `src/dialogs3.cpp:1715-1725` — `CDriveInfo` free/used pie colors, hardcoded both branches. Saturated data colors; readable but not palette-driven.
- `src/salamdr1.cpp:2631` — `RGB(200,200,200)` glyph highlight; intentional dark-branch constant, should arguably live in `themes_palette.h`.
- `src/logo.cpp` (6) — splash bitmap composition; see B6.

### B11. `(HBRUSH)(COLOR_X + 1)` survivors
`src/common/winlib.cpp:487` and `:510` — `CWindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1)` for **every** window registered through `RegisterUniversalClass`/`RegisterUniversalClassW`. Individual windows patch this per-HWND afterwards; any custom `CWindow` that doesn't gets a white background. `src/common/sheets.cpp:610` — `FillRect(hdc, &r, (HBRUSH)(COLOR_GRAYTEXT + 1))`, config header fallback on ≤256-color devices only.

### B12. Threading — engine is documented main-thread-only but called from the viewer thread
Contract invariant 3 (`theme-engine.md:18`) says main-thread only. The viewer runs its own message loop on its own thread and calls `ThemeUpdateWindowClassBackground` + `ThemeApplyToTopLevel` + `ThemeSysColorBrush` from it: `src/viewer2.cpp:279-281` and `src/viewer3.cpp:709-711`. `ThemeSysColorBrush` (`themes.cpp:105-106`) lazily `CreateSolidBrush`es into the unguarded global `ThemeDarkBrushes[]`. The mitigation is a comment + eager warm-up of only two indices (`themes.cpp:201-202`, `COLOR_BTNFACE`/`COLOR_WINDOW`); any other index requested first from a viewer thread races. Also `SetClassLongPtr(GCLP_HBRBACKGROUND)` from a non-owning thread mutates process-global class state.

---

# (C) THEME-REFRESH PATH

## C1. Live switch (`SetThemeMode`, `src/themes.cpp:877-910`)
1. write `Configuration.ThemeMode`; `RefreshThemeHighContrastState()`; `UpdateCurrentColorsForTheme()`
2. `ThemeUpdateWindowClassBackground(MainWindow->HWindow, COLOR_WINDOW)`
3. `ColorsChanged(TRUE, FALSE, TRUE)` — `src/salamdr1.cpp:3135-3172`: rebuild all brushes/pens/imagelists → `MainWindow->OnColorsChanged()` (`src/mainwnd1.cpp:3176-3282`, which itself ends with `ThemeUpdateWindowClassBackground` + `ThemeUpdateRebarStyle` + `ThemeApplyToDialog(HWindow)`) → `FindDialogQueue.BroadcastMessage(WM_USER_COLORCHANGEFIND)` → `Plugins.Event(PLUGINEVENT_COLORSCHANGED)` → `RB_SETBKCOLOR` on the rebar → `BroadcastConfigChanged()`
4. `MainWindow->RebuildRebarBands()` (grippers are classic-drawn), `ThemeApplyToDialog(MainWindow->HWindow)`, `SWP_FRAMECHANGED` + `RedrawWindow(RDW_ALLCHILDREN)`

## C2. The broadcast reaches exactly two queues
`BroadcastConfigChanged()` — `src/mainwnd3.cpp:552-557`:
```
ViewerWindowQueue.BroadcastMessage(WM_USER_CFGCHANGED, 0, 0);
FindDialogQueue.BroadcastMessage(WM_USER_CFGCHANGED, 0, 0);
```
Handled by: viewer `src/viewer3.cpp:702-715` (rebuilds brushes + re-applies title bar/class bg/`DarkMode_Explorer`) and Find `src/finddlg2.cpp:307-312` (`ThemeApplyToDialog` + `UpdateProgressBarTheme`).

## C3. Who misses it
- **Any open modal or modeless dialog other than Find.** No queue, no broadcast. The config property sheet, progress dialogs, `CMessageBox` etc. receive nothing. In practice the switch is driven from the main menu so nothing else is usually open — but it is a structural hole, and a re-opened dialog only re-themes because `NotifDlgJustCreated` runs again.
- **`CWaitWindow` / `CExecuteWindow`** — no registration in any queue (transient, so low practical impact beyond B3).
- **Plugin windows** get `PLUGINEVENT_COLORSCHANGED` but each plugin must re-call `ThemeApplyToDialog`/`ThemeApplyToTopLevel` itself. Callers today: sftp, uncab, zip, ftp, pictview, mdview, dbviewer, filecomp, diskmap. Non-callers (7zip, checksum, folders, mmviewer, nethood, peviewer, regedt, renamer, undelete, uniso, automation, demo*) stay light.

## C4. System-driven refresh
- `WM_SYSCOLORCHANGE` — `src/mainwnd3.cpp:1251-1269`: `RefreshThemeHighContrastState()` → `UpdateCurrentColorsForTheme()` → `ThemeUpdateWindowClassBackground` → `ThemeApplyToTopLevel` → `ColorsChanged(TRUE,FALSE,TRUE)`. This is the only place High Contrast toggling is picked up.
- `WM_SETTINGCHANGE` — `src/mainwnd3.cpp:1271-1327`: **does not call `RefreshThemeHighContrastState()`**. It calls `BroadcastConfigChanged()` but not `ColorsChanged`. `SPI_SETHIGHCONTRAST` arrives as `WM_SETTINGCHANGE`; if a given Windows build does not also send `WM_SYSCOLORCHANGE`, the cached `ThemeHighContrast` goes stale. Also `WM_SETTINGCHANGE`/`lParam=="ImmersiveColorSet"` (the OS light/dark toggle) is not handled at all.
- `WM_THEMECHANGED` — **handled nowhere in the main app.** All the `SetWindowTheme` state applied by the enum proc is silently reset by comctl32 on a visual-style change with no re-apply.
- Per-dialog `WM_SYSCOLORCHANGE` handlers (`dialogs2.cpp:1152`, `dialogs4.cpp:760/1360/2403`, `dialogs5.cpp:916/1268`, `dialogs6.cpp:718/1392/2613`, `finddlg1.cpp:4619`, `packac.cpp:328`, `sheets.cpp:940`) only re-set listview/treeview bk colors — **none re-runs `ThemeApplyToDialog`**, so subclasses and `SetWindowTheme` assignments are not refreshed for open dialogs.

## C5. Contract deviation worth flagging
`theme-engine.md:68` requires `UpdateViewerColors(CurrentViewerColors)` on the ColorsChanged path. `src/salamdr1.cpp:3143` actually calls `UpdateViewerColors(ViewerColors)` — the light array. Harmless today (dark viewer entries are explicit RGB with flag 0, so `SCF_DEFAULT` seeding would no-op anyway) but it diverges from the normative contract and will bite if a dark entry is ever given `SCF_DEFAULT`.

---

# (D) CANONICAL EXTENSION POINTS A FIX SHOULD USE

| Need | Use | Definition |
|---|---|---|
| Any new dialog | derive from `CCommonDialog` / `CCommonPropSheetPage` — theming is automatic | `src/salamand.h:755` / `:813` |
| Non-`CCommonDialog` dialog or a control created after `WM_INITDIALOG` | call `ThemeApplyToDialog(HWindow)` again (idempotent) | `src/themes.cpp:679`; precedent `src/finddlg1.cpp:3106-3109` |
| Custom `WindowProc` handling `WM_CTLCOLOR*` | `ThemeHandleCtlColor` first, return its result when TRUE | `src/themes.cpp:696`; precedent `src/mainwnd3.cpp:1240-1249` |
| New top-level window | `ThemeApplyToTopLevel` + `ThemeUpdateWindowClassBackground(hwnd, COLOR_WINDOW)` + `SetWindowTheme(hwnd, L"DarkMode_Explorer")` if it has scrollbars | `themes.cpp:211` / `:227`; precedent `src/viewer2.cpp:277-282` |
| A single runtime-created control | `SetWindowTheme(h, L"DarkMode_CFD")` for edits/combos, `L"DarkMode_Explorer"` for scrollables | precedent `src/editwnd.cpp:1706-1710` |
| Drawing code | `ThemeSysColor` / `ThemeSysColorBrush` / `ThemeDrawEdge`, never `GetSysColor`/`GetSysColorBrush`/`(HBRUSH)(COLOR_X+1)`/`DrawEdge` | `themes.cpp:81/99/130` |
| New palette color | add an `ENTRY` to `THEME_DARK_SYSCOLORS` (or the panel/viewer tables) — never a literal `RGB()` at the draw site | `src/common/themes_palette.h:19/55/93` |
| `src/common/` code (must not include `themes.h`) | route through `SheetsGetSysColorHook` or add a sibling hook pointer | `src/common/sheets.h:15`, installed `src/salamdr1.cpp:1884` |
| A control class comctl32 won't darken | add a branch to `ThemeApplyChildEnumProc` + a `WM_PAINT` subclass with a fresh subclass ID (1..5 are taken: 1 propsheet frame, 2 static, 3 etched, 4 edit, 5 statusbar) | `src/themes.cpp:564-677` |
| Live-switch reactions for a new window kind | register in a broadcast queue and handle `WM_USER_CFGCHANGED`, or extend `BroadcastConfigChanged` | `src/mainwnd3.cpp:552`; precedent `src/viewer3.cpp:702` |
| Legacy raster glyphs | `ThemeAdjustBitmapForDarkMode`; for vector, `ThemeDarkAdaptColor` | `themes.cpp:737` / `themes_palette.h:117` |
| Plugin windows | `SalamanderGeneral->ThemeApplyToDialog` / `ThemeApplyToTopLevel` / `ThemeHandleCtlColor` | `plugins/shared/spl_gen.h:3488/3494`, impl `src/zip.cpp:5277-5291` |

Agent's closing structural suggestions (recorded verbatim; item 2 violates the documented-APIs-only invariant and must not be adopted): (1) make the theme sweep re-entrant on a message rather than a one-shot enum — a `WM_THEMECHANGED` handler plus a re-apply on `WM_SYSCOLORCHANGE` in `CCommonDialog` would close B5 and most of C4 at once; (2) `SetPreferredAppMode(AllowDark)` via the uxtheme ordinal would close B7 (native `HMENU` popups) and B1's dropdowns wholesale.
