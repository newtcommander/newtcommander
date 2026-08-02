# Raw findings — Agent 6: Control-class coverage matrix & enabled-plugin adoption

> Unedited output of the control-class/plugin sweep exploration agent (feature 049 initial audit).

## A. Control-class → dark-theming matrix (main app)

Central engine: `src\themes.cpp`; per-child dispatcher = `ThemeApplyChildEnumProc` (themes.cpp:564-677).

| Control class | Themed? | How / evidence |
|---|---|---|
| Button (push/check) | YES | `SetWindowTheme(DarkMode_Explorer)` themes.cpp:582; `WM_CTLCOLORBTN` themes.cpp:704-712 |
| Button (radio/groupbox) | YES | stripped to classic `SetWindowTheme(L"",L"")` themes.cpp:579-580 so WM_CTLCOLORSTATIC color applies |
| Static | YES | themes.cpp:608-629; disabled-label repaint subclass themes.cpp:266-326; SS_ETCHED* subclass themes.cpp:333-368 |
| Edit | YES | `DarkMode_CFD`/`DarkMode_Explorer` themes.cpp:584-602; disabled-edit repaint themes.cpp:375-429; `WM_CTLCOLOREDIT` themes.cpp:714-721; command line editwnd.cpp:1710 |
| ComboBox / ComboBoxEx32 (incl. drop-down list) | YES | `DarkMode_CFD` themes.cpp:639-643; list portion via `WM_CTLCOLORLISTBOX` themes.cpp:715-721 |
| ListBox | YES | themes.cpp:603-607 + WM_CTLCOLORLISTBOX |
| ScrollBar (control + NC) | YES | themes.cpp:603-607; generic WS_V/HSCROLL windows themes.cpp:666-674; WM_CTLCOLORSCROLLBAR themes.cpp:723-727 |
| SysListView32 | YES | themes.cpp:644-655 (theme + `ListView_SetBkColor/TextColor`); Find results NM_CUSTOMDRAW finddlg1.cpp:4095-4221 |
| ↳ LVS_EX_CHECKBOXES state glyphs | **NO** | dialogs2.cpp:757, dialogs4.cpp:1317, dialogs4.cpp:3032, dialogs6.cpp:2591, packac.cpp:1550 use native (light) state imagelist. Dark glyphs exist (gui.cpp:3788 `CreateCheckboxImagelist`) but only wired at dialogs5.cpp:1674 and finddlg2.cpp:1068 |
| SysHeader32 (LV header) | YES | `DarkMode_ItemsView` themes.cpp:652-654, 662-665; Find header NM_CUSTOMDRAW finddlg1.cpp:1184-1239 |
| SysTreeView32 | YES | themes.cpp:656-661; sheets tree common\sheets.cpp:702 + `SheetsGetSysColorHook` salamdr1.cpp:1884 / sheets.cpp:942 |
| msctls_statusbar32 | YES (mostly) | full-repaint subclass themes.cpp:436-562, installed themes.cpp:630-638. **GAP:** packac.cpp:75 creates it inside its own WM_INITDIALOG (i.e. after theming ran) and never re-applies |
| Custom status bar (CStatusWindow) | YES | stswnd.cpp:796-1296 |
| ReBarWindow32 | PARTIAL | styles stripped `ThemeUpdateRebarStyle` themes.cpp:237-254; grippers via `RebuildRebarBands` mainwnd1.cpp:834; visual styles killed mainwnd3.cpp:1063. **No `RB_SETBKCOLOR`/`RBBIM_COLORS` anywhere** → exposed background strip stays classic BTNFACE |
| Toolbar | YES | no native ToolbarWindow32; custom CToolBar toolbar2.cpp:556-671, toolbar3.cpp:473-512; glyph recolor themes.cpp:737 + common\themes_palette.h:117 |
| Tooltip — custom CToolTip | YES | tooltip.cpp:644-646 (`COLOR_INFOBK/INFOTEXT`) |
| Tooltip — native tooltips_class32 | **NO** | mainwnd3.cpp:5100 (split-bar drag), viewer3.cpp:560 (viewer). No SetWindowTheme / TTM_SETTIPBKCOLOR / custom draw |
| SysTabControl32 | N/A core / **NO** plugins | core config is a hand-built tree dialog (common\sheets.cpp:1228 `CTreePropDialog::Execute`), no visible tabs. Plugin sheets use real tabs — see §B |
| msctls_progress32 | ONLY in Find | finddlg1.cpp:1527-1541 (`SetWindowTheme(L"",L"")` + PBM_SETBKCOLOR/PBM_SETBARCOLOR), refresh finddlg2.cpp:312. **No branch in `ThemeApplyChildEnumProc`** → every other native progress bar is light. Core's own bars are custom (gui.h:12) |
| msctls_trackbar32 | n/a | not used in core or any enabled plugin |
| msctls_updown32 | **NO** | no standalone use; the internal up-down of DTS_UPDOWN pickers is light |
| msctls_hotkey32 | **NO** | src\lang\lang.rc:1277 (IDD_PLUGINKEYS, "Plugin %s - Keyboard Shortcuts") |
| SysDateTimePick32 / SysMonthCal32 | **NO** | 10 controls: lang.rc:615-622 (IDD_ATTRIBUTES "Change Attributes"), lang.rc:1407-1411 (IDD_FINDADVANCED "Advanced Options"). Drop-down calendar is a separate popup, also light |
| RichEdit | n/a | not used (retired: plugins\mdview\render.h:7) |
| Property sheets / wizards | YES (core) | `ThemeSubclassPropSheetFrame` themes.cpp:822-870, wired dialogs2.cpp:343-344 |
| Menus — core owner-drawn | YES | menu3.cpp:89-94/431-432/601-602/733-734; menubar.cpp:175-232 |
| Menus — native TrackPopupMenu | **NO** (13 sites) | viewer3.cpp:2877, shellsup.cpp:101, execute.cpp:2037, gui.cpp:1351, dialogs3.cpp:192/234/556, dialogs4.cpp:1827/1893/3964, dialogs5.cpp:2972, mainwnd3.cpp:7009 |
| MessageBox — CMessageBox | YES | msgbox.cpp (6 theme refs), routed through CCommonDialog |
| MessageBox — raw ::MessageBox | **NO** | fileswn2.cpp:2768, packac.cpp:276, dialogs2.cpp:1105, regwork.cpp:128/175/258, common\allochan.cpp:111/122/138, common\winlib.cpp:1301/1352/1410/1476 (numeric-validation box, fires from plugin dialogs too) |
| OS common dialogs | NO (by design) | dialogs.cpp:1840, dialogs4.cpp:1848/1913/3999, dialogs5.cpp:747/2990, execute.cpp:2152, mainwnd3.cpp:2883, viewer3.cpp:886/1579, shellib.cpp:2543, salamdr6.cpp:1695-1752 |
| SysLink / SysAnimate32 / SysIPAddress32 | n/a | not used (CAnimate gui.cpp:2967 is defined but never instantiated) |

