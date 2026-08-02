# Dark Mode Stabilization — Consolidated Audit (Feature 049)

**Created**: 2026-08-02
**Sources**: six independent exploration agents, raw reports in this directory
(`raw-01-theme-engine-core.md`, `raw-02-dialogs-popups.md`, `raw-03-scrollbars-recreation.md`,
`raw-04-links-foreground-colors.md`, `raw-05-history.md`, `raw-06-control-plugin-sweep.md`).
This file is the deduplicated synthesis; the raw files carry full evidence chains.

## 1. User-reported symptoms → root causes

| Reported symptom | Root cause | Defect ID |
|---|---|---|
| Scroll bars turn light after ALT+3/4/5 view switch | `CFilesBox::ShowHideChilds()` (`src/filesbx1.cpp:2128-2214`) destroys and recreates the `"scrollbar"` child HWNDs; `SetWindowTheme` is per-HWND and nothing on the view-switch path re-applies it. Not a style reset — brand-new never-themed windows. Every view-mode entry point (Alt+digit, menu/toolbar commands, Alt+wheel, plugin-forced view modes, header-line toggle) funnels through this one function. | A1 |
| Black text fields on dark background in some pop-ups | Two combined causes: (a) palette maps `COLOR_WINDOW` to RGB(32,32,32), *darker* than the dialog face RGB(45,45,45), so every input surface reads as a black hole (opposite of the Win11 dark convention); (b) read-only/disabled edits are answered via `WM_CTLCOLORSTATIC` with the *face* brush, so sibling fields in one dialog are inconsistently black vs. gray. | C1, C2 |
| White separator lines that look wrong | `SS_ETCHED*` statics were fixed in 044, but **group boxes** (26 controls across 15 dialogs) were stripped to classic rendering in 036 for label readability — classic group-box chrome paints its etched frame with real (light) system colors and never consults `WM_CTLCOLORSTATIC`. Radio glyphs have the same classic-strip cause. | B1, B2 |
| Dark-blue About-dialog link on dark background | `CStaticText::WindowProc` hardcodes `SetTextColor(hDC, RGB(0,0,255))` for `STF_HYPERLINK_COLOR` (`src/gui.cpp:1146`), discarding the theme-correct color it just fetched. ~2.1:1 contrast on About navy. Single root cause for **all** links in the app (6 live core sites + 8 plugin sites). Palette already carries a validated replacement: `COLOR_HOTLIGHT` = RGB(102,178,255), ≥4.5:1 tested, currently unread by any drawing code. | D1 |

## 2. Master defect inventory

Severity: ● high (visible daily / unreadable), ◐ medium (visible in specific dialogs), ○ low (edge/latent).

### Cluster A — Windows created or recreated after the theming sweep (systemic root cause)

The engine's per-child sweep (`ThemeApplyChildEnumProc` via `ThemeApplyToDialog`) is a one-shot
`EnumChildWindows` snapshot; `NotifDlgJustCreated` fires *before* the dialog's own `WM_INITDIALOG`
body (`common/winlib.cpp:726` vs `:767`). Anything born later is never themed. There is **no public
subtree re-apply helper** — that is the missing engine piece.

| ID | Sev | Defect | Evidence |
|---|---|---|---|
| A1 | ● | Panel H+V scrollbars (and bottom bar) light after every view-mode change; covers Alt+3/4/5, `CM_*MODE_*` commands, Alt+wheel, smart-column toggle, config apply, font/DPI reload, plugin FS/archives forcing Detailed, header-line toggle | raw-03 (A), (B) rows 1a–1g, 2 |
| A2 | ● | Quick-rename inline edit (F2) — white edit over the dark panel; parent `CFilesBox` has no `WM_CTLCOLOREDIT` | raw-03 row 11; raw-01 B2 (`fileswn5.cpp:2749-2757`) |
| A3 | ◐ | `CEditListBox` inline editor (User Menu / Hot Paths / Viewers / Editors config pages) — created after sweep, untouched | raw-03 row 12 (`edtlbwnd.cpp:474-485`) |
| A4 | ◐ | ListView in-place label edits in Configuration → Views / Hot Paths — white edit inside dark listview | raw-01 B5 (`dialogs4.cpp:1372,1464,3129,3164`) |
| A5 | ◐ | Auto-Configuration (Archivers) status bar created inside `WM_INITDIALOG` → 044 status-bar subclass never installed; 044's assumption it would "heal" was wrong | raw-02 #7 (`packac.cpp:75-78`) |
| A6 | — | Engine gap enabling all of the above: no public `ThemeApplyToWindowTree`-style helper; Find's second `ThemeApplyToDialog` call is the only (fragile, undiscoverable) workaround | raw-03 (C); raw-01 B5 |

