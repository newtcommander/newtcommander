# Raw findings — Agent 2: Dialog & popup audit (black edits, bright separators, unthemed dialogs)

> Unedited output of the dialog/popup exploration agent (feature 049 initial audit).

## (A) Central dialog-theming mechanism

| Layer | Location |
|---|---|
| WM_CTLCOLOR* engine | `src\themes.cpp:696-730` — `ThemeHandleCtlColor`: `WM_CTLCOLORDLG/STATIC/BTN` → text `COLOR_BTNTEXT`(240,240,240) + brush `COLOR_BTNFACE`(45,45,45); `WM_CTLCOLOREDIT/LISTBOX` → text `COLOR_WINDOWTEXT` + brush `COLOR_WINDOW`(**32,32,32**); `WM_CTLCOLORSCROLLBAR` → BTNFACE. Returns FALSE in Default theme. |
| Route for modal/modeless dialogs | `src\dialogs2.cpp:257-263` (`CCommonDialog::DialogProc`, calls `ThemeHandleCtlColor` before its own switch) |
| Route for config pages | `src\dialogs2.cpp:347-356` (`CCommonPropSheetPage::DialogProc`) |
| Per-child theming | `src\dialogs2.cpp:251-255` + `:339-345` (`NotifDlgJustCreated` → `ThemeApplyToDialog`, and `ThemeSubclassPropSheetFrame(GetParent())` for pages) → `themes.cpp:679-694` → `ThemeApplyChildEnumProc` `themes.cpp:564-677` |
| Property-sheet frame subclass | `themes.cpp:822-870` (`WM_CTLCOLOR*` + `WM_ERASEBKGND`) — this is what themes the Configuration holder dialog |
| Dispatch (winlib) | `src\common\winlib.cpp:704-772`; **note `NotifDlgJustCreated` fires at `:726`, i.e. BEFORE the derived class' `WM_INITDIALOG` body at `:767`** |
| Base `CDialog::DialogProc` | `winlib.cpp:635-701` — has **no** theme hook (unlike the plugin copy `plugins\shared\winliblt.cpp:571,836`) |
| Sheets library hook | `common\sheets.cpp:29-35` `SheetsGetSysColorHook`, set to `ThemeSysColor` at `salamdr1.cpp:1884` |
| Existing dark subclasses | static-disabled `themes.cpp:266-326`; **etched separator** `themes.cpp:333-368` (feature 044); disabled-edit flat repaint `themes.cpp:375-429` (**single-line only**, bails on `ES_MULTILINE` at `:387-388`); status bar `themes.cpp:436-562` |

**Procs that do NOT route through it (main app):**
- `CSplashScreen : CDialog` — `src\dialogs.h:815`, `src\logo.cpp:105` (self-painted brand navy; intentional).
- `CTreePropHolderDlg : CDialog` — `src\common\sheets.h:175`, proc `sheets.cpp:677`; only themed indirectly when a page calls `ThemeSubclassPropSheetFrame` (`dialogs2.cpp:344`). It calls `SetWindowTheme(HTreeView, L"explorer")` at `sheets.cpp:702` (light) and relies on the later pass to overwrite it.
- System common dialogs (never themed): `ChooseFont` `dialogs4.cpp:1848`, `dialogs5.cpp:2990`; `ChooseColor` `dialogs4.cpp:1913`, `dialogs4.cpp:3999`; `SafeGetOpenFileName/SaveFileName` `dialogs.cpp:1840`, `dialogs5.cpp:747`, `mainwnd3.cpp:2883`, `viewer3.cpp:886`, `viewer3.cpp:1579`, `execute.cpp:2152`; folder picker via `GetTargetDirectory` `execute.cpp:1709`, `dialogs2.cpp:1258`, `dialogs5.cpp:1881`, `fileswn0.cpp:1463`.
- Raw system `MessageBox` (bypasses the themed `CMessageBox`, `dialogs.h:122`): `fileswn2.cpp:2768`, `packac.cpp:276`, `dialogs2.cpp:1105`, `regwork.cpp:128/175/258`, `salamdr1.cpp:3879/3992/4057/4066/4674`.
- No raw `DialogBoxParam`/`CreateDialogParam` with standalone procs exists in the main app — the only call sites are `winlib.cpp:612-631` and `sheets.cpp:1155`.

