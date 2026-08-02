# Contract: Theme Engine Additions (049)

Delta on `specs/028-visual-themes/contracts/theme-engine.md` and
`specs/044-fix-find-dark-mode/contracts/theme-engine-additions.md`. All 028/044 invariants
remain binding: Default theme = byte-identical passthrough; High Contrast wins (folded into
`IsDarkThemeActive`); documented APIs only; draw-site-only conversions; paint-class messages
only in subclasses; idempotent application.

## 1. New public API (`src/themes.h`)

```c
// Re-applies per-HWND theming (SetWindowTheme variants + engine subclasses +
// control colors) to 'hWnd' itself and every descendant. Use after creating
// or recreating controls at runtime (view-mode switches, inline editors,
// late-created children). Symmetric: restores light variants in the Default
// theme. Idempotent. Does NOT touch the DWM title bar (that stays with
// ThemeApplyToTopLevel/ThemeApplyToDialog).
// Restriction: thread owning 'hWnd'.
void ThemeApplyToWindowTree(HWND hWnd);

// Applies/removes dark colors on a native tooltips_class32 window (tooltips
// are WS_POPUP — the child sweep never reaches them). Dark: strips the visual
// style and sets TTM_SETTIPBKCOLOR/TTM_SETTIPTEXTCOLOR from the palette
// (COLOR_INFOBK/COLOR_INFOTEXT); Default: restores the native themed look.
// Call once after creating the tooltip.
void ThemeApplyToTooltip(HWND hTooltip);
```

## 2. `ThemeApplyChildEnumProc` dispatch-table delta

| Class / condition | New dark behavior | Light behavior |
|---|---|---|
| `Button` + `BS_GROUPBOX` | keep classic strip (036) **+ install group-box paint subclass (ID 6)**: etched frame via `ThemeDrawEdge`, label opaque `COLOR_BTNFACE`/`COLOR_BTNTEXT` (`COLOR_GRAYTEXT` disabled) | remove subclass; classic unchanged |
| `Button` + `BS_(AUTO)RADIOBUTTON` | keep classic strip (036) **+ install radio-glyph subclass (ID 7)**: default paint via `WM_PRINTCLIENT` into memory DC, glyph square overlaid dark (interior `COLOR_WINDOW`, ring/dot `COLOR_BTNTEXT`, disabled `COLOR_GRAYTEXT`), single blit | remove subclass |
| `SysDateTimePick32`, `msctls_hotkey32` | **install grayscale-remap subclass (ID 8)**: native `WM_PRINTCLIENT` into 32-bpp DIB; grayscale pixels (channel spread ≤ 8) remapped linearly white→`COLOR_WINDOW`, black→`COLOR_WINDOWTEXT`; chromatic pixels (segment selection) untouched; `WM_ERASEBKGND` = TRUE | remove subclass; native |
| `msctls_progress32` | `SetWindowTheme(L"", L"")` + `PBM_SETBKCOLOR = ThemeSysColor(COLOR_BTNSHADOW)` + `PBM_SETBARCOLOR = ThemeSysColor(COLOR_HIGHLIGHT)` (exact 044 Find recipe, now central) | `SetWindowTheme(NULL, NULL)` + `CLR_DEFAULT` both |
| `SysListView32` + `LVS_EX_CHECKBOXES` | install `CreateCheckboxImagelist(IconSizes[ICONSIZE_16])` as `LVSIL_STATE`, marker prop "SalDarkChkIL" guards re-entry | if marker present: detach + destroy custom list, re-toggle `LVS_EX_CHECKBOXES` (regenerates native), clear marker |
| `Edit` (disabled repaint subclass, ID 4) | **`ES_MULTILINE` no longer bails**: multiline branch fills `COLOR_BTNFACE`, draws `WM_GETTEXT` via `DrawTextW(DT_WORDBREAK\|DT_NOPREFIX\|DT_EDITCONTROL)` inside `EM_GETRECT`, `COLOR_GRAYTEXT` | unchanged |

