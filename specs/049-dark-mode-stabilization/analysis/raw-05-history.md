# Raw findings — Agent 5: Dark-mode history, known issues, binding contracts

> Unedited output of the history/contracts exploration agent (feature 049 initial audit).

# (A) Per-feature summary — mechanism, API names, UI surface

## 028-visual-themes (`specs/028-visual-themes/`) — the theme engine
**Mechanism added** (`src/themes.h` / `src/themes.cpp`, contract `contracts/theme-engine.md`):
- `THEME_MODE_DEFAULT=0 / THEME_MODE_DARK=1`; `Configuration.ThemeMode` DWORD persisted as `CONFIG_THEMEMODE_REG` ("Theme Mode") under `SALAMANDER_CONFIG_REG`, early-read before any window exists (D10, no light flash).
- `IsDarkThemeActive()`, `RefreshThemeHighContrastState()`, `ThemeSysColor(int)`, `ThemeSysColorBrush(int)`, `ThemeDrawEdge()`, `UpdateCurrentColorsForTheme()`, `SetThemeMode(DWORD)`, `ThemeApplyToTopLevel(HWND)`, `ThemeApplyToDialog(HWND)`, `ThemeHandleCtlColor(...)`, `ReleaseThemeGraphics()`.
- Later additions visible in current `src/themes.h` (from the two in-feature fix commits): `ThemeUpdateWindowClassBackground(HWND,int lightSysColor)`, `ThemeUpdateRebarStyle(HWND)`, `ThemeSubclassPropSheetFrame(HWND)`, `ThemeAdjustBitmapForDarkMode(HBITMAP, COLORREF transparent)`.
- Palette data: `DarkColors[34]` + `DarkViewerColors[4]` (zero `SCF_DEFAULT` flags), pointer decoupling `SchemeColors` (user scheme) vs `CurrentColors` (derived), `CurrentViewerColors`; dark chrome table in `data-model.md §5` (BTNFACE 45,45,45; WINDOW 32,32,32; BTNTEXT/WINDOWTEXT 240,240,240; HIGHLIGHT 38,79,120; GRAYTEXT 150,150,150; 3DDKSHADOW 16,16,16; 3DLIGHT 58,58,58).
- Shared header `src/common/themes_palette.h`: `THEME_DARK_SYSCOLORS` / `THEME_DARK_PANEL_COLORS` / `THEME_DARK_VIEWER_COLORS` X-macros + `ThemeDarkAdaptColor` (added by 029), shared with `saltests`.
- Hook obligations (normative, `contracts/theme-engine.md`): `CDialog::DialogProc` (winlib.cpp) + `CPropSheetPage::DialogProc` call `ThemeHandleCtlColor` first; `CCommonDialog/CCommonPropSheetPage::NotifDlgJustCreated` → `ThemeApplyToDialog`; draw sites use accessors only; `GetSVGSysColor` routed via `ThemeSysColor`.
- `SetThemeMode` sequence (normative): set config → refresh HC → `UpdateCurrentColorsForTheme()` → swap WNDCLASS brushes/invalidate brush cache → `ColorsChanged(TRUE,FALSE,TRUE)` → `ThemeApplyToTopLevel(MainWindow)`.
- DWM dark title bar via documented `DWMWA_USE_IMMERSIVE_DARK_MODE` (=20); OS control darkening via `SetWindowTheme(L"DarkMode_Explorer")` / `L"DarkMode_CFD"` (combos). **Undocumented `SetPreferredAppMode`/`AllowDarkModeForWindow`/`FlushMenuThemes` explicitly rejected** (research D5).

**Surface**: main window chrome (panels, captions, dir/info lines, toolbars, rebar, owner-drawn menu bar + popups, command line, F-key bar, tooltips, msgbox, title bars), ~107–165 dialog templates via the two central procs, config property sheets, internal viewer, Find (live retheme), toolbar bitmap re-baking + SVG re-rasterization.