## Global mechanisms (do not false-positive these)

- **There is NO CBT/global dialog hook.** Only hooks present: menu1.cpp:156 (WH_CALLWNDPROC, menu tracking), salamdr3.cpp:2967 (WH_GETMESSAGE, wheel), plugins\pictview\dialogs.cpp:1090.
- Core auto-coverage #1: `CCommonDialog::NotifDlgJustCreated` → `ThemeApplyToDialog` (dialogs2.cpp:251-255) and `CCommonDialog::DialogProc` → `ThemeHandleCtlColor` (dialogs2.cpp:262). Every core dialog derived from CCommonDialog is covered without per-dialog code.
- Core auto-coverage #2: `CCommonPropSheetPage` dialogs2.cpp:339-356 (page + frame).
- Core auto-coverage #3: `SheetsGetSysColorHook = ThemeSysColor` salamdr1.cpp:1884 → all of common\sheets.cpp drawing.
- Plugin auto-coverage: `SetupWinLibTheme` (plugins\shared\winliblt.cpp:75-78) installs `ThemeApplyToDialog` at winliblt.cpp:528-529 (dialogs) / 793-794 (prop pages) and the `ThemeHandleCtlColor` fallback at winliblt.cpp:573-575 / 838-840.
- Plugin menus wrapped in `SalamanderGUI->CreateMenuPopup()/CreateMenuBar()` (plugins\shared\spl_gui.h:2068/2080) are rendered by the core's dark owner-draw path — ftp dialogs1.cpp:534/1382, dialogs7.cpp:501/646/1532/1643/1765, dialogs8.cpp:843/2019; pictview dialogs.cpp:383/1509, histwnd.cpp:565, pictview.cpp:2876/2880; dbviewer dbviewer.cpp:1147/1154, renmain.cpp:1413; filecomp controls.cpp:221, dialogs3.cpp:48; checksum dialogs.cpp:1110 — all already dark.
- Glyph auto-recolor: `ThemeAdjustBitmapForDarkMode` themes.cpp:737 + `ThemeDarkAdaptColor` common\themes_palette.h:117 (also svg.cpp).
- **Systemic ordering caveat:** `ThemeApplyToDialog` runs at the TOP of WM_INITDIALOG, *before* the dialog object's own handler — common\winlib.cpp:709-726 and plugins\shared\winliblt.cpp:522-529. Any control created inside a WM_INITDIALOG handler is never themed unless re-applied (Find does this at finddlg1.cpp:3109; packac.cpp does not).
- 036 API surface is only 6 methods (plugins\shared\spl_gen.h:3469-3503) — **`ThemeSubclassPropSheetFrame` is NOT exported to plugins**.