Subclass ID registry after 049: 1 propsheet frame, 2 disabled static, 3 etched static,
4 disabled edit (incl. multiline), 5 status bar, **6 group box, 7 radio glyph, 8 grayscale
remap (incl. dark `WM_NCPAINT` client edge), 9 listview header labels** (installed on the
listview; answers the header's `NM_CUSTOMDRAW` with the 044 Find light-text recipe — closes
the black-on-dark "Name"/"Hot Key" header class app-wide). New subclasses handle paint-class
messages only (`WM_PAINT`/`WM_ERASEBKGND`/`WM_NCPAINT`/the header `NM_CUSTOMDRAW` reply); all
else `DefSubclassProc`.

## 3. Call-site obligations (main app)

| Site | Obligation |
|---|---|
| `CFilesBox::SetMode` (`filesbx1.cpp`) | `ThemeApplyToWindowTree(HWindow)` after `ShowHideChilds`/`LayoutChilds` — covers every view-mode entry point and the header toggle |
| `CFilesBox::WindowProc` | route `WM_CTLCOLOREDIT` through `ThemeHandleCtlColor` (quick-rename colors) |
| Quick rename (`fileswn5.cpp`) | `ThemeApplyToWindowTree(edit)` after creation |
| `CEditListBox` (`edtlbwnd.cpp`) | `ThemeApplyToWindowTree(EditLine->HWindow)` after creation; dark branch of the arrow-button paint uses face fill + `ThemeDrawEdge` + `COLOR_BTNTEXT` triangle |
| Views/Hot Paths label edits (`dialogs4.cpp`) | `ThemeApplyToWindowTree(ListView_GetEditControl(...))` in `LVN_BEGINLABELEDIT` |
| `CPackACDialog` (`packac.cpp`) | second `ThemeApplyToDialog(HWindow)` at end of `WM_INITDIALOG` (Find precedent) |
| Native tooltip creations (`mainwnd3.cpp`, `viewer3.cpp`) | `ThemeApplyToTooltip(hTT)` after creation |
| `CCommonDialog` / `CCommonPropSheetPage` (`dialogs2.cpp`) | `WM_THEMECHANGED` → `ThemeApplyToDialog(HWindow)`; `CCommonDialog` also on `WM_SYSCOLORCHANGE` |
| `CMainWindow` (`mainwnd3.cpp`) | `WM_THEMECHANGED` → `ThemeApplyToDialog(HWindow)`; `WM_SETTINGCHANGE` with `wParam == SPI_SETHIGHCONTRAST` → full `WM_SYSCOLORCHANGE` sequence |
| Viewer (`viewer3.cpp`) | `WM_THEMECHANGED` → existing `WM_USER_CFGCHANGED` re-apply block |
| `ColorsChanged` (`salamdr1.cpp`) | `UpdateViewerColors(CurrentViewerColors)` (contract 028 §68 conformance) |
| `UpdateCurrentColorsForTheme` (`themes.cpp`) | pre-create the full dark brush cache (thread-safety; viewer thread must never trigger lazy `CreateSolidBrush`) |

## 4. Palette / drawing contract changes

- `THEME_DARK_SYSCOLORS`: `COLOR_WINDOW` = 56,56,56. New invariant: `Lum(COLOR_WINDOW) >
  Lum(COLOR_BTNFACE)` (input surfaces lighter than dialog face) — saltests-enforced.
- `CStaticText` `STF_HYPERLINK_COLOR`: dark → `ThemeSysColor(COLOR_HOTLIGHT)`; light →
  `RGB(0,0,255)` unchanged. New saltests floor: HOTLIGHT vs `RGB(0x0A,0x14,0x24)` ≥ 4.5.
- `common/sheets.cpp` tree theme: `SheetsIsDarkHook` (new, default NULL = light) selects
  `L"DarkMode_Explorer"` vs `L"explorer"` at creation.
- `common/winlib`(+`winliblt`) validation message boxes: `WinLibMessageBoxHook` (new, default
  `::MessageBox`); core installs themed `SalMessageBox` adapter, winliblt installs
  `SalamanderGeneral->SalMessageBox` adapter in `SetupWinLibTheme`.

## 5. Explicitly unchanged

Native Win32 menus (deferred feature), OS common dialogs, splash, auxiliary executables,
`SchemeColors`, panel/viewer palettes, plugin reopen-adopts contract, subclass IDs 1–5
semantics (except the documented ID-4 multiline extension).