## 029-dark-toolbar-icons
**Mechanism**: `ThemeDarkAdaptColor()` extracted into `src/common/themes_palette.h` (bit-identical `MulDiv` rounding to the 028 bitmap transform); applied per-shape at SVG rasterization time in `RenderSVGImage` (nanosvg, ABGR↔RGB, alpha preserved, only `NSVG_PAINT_COLOR`); `ThemeAdjustBitmapForDarkMode` kept for the legacy raster sheet. Per-icon override contract (`contracts/dark-icon-override.md`): `<exe>\toolbars\dark\<Name>.svg` (source `src\res\toolbars\dark\`), `<Name>` = `SVGName` in `ToolBarButtons[]` (`src/toolbar4.cpp`), read **only** in Dark, only for enabled state, used **verbatim** (no auto-adaptation applied to it).
**Precedence (Dark)**: `dark override SVG > standard SVG auto-adapted > legacy raster (028 transform)`; Default: `standard SVG verbatim > legacy raster verbatim`.
**Surface**: top toolbar, directory-line toolbars, middle toolbar, bottom F-key bar, Find window toolbars, menu glyph icons. Also fixed asset typo `CilpboardCut.svg` → `ClipboardCut.svg` and added the `toolbars\dark` deploy line to `!populate_build_dir.cmd`.

## 036-plugin-dark-theme
**Mechanism**: 6 virtuals **appended** to `CSalamanderGeneralAbstract` (`src/plugins/shared/spl_gen.h:3469-3503`), ABI `LAST_VERSION_OF_SALAMANDER` 104 → **105**: `IsDarkThemeActive()`, `GetThemeSysColor(int)`, `GetThemeSysColorBrush(int)`, `ThemeApplyToDialog(HWND)`, `ThemeApplyToTopLevel(HWND)`, `ThemeHandleCtlColor(UINT,WPARAM,LPARAM,INT_PTR*)` — one-line delegations in `CSalamanderGeneral` to `themes.cpp`.
- winliblt central hook (`contracts/winliblt-theming.md`): `void SetupWinLibTheme(CSalamanderGeneralAbstract*)` in `src/plugins/shared/winliblt.{h,cpp}`; `CDialog::CDialogProc` + `CPropSheetPage::CPropSheetPageProc` call `ThemeApplyToDialog` on WM_INITDIALOG and forward unhandled CTLCOLOR to `ThemeHandleCtlColor` **object-first** (the dialog's own `DialogProc` wins). Called today in 14 plugins (7zip, checksum, dbviewer, filecomp, folders, ftp, mdview, pictview, portables, regedt, renamer, undelete, uniso + winliblt itself).
- Raw-dialog-proc guard pattern: per-plugin helpers `SFTPThemeDlgMsg` (`src/plugins/sftp/dialogs.h:23`), `ZIPThemeDlgMsg` (`src/plugins/zip/dialogs.h:25`), uncab ×4.
- Engine-level readability fixes landed in `themes.cpp` (post-audit): radios/group boxes stripped to classic (`SetWindowTheme(hwnd,L"",L"")`) in Dark; `ThemeFlatDisabledTextSubclassProc` for disabled statics.
**Surface**: all 18/19 shipped plugins — SFTP (8 raw dialogs + Logs), ftp (55 templates), zip (18 procs), uncab, pictview, dbviewer, filecomp, mdview, regedt, undelete, checksum, 7zip, peviewer, uniso, folders, portables, diskmap, tar.

## 037-mdview-dark-polish
**Mechanism**: two-layer first-paint fix — `CViewerWindow::BgBrush` created in `WM_CREATE` from `EffectiveTheme()->docBg`, answers `WM_ERASEBKGND`; plus `CMdWebHost::SetBackgroundColor(COLORREF)` → `ICoreWebView2Controller2::put_DefaultBackgroundColor` (progressive QI, failure tolerated), refreshed in `RebuildHtml`. Owner-drawn dark native menu in new `src/plugins/mdview/darkmenu.{h,cpp}`: `DarkMenuApply(HMENU)`, `DarkMenuRelease`, `DarkMenuMeasureItem`, `DarkMenuDrawItem`, `DarkMenuHandleMenuChar`, `DarkMenuReleaseFont`; `MENUINFO.hbrBack` with `MIM_BACKGROUND|MIM_APPLYTOSUBMENUS`; colors exclusively from `GetThemeSysColor`; `bool CViewerWindow::DarkMenus` = `IsDarkThemeActive()` snapshot at creation (036 reopen-adopts). Also fixed a pre-existing Debug `/RTCc` crash (`GetGValue` WORD-cast) in `htmlgen.cpp HexColor`.
**Surface**: mdview viewer window first paint + menu bar/drop-downs/scheme submenu.

## 044-fix-find-dark-mode
**Mechanism** (`contracts/theme-engine-additions.md`, delta on 028 — no public API signature changes; new subclasses are `static` internals of `themes.cpp`):
- `ThemeApplyChildEnumProc` dispatch-table delta: `Static` with `SS_ETCHEDHORZ/VERT/FRAME` → etched-line subclass painting via `ThemeDrawEdge`; `Edit` → **`DarkMode_CFD`** instead of `DarkMode_Explorer` + disabled-edit flat-repaint subclass (`COLOR_BTNFACE` fill, `COLOR_GRAYTEXT` text); `msctls_statusbar32` → new central dark status-bar subclass (`WM_ERASEBKGND` fill, `WM_PAINT` per-part via `SB_GETPARTS`/`SB_GETRECT`/`SB_GETTEXT`, `SBT_OWNERDRAW` forwarded as `WM_DRAWITEM` to parent, size grip drawn with dark bevels).
- Find-side obligations: second `ThemeApplyToDialog(HWindow)` at end of `CFindDialog` `WM_INITDIALOG` (idempotence relied on, `THEME_DARKENED_PROP` sentinel); `SetTextColor(ThemeSysColor(COLOR_BTNTEXT))` in the `IDC_FIND_STATUS` `WM_DRAWITEM`; `CFindTBHeader` `WM_NCPAINT` override; `CFoundFilesListView` header `NM_CUSTOMDRAW`; `UpdateProgressBarTheme` (`SetWindowTheme(L"",L"")` + `PBM_SETBKCOLOR`/`PBM_SETBARCOLOR`); `CToolBar` disabled-text single-pass `COLOR_GRAYTEXT` in dark (`toolbar2.cpp:648,659`).
**Surface**: Find window (separators, advanced-options box, status bar, header, disabled toolbar text, progress bar) — plus app-wide side effects: every `SS_ETCHED*` static, every bordered edit, `src/packac.cpp` status bar, all toolbars' disabled items.

## 009-sftp-dialog-style (skim)
`contracts/ui-house-style.md` — dialog conventions predating dark mode and still binding: **C1** `DIALOGEX` + `DS_SHELLFONT` + `FONT 8,"MS Shell Dlg"`; **C2** focused-field decoration must match core/FTP; **C3** *no process-wide or per-plugin styling side effects* — "**MUST NOT** register `ICC_STANDARD_CLASSES`, embed its own manifest, or apply per-plugin theming/subclassing that changes standard-control rendering"; **C5** recorded in constitution principle VI.

---

# (B) Consolidated documented known issues / deferred / out-of-scope / validation caveats

### OS-boundary limitations (recurring, accepted in 028 and re-affirmed in 036/037)
1. `specs/028-visual-themes/spec.md:162-166` — "windows drawn by the operating system or third parties (shell context menus, common Open/Save dialogs, shell property pages, other applications' windows launched from the program) follow the operating system's own theming, not the program's theme. This is expected and out of scope."
2. `specs/036-plugin-dark-theme/audit.md:75-81` — "**Known limitations (accepted, recorded)** … 1. **Native Win32 menu bars** on plugin frame windows (diskmap, mdview, dbviewer, filecomp, pictview, renamer window menus) stay light — the OS draws them; darkening needs undocumented APIs rejected in 028". (mdview's own menu bar was later fixed by 037; the other five remain.)
3. `specs/036-plugin-dark-theme/audit.md:26` — diskmap: "**native Win32 menu bar stays light — OS-drawn, 028 boundary**".
4. `specs/037-mdview-dark-polish/research.md:132-135` + `validation-results.md:33-36` — caption (Alt+Space) system menu: "the main window's own Alt+Space menu is the native light popup even in Dark theme (028/036 OS boundary); the viewer shows the same native popup — parity per spec FR-004/research R5."
5. `specs/037-mdview-dark-polish/research.md:118-121` — "**Known cosmetic limits** (accepted): the popup frame/border and the menu-bar nonclient edge are drawn by the system and may keep a thin light line; the main application's owner-drawn menus have the same class of edge artifacts."

### Deliberately deferred behaviors
6. `specs/036-plugin-dark-theme/audit.md:82-84` — "2. Already-open plugin windows keep the old theme until reopened — by design (clarification 2026-07-25). 3. Third-party plugins without the new API stay light — by design (FR-006)."
7. `specs/036-plugin-dark-theme/research.md:120-133` — "**R6 — What is explicitly NOT done**: No live repaint of open plugin windows on theme switch … No theming of OS-owned surfaces launched by plugins … No forced dark for plugins that never call the new API … No changes to `themes.cpp` palette values … Disabled-in-default-build plugins (automation, mmviewer, nethood, unchm, unmime, unole, unrar, checkver, demo*) receive the same treatment ONLY where the change is shared (winliblt, spl_gen.h); their plugin-local sweeps are out of scope until they are re-enabled."
8. `specs/028-visual-themes/spec.md:280-282` — "Manual switching only: automatic following of the Windows light/dark setting is **not** part of this feature (noted as a possible future enhancement)."
9. `specs/028-visual-themes/spec.md:167-171` — "Plugin-owned user interface: … any additional plugin windows that use their own fixed colors may remain light. Restyling plugin-internal UI beyond the program-provided color mechanism is out of scope for this feature." (closed by 036.)
10. `specs/028-visual-themes/spec.md:290-291` — "The feature targets the main program; the crash reporter, installer, and other auxiliary executables keep their current appearance." **← still open; salmon/setup/selfextr never themed.**
11. `specs/028-visual-themes/spec.md:283-285` — "The Dark theme changes colors and imagery only — no layout, spacing, font, or behavioral changes"; and 028 FR-011: the dark panel palette is **not user-editable** ("Dark uses a built-in dark panel palette that is not user-editable in this feature").
12. `specs/028-visual-themes/research.md:145-147` — "**Risk accepted**: a small tail of exotic controls may stay imperfect; SC-004 allows ≥95% dialog coverage, walkthrough will catch stragglers." (SC-004 itself only demands "at least 95% of program-created dialogs".)

### Known un-fixed defect classes named in research
13. `specs/044-fix-find-dark-mode/research.md:209-213` — "**R10 — Out of scope (recorded deliberately)**: `CToolbarHeader::OnPaint` (`src/gui.cpp:2860-2877`) has the same missing-`SetTextColor` defect class but is not used by the Find window — **left untouched (spec scope; candidate for a future sweep)**."
14. `specs/029-dark-toolbar-icons/research.md:84-92` — "`src\res\toolbars\CilpboardCut.svg` is misspelled … **Stale `CilpboardCut.svg` may remain in existing build outputs** — harmless (never referenced). Many buttons have no SVG at all (Select/Unselect/…): **out of scope to author them**; they stay on the raster path which already gets the 028 dark transform."
15. `specs/029-dark-toolbar-icons/analysis-toolbar-icons.md:79-80` — "**Ikony pluginů** (`CPlugins::CreateIconsList`, plugins2.cpp:984-1010) žádnou tmavou úpravu nedostávají — mimo rozsah 029, samostatné téma." (**plugin-supplied icons get no dark treatment at all**; spec FR-008 / assumptions repeat it: "Plugin-supplied icons and system-object icons are explicitly out of scope; adapting plugin icons to themes would be a separate feature.")
16. `specs/029-dark-toolbar-icons/contracts/dark-icon-override.md:40-42` — override files "read at image-list build time: the app picks it up on next start or next theme switch, **not mid-session**".
17. `specs/044-fix-find-dark-mode/research.md:24` — root-cause record still relevant: list-view **header text stays black** unless custom-drawn, because "the app (deliberately) never calls undocumented `SetPreferredAppMode`" — any new themed header needs the `NM_CUSTOMDRAW` escape hatch.

### Validation gaps / items not run-verified
18. `specs/044-fix-find-dark-mode/validation-results.md:85-92` — "**Windows High Contrast (FR-007)**: not toggled on the test machine (system-wide accessibility setting…). Guarantee is structural."
19. `specs/044-fix-find-dark-mode/validation-results.md:93-100` — "**Progress bar during duplicate search (R9)**: the plain search completes without showing the progress child … **Recommended one-time manual check: Find duplicates over a large folder in the Dark theme.**"
20. `specs/044-fix-find-dark-mode/validation-results.md:105-108` — "`src/packac.cpp` status bar inherits the central subclass (themed **the next time its dialog's `ThemeApplyToDialog` runs while it exists**)." (i.e. ordering-dependent, not directly verified.)
21. `specs/036-plugin-dark-theme/validation-results.md:60-64` — "**Runtime audit depth: 6 plugin surfaces opened live; the rest verified at mechanism level**" — 13 plugins were never opened in Dark mode; only compile-checked use of a proven mechanism.
22. `specs/036-plugin-dark-theme/validation-results.md:8-16` — SC-001 recorded as "✅ (with recorded OS-boundary exceptions)".
23. `specs/037-mdview-dark-polish/validation-results.md:55-73` — pre-existing Debug-only `/RTCc` `GetGValue` WORD-cast trap: fixed in `htmlgen.cpp HexColor` and `CMdWebHost::SetBackgroundColor`, but the **macro trap itself exists project-wide in plugin Debug builds** ("plugin debug builds compile with `/RTCc` … the WinAPI macro `GetGValue` contains a `(WORD)` cast that loses the blue byte of any `COLORREF`").
24. `specs/044-fix-find-dark-mode/validation-results.md:66-68` — light-mode diff caveat: "A Debug-vs-Release comparison showed small differences in disabled toolbar icon rasterization (y≈210–225)" (attributed to build flavor, not the feature).
25. `specs/044-fix-find-dark-mode/validation-results.md:17-19` — build-tooling gotcha: "builds must be invoked through PowerShell (`cmd /c` from the Git Bash tool mangles `/c` into `C:\` and silently does nothing)."

### Design risks recorded in 028 research (still load-bearing)
26. `specs/028-visual-themes/research.md:255-261` risk table — "`DarkMode_Explorer` SetWindowTheme is semi-documented | Fallback: colors still come from central WM_CTLCOLOR"; "Property-sheet frame (comctl-owned) resists theming | Subclass from first page; SC-004 tolerates imperfection tail"; "Deleting converted brushes that were once system brushes"; "Owner-drawn buttons (gui.cpp) draw uxtheme light parts in Dark".
27. `specs/028-visual-themes/research.md:181-184` — "**Non-drawing uses stay untouched**: `UpdateDefaultColors` seeding (`P`), luminance checks (`X`), the find high-contrast test — they must keep reflecting real system colors." (a hard rule for any new accessor conversion.)

---

# (C) Dark-mode fix commits (ad-hoc / outside the main feature-implementation commits)

**Outside the five theming features entirely** (only two exist; verified by pickaxe on `IsDarkThemeActive`, `DarkMode_`, `ThemeApplyToDialog`, `GetThemeSysColor`, and by `git log -- src/themes.cpp src/themes.h src/common/themes_palette.h src/plugins/shared/winliblt.* src/plugins/mdview/darkmenu.*`):
- `c92acf9` — **[032] visuals+governance: About/splash redesign (GDI wordmark, dark/light theme, brand accent)…** — the only non-theme feature that added `IsDarkThemeActive()` usage; `src/logo.cpp` (+118/−…) gained a dark branch. Current `src/logo.cpp` still keys the About box and splash gradient/text off `IsDarkThemeActive()` (lines 397, 446-469, 520-531; `TC_COLOR_NAVY`, `TC_COLOR_*_DARKBG`), and comments at line 207 note "the splash always uses the brand dark look (theme configuration may not be [loaded yet])". `src/logo.cpp` was later re-touched by 035 (`11726ff`, `102f80d`), 040 (`714ee22`) and 046 (`7f94f11`, `0db5255`) — brand/text only, but they sit on top of the dark branch.
- `f8b2c71` — **[045] copyright holder: the 2026 notice names Pavel Stupka, from a single define** — touches `src/themes.cpp`/`themes.h` **SPDX headers only**, no behavior.

**Ad-hoc fix commits inside the feature tags but after that feature's main implementation** (these are the "GUI-test gap" fixes and are where several undocumented behaviors live):
- `a483b31` — **[028] fix: dark-mode gaps from GUI test** — computed-brush fills (menu bar items, cmdline, edit-listbox), legacy icon dark transform, F-key keycap interiors, scrollbars via `DarkMode_Explorer` + style rule, universal class background swap, rebar grippers suppressed in dark. Touched: `themes.cpp/h`, `toolbar4.cpp`, `mainwnd1.cpp`, `mainwnd.h`, `salamdr1.cpp`, `menubar.cpp`, `editwnd.cpp`, `edtlbwnd.cpp`, `viewer2.cpp`, `viewer3.cpp`.
- `a65ac01` — **[028] fix: light lines between band rows and around command line in dark** — strips classic rebar `WS_BORDER`/`RBS_BANDBORDERS` while dark (`ThemeUpdateRebarStyle`), themes the command-line combo at creation ("created after startup theming pass"). Touched: `themes.cpp/h`, `editwnd.cpp`, `mainwnd1.cpp`, `mainwnd3.cpp`.
- `6b82de7` — **[036] fix: dark-theme readability from user feedback** — radios/group boxes stripped to classic drawing in Dark (`DarkMode_Explorer` paints their labels black), disabled static labels repainted flat via `SetWindowSubclass`. Touched: `src/themes.cpp` (+96) and `specs/036-plugin-dark-theme/audit.md` only.
- `e0f0a91` — **[029] US1** also carried an asset fix (`CilpboardCut.svg` typo + `build.cmd /E` deploy of `toolbars\dark`).

**No other dark-mode commits exist between features.** 047/048 (`8c7a433`, `fb21eb1`) touch no theme file; 047 only asserts icon legibility "in light and dark themes" (`specs/047-hot-path-names-icons/contracts/icon-set.md:32`) and left two theme-legibility verification tasks unchecked: `specs/047-hot-path-names-icons/tasks.md:84` (T021, "*deferred to a manual GUI session*") and `:114` (T027, "light/dark theme legibility of all 10 swatches (SC-005)").

---

# (D) Canonical patterns/contracts feature 049 must respect

**Invariants (non-negotiable across all five features)**
1. **Default = passthrough.** `ThemeSysColor(i) ≡ GetSysColor(i)`, `ThemeSysColorBrush ≡ GetSysColorBrush`, `ThemeDrawEdge ≡ DrawEdge`, every `ThemeApply*` is a no-op when `IsDarkThemeActive()` is FALSE (`028/contracts/theme-engine.md` §Invariants 1; `044/contracts/theme-engine-additions.md` §1). Every new subclass MUST route to `DefSubclassProc` in light mode.
2. **High Contrast wins** — folded into `IsDarkThemeActive()`; never re-implement it (`044` invariant 2).
3. **Documented APIs only** — `DWMWA_USE_IMMERSIVE_DARK_MODE`, `SetWindowTheme("DarkMode_Explorer"/"DarkMode_CFD"/"DarkMode_ItemsView")`, custom draw. `SetPreferredAppMode`, `AllowDarkModeForWindow`, `FlushMenuThemes`, UAH menu messages are **rejected repeatedly** (028 D5, 037 R4, 044 R7).
4. **Draw sites only** — never convert `UpdateDefaultColors` seeding, luminance math, or high-contrast probes to theme accessors (028 D7).
5. **No functional side effects** — subclasses handle paint-class messages only (`WM_PAINT/ERASEBKGND/NCPAINT/DRAWITEM` forwarding); input/focus/layout go to `DefSubclassProc` (044 invariant 4).
6. **Idempotence** — `ThemeApplyToDialog` is safe to call repeatedly (`THEME_DARKENED_PROP` sentinel, fixed `SetWindowSubclass` id); 044 depends on the second call after `WM_INITDIALOG`.
7. **Constitution VI** — application-wide visual changes must be one deliberate versioned decision, never a plugin side effect; no `ICC_STANDARD_CLASSES`, no plugin manifests, no class re-registration (`.specify/memory/constitution.md:123-128`, `009/contracts/ui-house-style.md` C3).

**Central hook points (add behavior here, not per dialog)**
- `ThemeApplyChildEnumProc` dispatch table in `src/themes.cpp` — the single place per-child-class theming lives (028 + 036 radio/groupbox/disabled-static + 044 etched-static/Edit/status-bar entries).
- `CDialog::CDialogProc` / `CPropSheetPage::CPropSheetPageProc` (core `winlib.cpp` / `sheets.cpp`; plugin `winliblt.cpp`) — `ThemeHandleCtlColor` first in core, **object-first with theme fallback** in winliblt.
- `CCommonDialog/CCommonPropSheetPage::NotifDlgJustCreated` → `ThemeApplyToDialog`; note it fires **before** the dialog's own `WM_INITDIALOG` body, so any child created there needs a second `ThemeApplyToDialog` (044 R6).
- `SetThemeMode` → `ColorsChanged(TRUE,FALSE,TRUE)` → `ReleaseGraphics`/`InitializeGraphics` — all image lists/toolbar bitmaps/pens/brushes rebuild for free; do not add a parallel rebuild hook (029 R3).

**Plugin theme API entry points (ABI 105 — must not be reordered or extended in the middle)**
- `CSalamanderGeneralAbstract`: `IsDarkThemeActive`, `GetThemeSysColor`, `GetThemeSysColorBrush`, `ThemeApplyToDialog`, `ThemeApplyToTopLevel`, `ThemeHandleCtlColor` — appended at vtable end; new methods may only be appended after them with a `spl_vers.h` history row and `LAST_VERSION_OF_SALAMANDER` bump.
- Brushes are **engine-owned**: plugins must not delete them and must fetch at draw time, never cache across a switch (`036/contracts/plugin-theme-api.md`).
- `SetupWinLibTheme(SalamanderGeneral)` once in `SalamanderPluginEntry`; raw procs use the two-touchpoint pattern (`WM_INITDIALOG` → `ThemeApplyToDialog`; CTLCOLOR → `ThemeHandleCtlColor` guard) via per-plugin helpers `SFTPThemeDlgMsg` / `ZIPThemeDlgMsg`.
- **Theme is read at window creation; reopen adopts** — no live repaint pushed into plugin windows; `PLUGINEVENT_COLORSCHANGED` still fires (036 clarification, 037 R6, `DarkMenus` snapshot pattern).

**Palette / color rules**
- Single source of truth: `src/common/themes_palette.h` (`THEME_DARK_SYSCOLORS`, `THEME_DARK_PANEL_COLORS`, `THEME_DARK_VIEWER_COLORS`, `ThemeDarkAdaptColor`), shared with `saltests`; `DarkColors`/`DarkViewerColors` must keep **zero `SCF_DEFAULT` flags**.
- `SchemeColors` (user scheme) is never mutated by the theme; `CurrentColors`/`CurrentViewerColors` are derived by `UpdateCurrentColorsForTheme()` (FR-011/FR-008).
- Contrast bars: text ≥ 4.5:1, disabled ≥ 3:1, icon strokes ≥ 3:1 vs `COLOR_BTNFACE` RGB(45,45,45) — unit-tested (`TestFindDarkModeSurfaces`, `TestDarkIconColorAdaptation`; saltests at 1133 checks after 044).
- Control-class conventions now canonical: edits/combos/command line = `DarkMode_CFD`; list/tree = `DarkMode_ItemsView`/`DarkMode_Explorer` + explicit `SetBk/TextColor`; radios & group boxes = classic (`L""`); disabled statics/edits = flat repaint subclass; status bars = engine subclass; toolbars' disabled text = single-pass `COLOR_GRAYTEXT` in dark.