### Cluster B — Bright chrome remnants ("white lines")

| ID | Sev | Defect | Evidence |
|---|---|---|---|
| B1 | ● | Group boxes draw classic bright etched frames — 26 controls / 15 dialogs (config pages, master password, load/save selection, language selectors, …) | raw-02 #1 (full control list) |
| B2 | ● | Radio buttons draw the classic bright glyph (white interior) while checkboxes stay themed | raw-02 #2 |
| B3 | ◐ | `CEditListBox` arrow button drawn with raw `DrawFrameControl` — classic light 3D button on dark list | raw-01 B8 (`edtlbwnd.cpp:555`) |
| B4 | ○ | Rebar background strip: styles stripped but no `RB_SETBKCOLOR`/`RBBIM_COLORS` anywhere | raw-06 matrix (ReBarWindow32) |
| B5 | ○ | Config tree gets light `L"explorer"` theme at creation (`sheets.cpp:702`), corrected only by a later ordering-dependent pass | raw-01 B6; raw-02 #11 |

### Cluster C — Field/background inconsistencies ("black boxes")

| ID | Sev | Defect | Evidence |
|---|---|---|---|
| C1 | ● | Palette inversion: `COLOR_WINDOW` (32,32,32) darker than `COLOR_BTNFACE` (45,45,45) → every edit/combo/listbox/listview is a black hole on a lighter face | raw-02 #10 (`common/themes_palette.h:24` vs `:32`) |
| C2 | ● | Read-only/disabled edits (routed via `WM_CTLCOLORSTATIC`) get the face brush while editable ones get near-black → inconsistent siblings; bordered read-only edits look like empty sunken boxes. ~20 concrete controls listed | raw-02 #8 |
| C3 | ◐ | Disabled **multiline** edits excluded from the 044 flat-repaint subclass (`ES_MULTILINE` bail-out) | raw-02 #9 (`themes.cpp:386-388`) |
| C4 | ◐ | Change Icon dialog owner-drawn list fills with raw system brushes → white list background | raw-02 #6 (`dialogs3.cpp:2392-2393`) |
| C5 | ● | `CExecuteWindow` — light class background + near-white themed text = unreadable | raw-01 B3 (`pack3.cpp:2025-2034`) |
| C6 | ◐ | `SAVEBITS_CLASSNAME` / `SHELLEXECUTE_CLASSNAME` keep light class brushes (feeds C5) | raw-01 B4 (`salamdr1.cpp:4325,4334`) |
| C7 | ○ | Configuration → Views listviews reset to light on `WM_SYSCOLORCHANGE` — the single un-converted `GetSysColor` drawing call in the app | raw-01 B9; raw-02 #12 (`dialogs4.cpp:1362-1363`) |

### Cluster D — Hardcoded foreground colors

| ID | Sev | Defect | Evidence |
|---|---|---|---|
| D1 | ● | All hyperlinks hardcode pure blue `RGB(0,0,255)` — About, message boxes with URLs, Plugins Manager, Config→Keyboard, Config→Regional, Language Selection + 8 plugin sites (pictview/mmviewer About, ftp hints, demoplug). Fix at single root `gui.cpp:1146`; palette slot `COLOR_HOTLIGHT` RGB(102,178,255) exists, is contrast-tested (≥4.5:1; ≈8:1 on About navy), and is currently unused | raw-04 (A),(B),(D) |
| D2 | ● | `CToolbarHeader::OnPaint` draws caption with no `SetTextColor` → black-on-dark headers; deferred by 044 as "future sweep". 7 live instances: Config→Views ×2, Hot Paths, Plugins ×2, Icon Overlays, and every `CEditListBox` header | raw-02 #5 (`gui.cpp:2860-2877`) |
| D3 | ◐ | Config→Colors: five swatch buttons forced white-on-white when no mask selected | raw-04 C2 (`dialogs4.cpp:3682-3694`) |
| D4 | ◐ | Drive Info gauge: hardcoded black outline pen + hardcoded pie/legend colors (256-color branch even reuses pure blue) | raw-04 C3, C4 (`gui.cpp:1574`, `dialogs3.cpp:1715-1725`) |
| D5 | ○ | Focus/drag markers: Customize-Toolbar listbox `DrawFocusRect` after black `SetTextColor`; toolbar drag insert-mark black pen; panel rubber-band white/black pair | raw-04 C5–C7 (`toolbar3.cpp:522`, `toolbar2.cpp:825`, `filesmap.cpp:679-680`) |

