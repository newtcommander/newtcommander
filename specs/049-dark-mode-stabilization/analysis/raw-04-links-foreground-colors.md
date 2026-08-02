# Raw findings — Agent 4: Hyperlinks & hardcoded foreground colors

> Unedited output of the links/text-colors exploration agent (feature 049 initial audit).
> All paths relative to repo root `E:\Projects\tandemcommander`.

## (A) About-dialog link mechanism + exact color source

**Mechanism: custom owner-drawn static, not SysLink.**

- Class `CHyperLink : public CStaticText` — declared `src\gui.h:148-172`, implemented `src\gui.cpp:1275-1420`.
- Default ctor flags (`src\gui.h:154`): `DWORD flags = STF_UNDERLINE | STF_HYPERLINK_COLOR`.
- About creates it at **`src\logo.cpp:494`**:
  `hl = new CHyperLink(HWindow, IDC_ABOUT_WWW);` (no flags → gets `STF_HYPERLINK_COLOR`), URL set at `src\logo.cpp:497-499` (`https://tandemcommander.org`).
- Control is a plain `LTEXT` in the template: `src\lang\lang.rc:646` (`LTEXT "tandemcommander.org",IDC_ABOUT_WWW,...,WS_TABSTOP`).

**Color source — hardcoded, and it deliberately overrides the theme:**

`src\gui.cpp:1142-1149` (inside `CStaticText::WindowProc`, `WM_PAINT`):
```c
HWND hParent = GetParent(HWindow);
if (hParent != NULL)
    SendMessage(hParent, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)HWindow);
if (Flags & STF_HYPERLINK_COLOR)
    SetTextColor(hDC, RGB(0, 0, 255));          // <-- src\gui.cpp:1146  THE DEFECT
BOOL enabled = IsWindowEnabled(HWindow);
if (!enabled)
    SetTextColor(hDC, ThemeSysColor(COLOR_GRAYTEXT));
```
The parent's `WM_CTLCOLORSTATIC` reply (which *is* theme-correct) is fetched first and then discarded by line 1146. Note line 1149 already calls `ThemeSysColor`, so `themes.h` is in scope in this exact function — no new include needed for a fix. Dead commented-out alternative sits at `src\gui.cpp:1151-1157` (`textClr = RGB(0,0,255)` / else `ThemeSysColor(COLOR_BTNTEXT)`).

**Background it lands on in dark mode:** the About dialog paints its own bitmap background and answers `WM_CTLCOLORSTATIC` itself:
- `src\logo.cpp:447` — `SetBkColor(hDC, dark ? TC_COLOR_NAVY : RGB(255,255,255))`
- `src\logo.cpp:515-533` — `WM_CTLCOLORSTATIC`: text `TC_COLOR_TEXT_DARKBG`, bk `TC_COLOR_NAVY`, returns `NULL_BRUSH`
- `TC_COLOR_NAVY = RGB(0x0A,0x14,0x24)` at `src\logo.cpp:26`

So the rendered pair is `#0000FF` on `#0A1424` ≈ **2.1:1** contrast (WCAG floor for the project is 4.5:1 per `src\common\themes_palette.h:12-13`). In the Default theme the same link sits on white and is fine — which is why it only reads as a dark-mode bug.

No `WC_LINK` / `SysLink` involvement (see D/4 below).

---

## (B) Every place this mechanism is used

### B1 — Blue-colored links (`STF_HYPERLINK_COLOR` set → hit `gui.cpp:1146`). All broken in dark mode.

