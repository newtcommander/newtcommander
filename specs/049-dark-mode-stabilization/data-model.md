# Data Model — Dark Mode Stabilization (049)

No persistent data changes (registry format, config, file formats all unchanged). The "data"
of this feature is the palette and per-window theming state.

## 1. Dark chrome palette delta (`src/common/themes_palette.h`)

| Entry | Before | After | Consumers affected |
|---|---|---|---|
| `COLOR_WINDOW` | 32,32,32 | **56,56,56** | edits, combos, list boxes, listviews, treeviews, message-box body, main/viewer class backgrounds |

All other `THEME_DARK_SYSCOLORS`, `THEME_DARK_PANEL_COLORS`, `THEME_DARK_VIEWER_COLORS` entries
unchanged (panels keep ITEM_BK_NORMAL 32,32,32 — deliberate: panels are content canvases, not
input fields).

**Derived invariants (saltests-enforced)**:
- `WINDOWTEXT/WINDOW ≥ 4.5` (10.2 after change)
- `HOTLIGHT/WINDOW ≥ 4.5` (5.2), `HOTLIGHT/BTNFACE ≥ 4.5` (unchanged)
- `GRAYTEXT/WINDOW ≥ 3.0` (3.9)
- **new** `Lum(WINDOW) > Lum(BTNFACE)` — field-lighter-than-face convention
- **new** `HOTLIGHT vs About navy RGB(0x0A,0x14,0x24) ≥ 4.5` (≈ 8:1)

## 2. Newly-consumed palette entry

| Entry | Value (dark) | New consumer |
|---|---|---|
| `COLOR_HOTLIGHT` | 102,178,255 | `CStaticText` `STF_HYPERLINK_COLOR` draw site (`gui.cpp:1146`), dark branch only |

## 3. Per-window theming state (transient, no persistence)

| State | Storage | Lifecycle |
|---|---|---|
| `THEME_DARKENED_PROP` ("SalThemeDark") | window prop | unchanged (028); still owned by `ThemeApplyToDialog`/`ThemeApplyToTopLevel` |
| Subclass IDs 1–5 | `SetWindowSubclass` | unchanged (028/036/044) |
| Subclass ID 6 — group box | `SetWindowSubclass` | installed dark / removed light by enum proc |
| Subclass ID 7 — radio glyph | `SetWindowSubclass` | installed dark / removed light by enum proc |
| Subclass ID 8 — grayscale remap (DTP, hotkey) | `SetWindowSubclass` | installed dark / removed light by enum proc |
| Dark checkbox state-imagelist marker | window prop ("SalDarkChkIL") + `HIMAGELIST` | set on dark sweep; on light sweep: detach, destroy, re-toggle `LVS_EX_CHECKBOXES`; listview destroys the list at window destruction |
| Dark brush cache `ThemeDarkBrushes[]` | engine globals | **pre-created in full** by `UpdateCurrentColorsForTheme` (was lazy) — removes viewer-thread race |

## 4. Plugin ABI (interface version)

| Item | Before | After |
|---|---|---|
| `LAST_VERSION_OF_SALAMANDER` | 105 | **106** |
| `CSalamanderGeneralAbstract` vtable | …`ThemeHandleCtlColor` (6 theme methods) | + `ThemeSubclassPropSheetFrame(HWND)` appended at end |
| Compatibility | — | plugins built for ≤ 105 load and run unchanged (pure append, 036 precedent) |

## 5. Hooks (src/common ↔ app decoupling)

| Hook | Type | Default | Installed by |
|---|---|---|---|
| `SheetsGetSysColorHook` | existing | `GetSysColor` | `salamdr1.cpp` → `ThemeSysColor` (unchanged) |
| `SheetsIsDarkHook` | **new** | NULL (= light) | `salamdr1.cpp` → `IsDarkThemeActive` |
| `WinLibMessageBoxHook` | **new** | `::MessageBox` | core → `SalMessageBox` adapter; winliblt → `SalamanderGeneral->SalMessageBox` adapter |