---

## (B) Ranked defect list

**1. Group boxes render a bright etched frame (bright-separator) — app-wide, 26 controls / 15 dialogs**
`themes.cpp:578-580` strips the visual style from `BS_GROUPBOX` (and radio buttons) via `SetWindowTheme(hChild, L"", L"")` to get dark label text. Classic group-box drawing paints its `EDGE_ETCHED` rectangle with real `GetSysColor(COLOR_3DHILIGHT/COLOR_3DSHADOW)` and never consults `WM_CTLCOLORSTATIC` — the exact root cause 044 documented for `SS_ETCHED*` (`specs\044-fix-find-dark-mode\research.md:21`) but fixed only for statics (`themes.cpp:611-619`). No group-box branch exists anywhere (`BS_GROUPBOX` occurs only at `themes.cpp:579` and `gui.cpp:3466`). Find has no group boxes, which is why 044 missed it.
Controls: `lang.rc:138,141` (IDD_CFGPAGE_REGIONAL), `156,161,164` (IDD_CFGPAGE_SYSTEM), `188,198` (IDD_CFGPAGE_VIEWS), `899` (IDD_CFGPAGE_VIEWERS), `1050,1057` (IDD_CFGPAGE_PACKERS), `1082` (IDD_CFGPAGE_UNPACKERS), `1106` (ARCHIVERSLOCATIONS), `1121` (ASSOCIATIONS), `1158,1161,1164` (IDD_CFGPAGE_APPEARANCE), `1190` (IDD_FILELIST), `1516` (IDD_SLGSELECTORPLUG), `1539` (IDD_SLGSELECTOR), `1621,1624` (IDD_LOADSELECTION), `1641` (IDD_SAVESELECTION), `1861,1866` (IDD_CFGPAGE_CHANGEDRIVE), `2002` (IDD_CHANGE_MASTERPWD), `2036` (IDD_ENTER_MASTERPWD).

**2. Radio buttons drawn classic (bright glyph) — same code path**
`themes.cpp:578-580` also strips the theme from `BS_RADIOBUTTON/BS_AUTORADIOBUTTON`. The classic glyph is drawn by user32 with real system colors (white interior), unlike checkboxes which keep `DarkMode_Explorer` (`themes.cpp:582`). Affects every radio group in config pages, Change Case (`lang.rc:707-732`), Change Coding, Load/Save Selection, Compare Directories.

**3. `SysDateTimePick32` fully unthemed (10 controls, 2 dialogs) — fully-unthemed / bright field**
`ThemeApplyChildEnumProc` (`themes.cpp:564-677`) has branches only for Button/Edit/ListBox/ScrollBar/Static/statusbar/ComboBox/ListView/TreeView/Header; date-time pickers fall into the generic `else` (`themes.cpp:666-673`) which only touches windows with `WS_*SCROLL`. No `SetWindowTheme(...DarkMode_CFD)` anywhere for them (`grep SysDateTimePick32` in `*.cpp` → 0 hits).
- Change Attributes `CChangeAttrDialog` (`dialogs.h:166`, ctor `dialogs.cpp:34`, IDD_ATTRIBUTES): `IDC_ATTR_MODIFIED_DATE/TIME`, `IDC_ATTR_CREATED_DATE/TIME`, `IDC_ATTR_ACCESSED_DATE/TIME` — `lang.rc:615-622`.
- Find Advanced / filter criteria `CFilterCriteriaDialog` (`filter.cpp:828`, IDD_FINDADVANCED): `IDC_FFA_FROM_DATE/TIME`, `IDC_FFA_TO_DATE/TIME` — `lang.rc:1407-1411`.

**4. `msctls_hotkey32` fully unthemed — bright field**
`IDC_PLUGINKEY` in IDD_PLUGINKEYS (`lang.rc:1277`), dialog `CPluginKeys` (`dialogs5.cpp:964`). Same missing-branch cause as #3.