| # | File:line | Site | Dark-mode background it draws on |
|---|---|---|---|
| 1 | `src\logo.cpp:494` | **About dialog**, `IDC_ABOUT_WWW` (reported defect) | `TC_COLOR_NAVY` `#0A1424` (`src\logo.cpp:447/531`) |
| 2 | `src\msgbox.cpp:469` | `CMessageBox` URL line `IDS_MSGBOX_URL` — all error/info/question boxes created with a URL | `ThemeSysColor(COLOR_WINDOW)` = `RGB(32,32,32)` (`src\msgbox.cpp:1142-1156`) |
| 3 | `src\dialogs5.cpp:502` | **Plugins Manager** (`CPluginsDlg`), plugin homepage `IDC_PLUGINWWW` | `COLOR_BTNFACE` = `RGB(45,45,45)` |
| 4 | `src\dialogs5.cpp:3569` | **Configuration → Keyboard** (`CCfgPageKeyboard`), `IDC_KEYBOARD_SHORTCUTS` | `COLOR_BTNFACE` |
| 5 | `src\dialogs4.cpp:919` | **Configuration → Regional** (`CCfgPageRegional`), incomplete-SLG URL `IDC_CFGREG_INCOMPLETE_URL` | `COLOR_BTNFACE` |
| 6 | `src\dialogs2.cpp:1059` | **Language Selection** dialog, `IDC_SLG_WEB` — explicit `STF_HYPERLINK_COLOR` (no underline) | `COLOR_BTNFACE` |
| 7 | `src\dialogs.cpp:2070` | `CBetaExpiredDialog`, `IDC_BETAEXPIREDURL` | dead code — `USE_BETA_EXPIRATION_DATE` is commented out at `src\consts.h:10` |

Same drawing path is exported to plugins via `CHyperLinkForPlugin` (`src\plugins3.cpp:220-234`, iface `src\plugins.h:1700-1725`, `AttachHyperLink` decl `src\plugins\shared\spl_gui.h:2012`). Plugin sites that pass `STF_HYPERLINK_COLOR` and therefore show the identical bug:
- `src\plugins\pictview\dialogs.cpp:130` and `:134` (About: email + www)
- `src\plugins\mmviewer\dialogs.cpp:81` and `:85` (About: email + www)
- `src\plugins\ftp\dialogs1.cpp:1309` (save-password hint)
- `src\plugins\ftp\dialogs8.cpp:1957` (proxy save-password hint)
- `src\plugins\demoplug\dialogs.cpp:386`, `:391`

### B2 — Same class, **not** affected (`STF_DOTUNDERLINE` only ⇒ no `STF_HYPERLINK_COLOR` ⇒ inherits the parent's themed `WM_CTLCOLORSTATIC` color; the dotted underline pen is derived from `GetTextColor(hDC)` at `src\gui.cpp:1208`)

`src\dialogs.cpp:1979` · `src\dialogs2.cpp:605` · `src\dialogs2.cpp:1238` · `src\dialogs3.cpp:174` · `:345` · `:967` · `:1223` · `:2165` · `src\dialogs4.cpp:1794` · `:3355` · `src\dialogs5.cpp:3172` · `:3175` · `src\dialogs6.cpp:1658` · `src\dialogsp.cpp:719` · `:1271` · `src\msgbox.cpp:512`
Plugin equivalents: `src\plugins\ftp\dialogs1.cpp:516`, `src\plugins\ftp\dialogs6.cpp:88`, `src\plugins\checksum\dialogs.cpp:1263`, `src\plugins\demoplug\dialogs.cpp:396`.

The generic themed color for these comes from `ThemeHandleCtlColor` (`src\themes.cpp:696-729`) routed through `CCommonDialog::DialogProc` (`src\dialogs2.cpp:262`) and `CCommonPropSheetPage::DialogProc` (`src\dialogs2.cpp:352`).

---

## (C) Other hardcoded foregrounds over theme-dependent backgrounds — ranked

**1. `src\gui.cpp:1146` — `SetTextColor(hDC, RGB(0,0,255))`.** Rank 1; single root cause for all 6 live sites in B1 plus 8 plugin sites. Impact: pure blue on `#0A1424` (About) / `#202020` (msgbox) / `#2D2D2D` (config, plugin manager, language selector). ~1.9–2.1:1.

