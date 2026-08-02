# Raw findings — Agent 3: Scrollbars on ALT+3/4/5 & window-recreation paths

> Unedited output of the scrollbar/view-switch exploration agent (feature 049 initial audit).

## (A) Root cause — evidence chain

**Command → recreation → no re-theme.**

1. **Command entry points for Alt+3/4/5**
   - Direct key handling in the panel: `src\fileswn0.cpp:1324-1338` — `altPressed && wParam >= '0' && '9'` → `index = wParam-'0'; index--` → `SelectViewTemplate(index, TRUE, FALSE)` (Alt+3→index 2 = detailed, Alt+4→3 = icons, Alt+5→4 = thumbnails).
   - Menu/toolbar/accelerator IDs: `src\mainwnd3.cpp:2524-2546` (`CM_ACTIVEMODE_1..10`, `CM_LEFTMODE_*`, `CM_RIGHTMODE_*`) → `SelectViewTemplate`. IDs at `src\resource.rh2:492-499`.
   - Alt+mouse-wheel over the panel: `src\filesbx1.cpp:1735-1745`.

2. **View switch → child window destroy/create**
   - `CFilesWindow::SelectViewTemplate` → `src\fileswn2.cpp:1101` `ListBox->SetMode(newViewMode, headerLine);`
   - `CFilesBox::SetMode` → `src\filesbx1.cpp:82-90`: `BOOL change = ShowHideChilds(); LayoutChilds(change);`
   - `CFilesBox::ShowHideChilds` → `src\filesbx1.cpp:2128-2214`:
     - `2140` `DestroyWindow(BottomBar.HWindow);` (kills the child `HHScrollBar`), `2142` `HHScrollBar = NULL;`
     - `2149` `DestroyWindow(HVScrollBar);`
     - `2159` `DestroyWindow(HeaderLine.HWindow);`
     - `2167` `BottomBar.Create(CWINDOW_CLASSNAME2, …)`
     - `2173` `HHScrollBar = CreateWindow("scrollbar", "", WS_CHILD | SBS_HORZ | …, BottomBar.HWindow, …)`
     - `2191` `HVScrollBar = CreateWindow("scrollbar", "", WS_CHILD | SBS_VERT | …, HWindow, …)`
     - `2204` `HeaderLine.Create(CWINDOW_CLASSNAME2, …)`
   - **No `SetWindowTheme` / theme call anywhere in `filesbx1.cpp`** — the only theme reference in the whole file is `ThemeDrawEdge` at `src\filesbx1.cpp:1359`.

3. **Where the dark scrollbar theme actually comes from (and why it is never re-applied)**
   - The panel scrollbars are *separate `"scrollbar"`-class child HWNDs*, not NC scrollbars of `CFilesBox`. They are darkened only by the whole-main-window child sweep:
     `src\themes.cpp:603-607` in `ThemeApplyChildEnumProc`:
     ```c
     else if (_stricmp(className, "ListBox") == 0 ||
              _stricmp(className, "ScrollBar") == 0)
         SetWindowTheme(hChild, dark ? L"DarkMode_Explorer" : NULL, NULL);
     ```
   - Driven from `ThemeApplyToDialog` → `src\themes.cpp:688` `EnumChildWindows(hDialog, ThemeApplyChildEnumProc, (LPARAM)dark);`
   - The only two callers that reach the main window are:
     - `CMainWindow::OnColorsChanged` — `src\mainwnd1.cpp:3280-3281` `if (HWindow != NULL) ThemeApplyToDialog(HWindow);` (reached only via global `ColorsChanged()`, `src\salamdr1.cpp:3152-3155`)
     - `SetThemeMode` — `src\themes.cpp:903` `ThemeApplyToDialog(MainWindow->HWindow);`
   - `ColorsChanged()` has only four call sites in the app: `src\mainwnd2.cpp:2545` (LoadConfig — the *startup* theming pass), `src\dialogs4.cpp:3545` (Configuration dialog OK), `src\mainwnd3.cpp:1267` (`WM_SYSCOLORCHANGE`), `src\themes.cpp:894` (theme switch).

**Conclusion:** `SetWindowTheme` is a *per-HWND* attribute. `ShowHideChilds()` destroys the old (themed) scrollbar HWNDs and creates brand-new ones; nothing on the view-switch path re-runs the `EnumChildWindows`/`SetWindowTheme` pass, so the new `"scrollbar"` controls come up with the default (light) visual style. It is not a style-reset — it is a **new, never-themed HWND**. The theme "comes back" only when one of the four `ColorsChanged()` triggers fires (config dialog OK, system color change, theme toggle) — hence "until some other action restores it".