**5. `CToolbarHeader::OnPaint` draws its caption with no `SetTextColor` → black text on dark face (unreadable)**
`gui.cpp:2860-2877` (`ThemeDrawEdge` + `DrawText` at `:2875`, background filled `ThemeSysColorBrush(COLOR_3DFACE)` at `:2954`). Explicitly deferred as out-of-scope by 044 (`research.md:210-213`) because Find does not use it. Live instances: `dialogs4.cpp:1303,1309` (IDD_CFGPAGE_VIEWS headers), `dialogs4.cpp:3026` (IDD_CFGPAGE_HOTPATH), `dialogs5.cpp:511` and `dialogs5.cpp:1249` (IDD_PLUGINS / plugin pages), `dialogs6.cpp:2589` (IDD_CFGPAGE_ICONOVRLS), and **`edtlbwnd.cpp:271`** — the `CEditListBox` header used by User Menu, Viewers, Editors, Packers/Unpackers, Archivers, Colors, Auto-config drives pages.

**6. Change Icon dialog: owner-drawn list uses raw system brushes → white list background**
`CChangeIconDialog` (`dialogs.h:932`, ctor `dialogs3.cpp:2229`, IDD_CHANGEICON, `IDL_CHI_LIST` `lang.rc:1226`, `LBS_OWNERDRAWFIXED`). `dialogs3.cpp:2392-2393`: `FillRect(lpdis->hDC, &r, (HBRUSH)(DWORD_PTR)(bkColor + 1))` with `bkColor = COLOR_HIGHLIGHT|COLOR_WINDOW` — real system brush instead of `ThemeSysColorBrush`. Every other owner-draw in the app was converted (`toolbar3.cpp:472-475`, `edtlbwnd.cpp:600-651`, `dialogs5.cpp:3193`, `dialogs4.cpp:3108`, `dialogs3.cpp:2688-2760`).

**7. Auto-Configuration dialog: status bar created after the theming pass → fully light status bar**
`CPackACDialog : CCommonDialog` (`pack.h:198`), `packac.cpp:75-78` creates `STATUSCLASSNAME` inside `WM_INITDIALOG`, i.e. after `NotifDlgJustCreated` already ran (`winlib.cpp:726` vs `:767`), so the 044 status-bar subclass (`themes.cpp:630-638`) is never installed. `packac.cpp` has no second `ThemeApplyToDialog` (only `ThemeSysColor` at `:330`). This is exactly defect 5 of 044 (`research.md:25`), which assumed this instance would be "silently healed" — it is not. The fix pattern used for Find is `finddlg1.cpp:3106-3109`.

**8. Read-only edits get the dialog-face brush while editable edits get near-black → black-vs-gray field inconsistency (black-edit class)**
Windows routes read-only/disabled edits through `WM_CTLCOLORSTATIC`, which `themes.cpp:704-712` answers with `COLOR_BTNFACE` (45,45,45), whereas normal edits get `COLOR_WINDOW` (32,32,32) at `themes.cpp:714-721`. In the same dialog, some fields are a black hole and some are face-colored, and bordered read-only edits look like empty sunken face-colored boxes. Concrete controls:
- `IDE_LANGUAGE` `lang.rc:140` (IDD_CFGPAGE_REGIONAL, `dialogs4.cpp:858`) — bordered.
- `IDE_VIEWERFONT` `lang.rc:240` (IDD_CFGPAGE_VIEWER, `dialogs4.cpp:1627`) — bordered.
- `IDE_PANELFONT` `lang.rc:1160` (IDD_CFGPAGE_APPEARANCE, `dialogs5.cpp:2792`) — bordered.
- `IDC_CM_ADVANCED_INFO` `lang.rc:366` (IDD_COPYMOVEMOREDIALOG, `dialogs3.cpp:587`) — bordered.
- `IDC_FIND_ADVANCED_TEXT` `lang.rc:1332` (IDD_FIND) — bordered; also `EnableWindow(FALSE)` at `finddlg1.cpp:1739` (covered by the 044 single-line subclass).
- `IDC_PLUGINTHUMBNAILS` `lang.rc:1259` (IDD_PLUGINS, `dialogs5.cpp:27`).
- Borderless value-label edits: `IDS_DIRSCOUNT/FILESCOUNT/SIZE/OCCUPIED/DISKUTILIZATION/COMPSIZE/COMPRATIO/IDC_EST_SIZE/IDC_EST_UTIL` `lang.rc:381-404` (IDD_SIZERESULTS, `CSizeResultsDlg` `dialogs2.cpp:365`); `IDT_MOUNTPOINT…IDT_FILESYSTEMFLAGS` `lang.rc:665-700` (IDD_DRIVEINFO, `CDriveInfo` `dialogs3.cpp:1254`); `IDE_ALTSTREAMS` `lang.rc:1790` (IDD_CONFIRMADSLOSS, `dialogs6.cpp:1979`); `IDS_DETAILS` `lang.rc:2106` (IDD_CONFIRMLINKTGTCOPY, `dialogs6.cpp:2038`).