## B. Per-enabled-plugin adoption (plugins.cfg = 19 `on`)

| Plugin | Adoption | Unthemed surfaces (file:line) |
|---|---|---|
| 7zip | ✅ SetupWinLibTheme 7zip.cpp:170 | none found (7zthreads.cpp:19 subclasses the core progress dlg) |
| checksum | ✅ checksum.cpp:122 | 2× native `msctls_progress32` in checksum\lang\*.rc → light bars in dark dialogs |
| dbviewer | ✅ dbviewer.cpp:299; menus core-drawn | raw `TrackPopupMenuEx` dialogs.cpp:434 (font dropdown); LVS_EX_CHECKBOXES dialogs.cpp:651 |
| diskmap | ⚠️ only `ThemeApplyToTopLevel` DiskMap\GUI.MainWindow.h:120 | native menu bar `LoadMenu` GUI.MainWindow.h:108; **About box raw `DialogBox` GUI.AboutDialog.h:34** (no theme call) |
| filecomp | ✅ dlg_com.cpp:435, filecomp.cpp:732, mainwnd.cpp:100 | **CPropertyDialog config frame dialogs4.cpp:281 / dialogs.h:302**; native menu bar filecomp.cpp:724; raw menus viewwnd.cpp:473, dialogs2.cpp:220, dialogs4.cpp:159 |
| folders | ✅ folders.cpp:97 | raw `TrackPopupMenuEx` fs2.cpp:496 |
| ftp | ✅ fs1.cpp:62; top-level dialogs2.cpp:1041/1399 | **CConfigDlg property-sheet FRAME dialogs1.cpp:705 / dialogs.h:293**; raw menus fs1.cpp:401, fs1.cpp:449, fs3.cpp:961, dialogs1.cpp:761, dialogs3.cpp:550/1000/1445/1565/1640, dialogs8.cpp:276/1814; LVS_EX_CHECKBOXES dialogs3.cpp:1319 |
| mdview | ✅ viewer.cpp:103/207/415-444 + own dark menu darkmenu.cpp/.h | **FindDlgProc viewer.cpp:338 (raw `DialogBoxParamW` viewer.cpp:667) — no `ThemeApplyToDialog`/`ThemeHandleCtlColor`** |
| peviewer | ❌ **NONE** (precomp.h:35 includes winliblt.h, `SetupWinLibTheme` never called) | entire IDD_CONFIG dialog: cfgdlg.h:20, cfgdlg.cpp:24, lang\lang.rc:61 |
| pictview | ✅ pictview.cpp:1523/1704/1833; menus core-drawn | **CConfigDialog sheet FRAME dialogs.cpp:713 / dialogs.h:218**; `SaveAsDlgProc` saveas.cpp:441 (OFN hook + custom child template); WIA progress wiawrap.cpp:537 / DialogBoxParam wiawrap.cpp:666; `msctls_hotkey32` + `msctls_progress32` in pictview\lang\*.rc |
| portables | ✅ fx.cpp:335 | none found |
| regedt | ✅ dialogs.cpp:78; own `CDialogEx` still routes through `CDialog::CDialogProc` (dialogs.cpp:327/338/355/366) | none found |
| renamer | ✅ dialogs.cpp:57 | `ComDlgHookProc` dialogs.cpp:221 = OS common-dialog hook (light by design) |
| sftp | ✅ guard `SFTPThemeDlgMsg` dialogs.cpp:81-88 in **all 9** procs (dialogs.cpp:106/163/271/323/427/559/643/998, logs.cpp:215) | raw `TrackPopupMenuEx` fs.cpp:1169 (file context menu) |
| tar | n/a — no dialog resources at all | — |
| uncab | ✅ guard `CABThemeDlgMsg` dialogs.cpp:21-28 in all 4 procs (97/292/403/497) + themed custom static (45/53) | none found |
| undelete | ✅ undelete.cpp:254 | none found |
| uniso | ✅ uniso.cpp:206 | none found |
| zip | ✅ guard `ZIPThemeDlgMsg` dialogs.cpp:38-45 in **all 18** procs (dialogs.cpp:208/806/1067/1167/1278/1355/1520/1771/1862/1951/2046, dialogs2.cpp:45/1714/1988, dialogs3.cpp:96/287/383, prevsfx.cpp:106) | raw menus dialogs2.cpp:188/407/1565; `zip\selfextr\*` is a separate SFX stub exe (dialog.cpp:136/205, extract.cpp:1079, selfextr.cpp:673/698) — always light, out of process |