Per-transition loss: Brief→Detailed creates a new `HVScrollBar` (light); Detailed/Brief→Icons/Thumbnails/Tiles destroys the bottom bar + `HHScrollBar`; Icons→Brief destroys `HVScrollBar` and creates a light `HHScrollBar`. The corroborating comment already exists in the codebase for the same class of bug: `src\editwnd.cpp:1706-1710` ("the command line is created after the startup theming pass over the main-window children, so theme it here").

## (B) All recreation / style-change paths and their re-theming status

| # | Path | File:line (destroy → create) | Windows affected | Needs per-HWND theming? | Re-themed? |
|---|---|---|---|---|---|
| 1 | **Panel view-mode switch** (`SelectViewTemplate`→`SetMode`→`ShowHideChilds`) | `fileswn2.cpp:1101` → `filesbx1.cpp:82-90` → `filesbx1.cpp:2140,2149,2159` → `2167,2173,2191,2204` | `"scrollbar"` H+V, `CBottomBar`, `CHeaderLine` | **YES** (`themes.cpp:603-607`) | **NO — root cause** |
| 1a | ↳ entry: Alt+digit in panel | `fileswn0.cpp:1324-1338` | " | " | NO |
| 1b | ↳ entry: `CM_ACTIVEMODE_*`/`CM_LEFTMODE_*`/`CM_RIGHTMODE_*` menu+toolbar | `mainwnd3.cpp:2524-2546` | " | " | NO |
| 1c | ↳ entry: Alt+wheel over panel | `filesbx1.cpp:1735-1745` | " | " | NO |
| 1d | ↳ entry: smart-column-mode toggle | `mainwnd3.cpp:727, 732` | " | " | NO |
| 1e | ↳ entry: config/view-template apply, font/DPI reload | `mainwnd3.cpp:1885-1886, 4303-4304`; `mainwnd1.cpp:2653-2655` | " | " | NO |
| 1f | ↳ entry: **plugin FS / archive** `CSalamanderView::SetViewMode` | `salamdr4.cpp:1544-1577` (→`SelectViewTemplate`) | " | " | NO (hit on every archive/FS enter by plugins that force Detailed: `plugins\ftp\fs3.cpp:1182`, `plugins\folders\fs1.cpp:396`, `plugins\regedt\fs4.cpp:259`) |
| 1g | ↳ entry: panel `WM_CREATE` / config load | `fileswnb.cpp:1084`; `mainwnd2.cpp:2243` | " | " | Covered incidentally — startup `ColorsChanged` at `mainwnd2.cpp:2545` runs *after* |
| 2 | **Header-line toggle** `CM_LEFTHEADER`/`CM_RIGHTHEADER` | `mainwnd3.cpp:3186-3198` → `fileswn2.cpp:993-1000` → same `ShowHideChilds` | `CHeaderLine` (self-painted, no theme needed); **but forces `vmDetailed` when current mode is icons/thumb/tiles → creates fresh scrollbars** | conditional YES | **NO** |
| 3 | Directory-line toggle | `fileswn2.cpp:921-961` (932 → 936) | `CStatusWindow` (self-painted) + `CToolBar` | no | n/a (no loss) |
| 4 | Status/info-line toggle | `fileswn2.cpp:963-991` (972 → 975) | `CStatusWindow` | no | n/a |
| 5 | Status-window toolbar toggle | `stswnd.cpp:645 → 650` | `CToolBar` | no (toolbar class not handled in `themes.cpp:564-677`) | n/a |
| 6 | **Command-line show/hide** | `mainwnd4.cpp:1305` → `mainwnd4.cpp:1284` → `editwnd.cpp:1691-1710` | ComboBox + inner Edit + STATIC | YES | **YES** — `editwnd.cpp:1709-1710` `SetWindowTheme(hWnd, L"DarkMode_CFD")`. *Precedent.* (inner Edit `1713` / STATIC `1719` rely on combo theme + `mainwnd3.cpp:1240-1249` `WM_CTLCOLOR*`) |
| 7 | Toolbar toggles: top/plugins/middle/usermenu/hotpaths/drive/bottom | `mainwnd1.cpp:556-830` (571,607,642,675,710,749,757,806 → 578,614,647,682,717,763,765,815) | `ToolbarWindow32` | no | n/a; band grip style re-read at insert (`mainwnd1.cpp:970,1040,1076,1112,1149,1194`), bands rebuilt on theme switch (`themes.cpp:900-901`) |
| 8 | Rebar style change (`SetWindowLong(GWL_STYLE)`+`SWP_FRAMECHANGED`) | `themes.cpp:237-254` (`ThemeUpdateRebarStyle`), called `mainwnd1.cpp:3279` | rebar | YES | YES |
| 9 | Find: progress bar in status bar | `finddlg1.cpp:1576` → `1563-1569` | `msctls_progress32` | YES | **YES** — `UpdateProgressBarTheme()` `finddlg1.cpp:1522-1543`, called at `1569`. *Precedent.* |
| 10 | Find: MenuBar / TBHeader / toolbars | `finddlg1.cpp:4565, 4576`; `finddlg2.cpp:512, 540` | custom bars/toolbars | no | n/a |
| 11 | **Panel quick-rename inline edit** | `fileswn5.cpp:2796` → `fileswn5.cpp:2749-2757` (`L"edit"`, parent = `CFilesBox`) | Edit control | **YES** (`themes.cpp:584-601`: `DarkMode_CFD` + disabled-edit subclass) | **NO** — and `CFilesBox` has no `WM_CTLCOLOREDIT` handler (no `WM_CTLCOLOR*` anywhere in `filesbx*.cpp`/`fileswn*.cpp`) |
| 12 | **Edit-list-box inline editor** (config dialogs: user menu, hot paths, filters…) | `edtlbwnd.cpp:514` → `edtlbwnd.cpp:474-484` | Edit control created *after* `ThemeApplyToDialog` ran on the dialog | **YES** | **NO** (no theme call in `edtlbwnd.cpp`) |
| 13 | Plugin bars (`CPluginsBar` band items) | `plugins3.cpp:395, 438` → `plugins3.cpp:465` | toolbar | disable-visual-styles only | YES — `SetWindowTheme(hWindow, L" ", L" ")` applied per create |
| 14 | Viewer window | created `viewer2.cpp:261` | NC `WS_VSCROLL\|WS_HSCROLL` | YES | YES — `viewer2.cpp:280-281` at create, `viewer3.cpp:709-711` on `WM_USER_CFGCHANGED`; never recreated |
| 15 | Property-sheet frame | `themes.cpp:862-870` | frame + children | YES | YES |
| 16 | Main-window shutdown teardown | `mainwnd3.cpp:6890-6959`; panel teardown `fileswnb.cpp:1108, 1112` | all | n/a | shutdown only |
| 17 | Tooltips | `mainwnd3.cpp:5100 / 5247`; `viewer3.cpp:3534` | `tooltips_class32` | not handled by the engine at all | n/a |