**2. `src\dialogs4.cpp:3682, 3685, 3688, 3691, 3694` — `Masks[i]->SetColor(RGB(255,255,255), RGB(255,255,255))`** in `CCfgPageColors::EnableControls()`. When no highlight-mask item is selected, five `CColorArrowButton` swatches are forced to white-on-white → five **glaring pure-white blocks** on the dark `COLOR_BTNFACE` config page. Related defaults `TextColor = RGB(0,0,0)` / `BkgndColor = RGB(255,255,255)` at `src\gui.cpp:2623-2624` (painted at `src\gui.cpp:2671`), which is what any `CColorArrowButton` shows before its first `SetColor`.

**3. `src\gui.cpp:1574` — `HPEN hBlackPen = CreatePen(PS_SOLID, 0, RGB(0,0,0))`** in `CColorGraph::PaintFace` (drive-info pie/free-space gauge, drawn at `src\gui.cpp:1582-1612`). Consumer: `CDriveInfo` (`src\dialogs3.cpp:1736-1738`, `IDB_GRAPH`). Black outline of every ellipse/pie/chord on the dark `COLOR_BTNFACE` (45,45,45) dialog → the gauge's silhouette disappears.

**4. `src\dialogs3.cpp:1715-1725` — `CDriveInfo` free/used space colors.** Truecolor branch `RGB(35,245,156)/RGB(9,159,96)/RGB(74,163,234)/RGB(18,95,156)`; the ≤256-color branch is `RGB(0,255,0)/RGB(0,128,0)/**RGB(0,0,255)**/RGB(0,0,128)` — line **1724 is the same pure blue** as the About link. Feeds both the legend swatches `CColorRectangle` (`IDB_FREESPACE`/`IDB_USEDSPACE`, painted `src\gui.cpp:1481-1482`) and the pie. None theme-derived.

**5. `src\toolbar3.cpp:522` — `SetTextColor(hDC, RGB(0,0,0)); DrawFocusRect(hDC, &r);`** in the Customize-Toolbar dialog's owner-drawn listboxes (`WM_DRAWITEM`, `src\toolbar3.cpp:461+`). `DrawFocusRect` derives its dot pattern from the DC's text/bk colors; every other focus-rect site in the codebase pairs `ThemeSysColor(COLOR_BTNFACE)`/`ThemeSysColor(COLOR_BTNTEXT)` — see `src\gui.cpp:941-942` and `src\gui.cpp:2243-2244`. Inconsistent focus visibility on dark.

**6. `src\toolbar2.cpp:825` — `CreatePen(PS_SOLID, 0, RGB(0,0,0))`** for the toolbar drag insert-mark (`src\toolbar2.cpp:826-836`), drawn straight on the dark toolbar (`COLOR_BTNFACE` 45,45,45) → near-invisible black drop marker while customizing toolbars.

**7. `src\filesmap.cpp:679-680` — `SetTextColor(hdc, RGB(255,255,255)); SetBkColor(hdc, RGB(0,0,0));`** before the rubber-band-selection `DrawFocusRect` over the panel (`ITEM_BK_NORMAL` = `RGB(32,32,32)` dark). Same class of inconsistency as #5; lower impact because white/black still XORs visibly.

**8. `src\dialogs4.cpp:1362-1363` — `ListView_SetBkColor(HListView, GetSysColor(COLOR_WINDOW))`** in `CCfgPageView::DialogProc` `WM_SYSCOLORCHANGE`. Raw `GetSysColor`, not `ThemeSysColor` → on any system color-change while the Dark theme is active, both `IDC_VIEW_LIST`/`IDC_VIEW_LIST2` listviews snap back to the **light** window background inside a dark page. (Latent; fires only on `WM_SYSCOLORCHANGE`.)

**9. `src\finddlg1.cpp:4158` — `if (GetSysColor(COLOR_3DFACE) != GetSysColor(COLOR_WINDOW))`.** Logic-only: the full-row-select background *decision* is made from light-mode system values while the actual paint two lines later correctly uses `ThemeSysColor` (`:4170`, `:4179`). Can pick the wrong branch under dark + high-contrast interplay.

**10. `src\logo.cpp:524-528`** — the `IDC_STATIC_6/7/8` muted-text case in `CAboutDialog`'s `WM_CTLCOLORSTATIC` is dead for `IDD_ABOUT` (that template has only `IDC_STATIC_1..5`, `src\lang\lang.rc:639-653`). Harmless, but it means "version/tagline muted color" is currently unreachable in About.