## C. Top 10 user-visible gaps (ranked)

1. **peviewer Configuration dialog — 100% light.** Zero adoption; single-line fix. `src\plugins\peviewer\peviewer.cpp` (no `SetupWinLibTheme`), `cfgdlg.cpp:24`.
2. **Core native popup menus on hot paths** — internal viewer right-click `viewer3.cpp:2877`; right-drag drop menu (Copy/Move/Shortcut here) `shellsup.cpp:101`. Full-white menus over dark windows.
3. **Plugin property-sheet FRAMES light around dark pages** (tab strip + OK/Cancel/Help + background): ftp `dialogs1.cpp:705`, pictview `dialogs.cpp:713`, filecomp `dialogs4.cpp:281`. Root cause: `plugins\shared\winliblt.cpp:853-884` + `ThemeSubclassPropSheetFrame` missing from `plugins\shared\spl_gen.h:3455-3503`.
4. **Date/time pickers — 10 white boxes in dark dialogs**: "Change Attributes" `src\lang\lang.rc:599,615-622`; Find > Advanced Options `src\lang\lang.rc:1378,1407-1411`. No `SysDateTimePick32` branch in `themes.cpp:666-674`.
5. **FS panel context menus in the two network plugins**: sftp `fs.cpp:1169`, ftp `fs1.cpp:401`, `fs1.cpp:449`, `fs3.cpp:961`.
6. **mdview Find dialog (Ctrl+F) light** inside an otherwise fully dark viewer: `mdview\viewer.cpp:338` + `viewer.cpp:667`.
7. **Native tooltips light-yellow over dark windows**: `viewer3.cpp:560`, `mainwnd3.cpp:5100`.
8. **Light checkbox glyphs in dark listviews**: Config>View modes `dialogs4.cpp:1317`, Config>Hot Paths `dialogs4.cpp:3032`, Config>Icon Overlays `dialogs6.cpp:2591`, Plugins>remove config `dialogs2.cpp:757`, Archivers auto-config `packac.cpp:1550`, ftp `dialogs3.cpp:1319`, dbviewer `dialogs.cpp:651`. Dark glyph generator already exists at `gui.cpp:3788`.
9. **Config-dialog dropdown menus (native)**: `execute.cpp:2037` (">>" variable insert, used by User Menu / viewers / editors), `dialogs3.cpp:192/234/556`, `dialogs4.cpp:1827/1893/3964`, `dialogs5.cpp:2972`, `gui.cpp:1351` (hyperlink copy).
10. **Ordering-bug leftovers + orphan classes**: `packac.cpp:75` status bar light (created after theming — `common\winlib.cpp:726`); `msctls_hotkey32` `lang.rc:1277`; native progress bars in checksum (2) and pictview (1); rebar background strip (no `RB_SETBKCOLOR` anywhere); diskmap About `GUI.AboutDialog.h:34`; native menu bars diskmap `GUI.MainWindow.h:108` / filecomp `filecomp.cpp:724`; raw `::MessageBox` at `common\winlib.cpp:1301/1352/1410/1476` (reachable from every winliblt plugin dialog).