### Cluster E — Control classes with no dark handling

| ID | Sev | Defect | Evidence |
|---|---|---|---|
| E1 | ● | `SysDateTimePick32` — 10 white fields: Change Attributes (6), Find → Advanced Options (4); no branch in the enum proc, drop-down calendar also light | raw-02 #3; raw-06 gap 4 |
| E2 | ◐ | `msctls_hotkey32` — Plugin Keyboard Shortcuts dialog (+ pictview resource) | raw-02 #4; raw-06 gap 10 |
| E3 | ◐ | `msctls_progress32` has no central branch — only Find fixed locally; light bars in checksum (×2) and pictview (×1) dialogs | raw-06 matrix + gap 10 |
| E4 | ◐ | Native `tooltips_class32` (split-bar drag tooltip, viewer tooltip) light-yellow over dark UI | raw-06 gap 7 (`mainwnd3.cpp:5100`, `viewer3.cpp:560`) |
| E5 | ◐ | `LVS_EX_CHECKBOXES` native light glyphs in dark listviews — 7 sites; dark glyph generator (`CreateCheckboxImagelist`, `gui.cpp:3788`) already exists and is wired at only 2 sites | raw-06 gap 8 |

### Cluster F — Plugin surfaces

| ID | Sev | Defect | Evidence |
|---|---|---|---|
| F1 | ● | **peviewer**: `SetupWinLibTheme` never called — its Configuration dialog is 100 % light; one-line adoption | raw-06 gap 1 |
| F2 | ● | Plugin property-sheet **frames** light around dark pages (ftp, pictview, filecomp): `ThemeSubclassPropSheetFrame` is not exported through the plugin API (ABI 105 has only 6 theme methods) | raw-06 gap 3 |
| F3 | ◐ | mdview Find dialog (Ctrl+F): raw `DialogBoxParamW` proc without the theme touchpoints — light box in an otherwise dark viewer | raw-06 gap 6 (`mdview/viewer.cpp:338,667`) |
| F4 | ◐ | diskmap About box: raw `DialogBox`, no theme call (frame window itself is only `ThemeApplyToTopLevel`) | raw-06 (`DiskMap/GUI.AboutDialog.h:34`) |
| F5 | ◐ | Raw `::MessageBox` sites bypassing themed `CMessageBox` — incl. `common/winlib.cpp:1301/1352/1410/1476` numeric-validation boxes reachable from **every** winliblt plugin dialog, plus 9 core sites | raw-02 (A); raw-06 matrix |
| F6 | — | Plugin hyperlink sites — resolved automatically by the D1 root fix (shared drawing path) | raw-04 B1 |

### Cluster G — Lifecycle / robustness (latent)

| ID | Sev | Defect | Evidence |
|---|---|---|---|
| G1 | ◐ | `WM_SETTINGCHANGE` does not call `RefreshThemeHighContrastState()` — if a Windows build delivers `SPI_SETHIGHCONTRAST` without `WM_SYSCOLORCHANGE`, the cached High-Contrast state goes stale (HC-wins invariant silently broken) | raw-01 C4 (`mainwnd3.cpp:1271-1327`) |
| G2 | ◐ | `WM_THEMECHANGED` handled nowhere — comctl32 resets all `SetWindowTheme` state on a visual-style change with no re-apply | raw-01 C4 |
| G3 | ○ | Viewer thread calls the main-thread-only engine (`ThemeSysColorBrush` lazy `CreateSolidBrush` into an unguarded global array; only 2 indices pre-warmed) | raw-01 B12 |
| G4 | ○ | `ColorsChanged` path calls `UpdateViewerColors(ViewerColors)` instead of contract-mandated `CurrentViewerColors` — harmless today, bites if a dark viewer entry ever gets `SCF_DEFAULT` | raw-01 C5 |
| G5 | ○ | Open dialogs' per-dialog `WM_SYSCOLORCHANGE` handlers only re-set list colors; none re-runs `ThemeApplyToDialog`, so subclass/`SetWindowTheme` state is not refreshed for windows open during a system color change | raw-01 C4 |