Nothing in the repo uses `SetWindowLong(GWL_STYLE)` + `SWP_FRAMECHANGED` on the panel; the panel path is pure destroy/create (so the "style change resets SetWindowTheme" hypothesis does not apply here).

## (C) Theme-engine "apply to window tree" helper

- **Exists only privately**: `static BOOL CALLBACK ThemeApplyChildEnumProc(HWND hChild, LPARAM lParam)` — `src\themes.cpp:564-677` (handles `ScrollBar` at `603-607`).
- Its only public wrapper is `void ThemeApplyToDialog(HWND hDialog)` — `src\themes.cpp:679-694` (declared `src\themes.h:59`), which is top-level-oriented: it also calls `ThemeApplyToTopLevel` (`themes.cpp:687` → `DwmSetWindowAttribute(DWMWA_USE_IMMERSIVE_DARK_MODE)`, `themes.cpp:211-225`) and stamps `THEME_DARKENED_PROP` (`themes.cpp:26`).
- There is **no** public per-HWND / per-subtree helper (`ThemeApplyToWindow`, `ThemeApplyToChildren`, …) — that is the missing piece the fix needs.

### Recommended hook points

1. **Primary (fixes the reported defect):** end of `CFilesBox::ShowHideChilds()` — `src\filesbx1.cpp:2213` (just before `return change;`), or equivalently in `CFilesBox::SetMode` right after line `filesbx1.cpp:86`. Re-theme `HHScrollBar` and `HVScrollBar` (and, for symmetry, `BottomBar.HWindow` / `HeaderLine.HWindow`) whenever `change == TRUE`. This single site covers **all** of rows 1a–1g and row 2 in the table, because every view-mode entry point funnels through it.
2. **Engine change enabling it:** promote the existing enum proc into a public helper in `src\themes.h` / `src\themes.cpp` (e.g. `void ThemeApplyToWindowTree(HWND hWnd)` = apply `ThemeApplyChildEnumProc` to `hWnd` itself + `EnumChildWindows(hWnd, ThemeApplyChildEnumProc, dark)`), reusing `themes.cpp:564` and `themes.cpp:688`. Calling `ThemeApplyToDialog(ListBox->HWindow)` would also work mechanically but drags in the DWM title-bar call and the `SalThemeDark` prop, which are top-level semantics.
3. **Secondary sites for the same helper (same class of defect, currently un-themed):** `src\fileswn5.cpp:2757` (quick-rename edit, right after `CreateExW` succeeds) and `src\edtlbwnd.cpp:485` (edit-list-box inline editor, right after `EditLine->Create`).
4. Existing in-repo precedents to mirror: `src\editwnd.cpp:1709-1710` and `src\finddlg1.cpp:1569` (`UpdateProgressBarTheme()` called immediately after `CreateWindowEx`).