**9. Disabled MULTILINE edits are not covered by the flat-repaint subclass**
`themes.cpp:386-388` explicitly bails on `ES_MULTILINE`, so a disabled multiline edit still paints its text with the light visual style's disabled gray. Candidates: `IDT_DRIVETYPE` `lang.rc:671`, `IDT_FILESYSTEMFLAGS` `lang.rc:700` (IDD_DRIVEINFO), `IDC_SLG_AUTHOR` `lang.rc:1518` and `lang.rc:1541` (IDD_SLGSELECTORPLUG / IDD_SLGSELECTOR, `CLanguageSelectorDialog` `dialogs2.cpp:783`).

**10. Palette inversion: input surfaces darker than the dialog face (black-edit class, systemic)**
`common\themes_palette.h:24` `COLOR_WINDOW = 32,32,32` vs `:32` `COLOR_BTNFACE = 45,45,45`. Every edit/combo/listbox/listview reads as a black hole on a lighter dialog (opposite of the Win11 dark convention). Visually confirmed in `temp\dark_after_find_expanded.png` (combo interiors #202020 on a #2D2D2D face).

**11. Config dialog (property-sheet holder) theming is second-hand**
`CTreePropHolderDlg` (`sheets.h:175`) is a plain `CDialog`; it becomes dark only because each page calls `ThemeSubclassPropSheetFrame` (`dialogs2.cpp:344` → `themes.cpp:862-870`). Its own `WM_INITDIALOG` sets the light `explorer` tree theme (`sheets.cpp:702`), and its grip is a `scrollbar`-class `SBS_SIZEBOX` child (`sheets.cpp:1393-1395`) that only gets `DarkMode_Explorer`. Its separator `_TPD_IDC_SEP` (`sheets.cpp:1379-1381`, `SS_ETCHEDHORZ`) is covered by the 044 subclass.

**12. `WM_SYSCOLORCHANGE` resets a themed listview to system colors**
`dialogs4.cpp:1360-1364`: `ListView_SetBkColor(HListView, GetSysColor(COLOR_WINDOW))` (light) in IDD_CFGPAGE_VIEWS, unlike every other site which uses `ThemeSysColor` (`dialogs5.cpp:918,1270`, `dialogs6.cpp:720,1394,2615`, `dialogs3.cpp:2941`, `dialogs2.cpp:1154`, `packac.cpp:330`).

**13. Unthemed system dialogs invoked from themed parents (fully-unthemed)**
Font/color pickers, file/folder pickers and raw `MessageBox` listed in section (A). They appear as white windows launched from dark dialogs (Colors page `dialogs4.cpp:1913,3999`, Appearance/Viewer font `dialogs4.cpp:1848`, `dialogs5.cpp:2990`). (Consolidator note: OS-owned common dialogs are a documented 028 boundary; raw `MessageBox` call sites are in-app and fixable by routing through `CMessageBox`.)

**14. Latent: shared winlib window class background is the light system brush**
`common\winlib.cpp:487` and `:510`: `CWindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1)`. Only the main window and panels get `ThemeUpdateWindowClassBackground` (`mainwnd3.cpp:1259`, `mainwnd1.cpp:3279`); any `CWindow::Create`d child inside a dialog erases white. Also `salamdr1.cpp:4325,4334` register `SAVEBITS_CLASSNAME`/`SHELLEXECUTE_CLASSNAME` with `(HBRUSH)(COLOR_3DFACE + 1)`.

**15. Minor: hardcoded black pen in `CColorGraph`**
`gui.cpp:1574` `CreatePen(PS_SOLID, 0, RGB(0,0,0))` — pie outline in IDD_DRIVEINFO disappears/reads as a black smear on dark. Also `gui.cpp:1146` `SetTextColor(hDC, RGB(0,0,255))` for `STF_HYPERLINK_COLOR` statics (low contrast on dark).

**Verified NOT defective (do not re-fix):** `SS_ETCHED*` statics (63 in `lang.rc`) — subclassed since 044 (`themes.cpp:333-368`, `:611-619`); `SS_GRAYFRAME` spacers `lang.rc:367,1330,1688` are `NOT WS_VISIBLE`; no `WS_EX_CLIENTEDGE/STATICEDGE` in `lang.rc` at all; the only runtime `WS_EX_STATICEDGE` (`finddlg2.cpp:342`) has a `WM_NCPAINT` override (`finddlg2.cpp:584-600`); `CEditListBox::OnDrawItem` (`edtlbwnd.cpp:600-651`) and `toolbar3.cpp:461-476` use theme brushes; `sheets.cpp` caption gradient is hooked to `ThemeSysColor`.

---

## (C) Per-dialog dark hacks already in place (patterns to copy)

| Pattern | Reference |
|---|---|
| Re-apply `ThemeApplyToDialog` at the **end** of `WM_INITDIALOG` for controls created there (idempotent via `THEME_DARKENED_PROP`) | `finddlg1.cpp:3106-3109` |
| Re-apply on live theme switch in the dialog's colors-changed handler | `finddlg2.cpp:310` |
| `WM_NCPAINT` override drawing the frame with `ThemeDrawEdge` | `finddlg2.cpp:584-600`; precedent `filesbx1.cpp:1342-1360` |
| Central repaint subclass installed from `ThemeApplyChildEnumProc` (disabled statics / etched lines / disabled edits / status bar) | `themes.cpp:266-326`, `:333-368`, `:375-429`, `:436-562`, installed at `:598-638` |
| Per-control class swap for dark (`DarkMode_CFD` for edit-likes) | `themes.cpp:591-595`, `:642`; command line precedent `editwnd.cpp:1709-1710` |
| Dialog-local `WM_CTLCOLOR*` override that wins over the central handler (returns before calling the base) | `msgbox.cpp:1118-1160` (custom Vista msgbox), `logo.cpp:515-546` (About, brand navy, feature 037/040), `mainwnd3.cpp:1240-1249` (command line) |
| Classic-control theme strip + `WM_CTLCOLORSTATIC` text color | `themes.cpp:578-582` |
| Non-themable comctl (progress) recolored at creation | `finddlg1.cpp:1563+` |
| Library-level color hook instead of forking the lib | `sheets.cpp:29-35` + `salamdr1.cpp:1884` |

---

## (D) Coverage

- Dialog classes found in the main app (`src\*`, excluding `plugins\`, `translator\`, `tserver\`, `setup\`, `salmon\`): **96** — 66 `: public CCommonDialog` (`dialogs.h` 50, `find.h` 6, `pwdmngr.h` 3, `pack.h` 2, `viewer.h` 2, `filter.h` 1, `zip.h` 1, `toolbar.h` 1), 28 `: public CCommonPropSheetPage` (all in `cfgdlg.h`), 2 raw `: public CDialog` (`dialogs.h:815`, `common\sheets.h:175`).
- Routed through the central `ThemeHandleCtlColor` + `ThemeApplyToDialog`: **94 / 96 (98%)**; bypassing: **2** (both examined, item 11 and `CSplashScreen`).
- Dialog templates inspected in resources: **107** in `src\lang\lang.rc` + **2** in `src\salamand.rc` (IDD_MSGBOX, IDD_SPLASH) + 1 generated template (`sheets.cpp:1330-1399`) = **110**.
- Standalone `DialogBoxParam`/`CreateDialogParam` procs in the main app: **0** (all creation goes through `winlib.cpp:612-631` / `sheets.cpp:1155`); 12 dialog procs exist in `src\setup\instwiz.c` and 1 in `src\sfx7zip\install.c` but those are separate binaries, out of scope (auxiliary executables were excluded by 028).