## 3. Scope decisions (proposed for the spec)

**In scope**: clusters A–G above.

**Out of scope, recorded with rationale**:
1. **Native Win32 menus** — internal viewer menu bar + dropdowns (raw-01 B1), 13 core `TrackPopupMenu` sites (raw-01 B7), plugin native menu bars (diskmap, filecomp) and plugin raw popup menus (ftp/sftp/zip/folders/dbviewer). Darkening native menus requires either undocumented APIs (`SetPreferredAppMode`/UAH — rejected in 028 D5, 037 R4, 044 R7 and **still rejected**) or converting each site to the owner-drawn `CMenuPopup`/`CMenuBar` infrastructure — a menu-subsystem feature of its own. Recommend a dedicated follow-up feature ("dark native menus / menu unification").
2. **OS-owned surfaces** — common Open/Save/Font/Color dialogs, shell context menus, folder pickers: 028 boundary, re-affirmed.
3. **Auxiliary executables** — setup, salmon, sfx stubs (`zip/selfextr`): excluded by 028, unchanged.
4. **Splash screen** — brand-dark by design (032); not a defect.
5. **Live re-theme of already-open plugin windows** — reopen-adopts stays the contract (036 clarification).
6. **Disabled-in-default-build plugins** (mmviewer, demoplug, …) — shared-code fixes (D1) reach them, plugin-local sweeps stay out of scope until re-enabled (036 R6).

## 4. Binding constraints for the fix (from raw-05, verified against raw-01)

1. Default theme = byte-identical passthrough; every new subclass routes to `DefSubclassProc` in light mode.
2. High Contrast wins; never re-implement the check.
3. **Documented APIs only** — `DWMWA_USE_IMMERSIVE_DARK_MODE`, `SetWindowTheme` families, custom draw. No `SetPreferredAppMode`/`AllowDarkModeForWindow`/`FlushMenuThemes`/UAH.
4. Draw sites only — never convert seeding/luminance/HC-probe reads (`UpdateDefaultColors` etc.).
5. Paint-class messages only in subclasses; no functional side effects.
6. `ThemeApplyToDialog` stays idempotent (`THEME_DARKENED_PROP`); subclass IDs 1–5 are taken.
7. Central hook points: `ThemeApplyChildEnumProc` dispatch table for per-class behavior; `CCommonDialog`/`CCommonPropSheetPage`/winliblt for routing; `SetThemeMode` → `ColorsChanged` for rebuilds; `SheetsGetSysColorHook` pattern for `src/common/` (which must not include `themes.h`).
8. Plugin ABI: the 6 theme virtuals are appended at vtable end; any new export (needed for F2) must be **appended** with a `spl_vers.h` history row and `LAST_VERSION_OF_SALAMANDER` bump (105 → 106). Engine-owned brushes; fetch at draw time.
9. Palette single source of truth: `src/common/themes_palette.h`; contrast bars text ≥4.5:1, disabled ≥3:1 (saltests-enforced, 1133 checks); no `RGB()` literals at draw sites.
10. Constitution VI: app-wide visual changes are one deliberate versioned decision; no per-plugin styling side effects (009 C3).

## 5. Verification notes for planning

- saltests currently asserts `COLOR_HOTLIGHT` ≥4.5:1 vs `COLOR_WINDOW`/`COLOR_BTNFACE` but **not** vs About navy `#0A1424` — add when D1 lands.
- 044 left two recorded manual checks never run: Find-duplicates progress bar in dark; High Contrast toggle (structural only). Fold into this feature's validation.
- 047 left two theme-legibility tasks unchecked (hot-path icon swatches in dark) — independent, but the same GUI session can close them.
- Build gotcha (044): invoke builds through PowerShell, not `cmd /c` from Git Bash.
- A previous GUI screenshot exists at `temp/dark_after_find_expanded.png` confirming C1 visually.
