# Data Model: Theme-Adaptive Toolbar Icons

**Feature**: 029-dark-toolbar-icons | **Date**: 2026-07-21

No persistent data is introduced. The "model" is the set of icon asset
kinds and the derivation of the runtime image lists from them.

## Entities

### Icon asset kinds (disk / resources)

| Entity | Location | Role | Themed how |
|--------|----------|------|-----------|
| Standard SVG glyph | `<exe>\toolbars\<Name>.svg` (source: `src\res\toolbars\`) | Primary artwork for ~63 commands (`CButtonData.SVGName`) | Dark: auto-adapted per shape (`ThemeDarkAdaptColor`), unless an override exists; Default: used verbatim |
| Dark override SVG | `<exe>\toolbars\dark\<Name>.svg` (source: `src\res\toolbars\dark\`) | Optional hand-tuned dark variant of one command icon | Dark: used verbatim (highest precedence); Default: never used |
| Legacy raster sheet | resources `IDB_TOOLBAR_16` (.bmp) / `IDB_TOOLBAR_256` (.png), also `IDB_MENU`, `IDB_EDTLBTB` | Fallback artwork for buttons without SVG; base layer under SVG stamps | Dark: whole-sheet pixel transform `ThemeAdjustBitmapForDarkMode` (existing 028 behavior) |
| Shell object icons | shell32.dll / imageres.dll (`Shell32ResID`) | Buttons depicting real system objects | Never recolored (FR-008) |

### Runtime derived objects

| Entity | Created in | States |
|--------|-----------|--------|
| Composited color strip | `CreateToolbarBitmaps()` (toolbar4.cpp:730) | enabled/hot look; per active theme |
| Gray strip + mask | `CreateGrayscaleAndMaskBitmaps_tmp()` (toolbar4.cpp:937) | disabled look, derived from color strip (theme-dependent transitively) |
| `HHotToolBarImageList` / `HGrayToolBarImageList` | `InitializeGraphics()` (salamdr1.cpp:2400-2538) | shared by all toolbars + menus; bk color = `ThemeSysColor(COLOR_BTNFACE)` |

## Precedence rule (Dark theme, per icon)

```
dark override SVG  >  standard SVG auto-adapted  >  legacy raster (028 transform)
```

Default theme: `standard SVG verbatim > legacy raster verbatim` (unchanged).

## Lifecycle / state transitions

- Built at startup (`InitializeGraphics`) and rebuilt wholesale on every
  theme switch (`SetThemeMode` → `ColorsChanged`) and system color/depth
  change. No caching across switches → no stale-variant states possible.
- Repeated switching allocates/frees the same set of GDI objects through
  the existing HANDLES-audited paths (leak detection active in Debug).

## Validation rules

- Override file must be a parseable SVG; unreadable/missing → silent
  fallback down the precedence chain (TRACE_I only), a button can never
  end up empty because of an override.
- Auto-adaptation touches only plain-color paints; alpha preserved.
- Adapted neutral colors must clear ≥3:1 contrast vs dark COLOR_BTNFACE
  (unit-tested bound, SC-002).