### Explicitly NOT offenders (verified — avoid false positives during the fix)
Monochrome-mask generation, where black/white are the mask's semantics, not visible foreground: `src\fileswn4.cpp:672, 1552, 1959` · `src\fileswn9.cpp:1730, 1811` · `src\menu3.cpp:634, 641-642, 666-671, 797-798, 1096-1103, 1192-1193, 1270-1271` · `src\toolbar4.cpp:878, 897, 902` · `src\iconlist.cpp:634, 682` · `src\common\dib.cpp:352-353` · `src\salamdr1.cpp:728-731` (`CustomColors` user palette seed).
Already themed and correct: status bar / panel captions (`src\stswnd.cpp:786-800, 1131-1294, 2400`), menu bar (`src\menubar.cpp:215, 231`), tooltips (`src\tooltip.cpp:646-647`), Tip of the Day (`src\dialogs6.cpp:90, 97`), Find results + header + status (`src\finddlg1.cpp:1223, 3931, 4170, 4179`), viewer (`src\viewer.cpp:812-1431`), splash screen (`src\logo.cpp:190-270`, brand-dark by design).

---

## (D) Palette "link color" slot status

**No dedicated link slot — but a suitable, already-contrast-validated candidate exists and is currently unused by drawing code.**

- `src\common\themes_palette.h` contains no `LINK`/`HYPERLINK` entry anywhere (neither in `THEME_DARK_SYSCOLORS`, `THEME_DARK_PANEL_COLORS`, nor `THEME_DARK_VIEWER_COLORS`).
- The nearest semantic match is **`ENTRY(COLOR_HOTLIGHT, 102, 178, 255)` at `src\common\themes_palette.h:42`** — Windows' own "hyperlink/hot item" system index, dark value `#66B2FF`.
- It is already asserted for contrast in the test suite: `src\saltests\saltests.cpp:646-647` — `>= 4.5:1` against both `COLOR_WINDOW` and `COLOR_BTNFACE`, and `COLOR_HOTLIGHT` is in the "must be mapped" list at `:632`.
- **It has zero readers in drawing code**: grep for `COLOR_HOTLIGHT` yields only the palette entry, the two test lines, and `src\salamdr1.cpp:1737` `COLORREF hotColor = GetSysColor(COLOR_HOTLIGHT);` — a raw (unthemed) read used to seed the light-theme `HOT_PANEL` default. There is no `ThemeSysColor(COLOR_HOTLIGHT)` call site anywhere in the repo.
- Panel palette also carries `ENTRY(HOT_PANEL, 102, 178, 255)` at `src\common\themes_palette.h:79` (same value), plus `HOT_ACTIVE` `(180,210,255)` `:80` and `HOT_INACTIVE` `(160,190,230)` `:81`.
- Not covered by any palette entry: the About surface `TC_COLOR_NAVY` `#0A1424` (`src\logo.cpp:26`) — it is a brand constant local to `logo.cpp`, so `COLOR_HOTLIGHT`'s 4.5:1 guarantee is proven against `COLOR_WINDOW`/`COLOR_BTNFACE` only, *not* against navy. (`#66B2FF` on `#0A1424` computes to roughly 8:1, so it clears the bar there too, but no test asserts it today.)

### SysLink / `WC_LINK`
**Zero occurrences.** Grep for `WC_LINK`, `SysLink`, `LWS_`, `LM_SETITEM`, `LITEM` across the whole repo returns no control usage — the only `NM_CLICK` hit (`src\dialogs5.cpp:1779`) is a ListView notification, and the `LVN_DELETEALLITEMS`/`LB_GETSELITEMS` hits are unrelated substring matches. So no `LM_SETITEM`/custom-draw/`WM_CTLCOLORSTATIC`+`LWS_*` work is needed; the entire link surface of the app funnels through `CStaticText::WindowProc` at `src\gui.cpp:1146`.
