# Research: Theme-Adaptive Toolbar Icons

**Feature**: 029-dark-toolbar-icons | **Date**: 2026-07-21

All findings were established by direct code inspection in this repository
(no external unknowns). Detailed pipeline map with file:line references is
in [analysis-toolbar-icons.md](analysis-toolbar-icons.md).

## R1. Where toolbar icons come from

- **Decision basis**: `CreateToolbarBitmaps()` (src/toolbar4.cpp:730)
  builds the shared image lists at startup: loads the legacy raster sheet
  (`IDB_TOOLBAR_16` .bmp / `IDB_TOOLBAR_256` .png selected by screen BPP),
  DPI-scales it, stamps per-button SVG glyphs over it
  (`RenderSVGImages`, toolbar4.cpp:709; files read from `<exe>\toolbars\`
  at runtime via nanosvg — src/svg.cpp:92-156), appends shell32 icons,
  then derives the gray/disabled variant + mask
  (`CreateGrayscaleAndMaskBitmaps_tmp`, toolbar4.cpp:937).
- The SVGs in `src\res\toolbars\` are **live runtime assets** deployed by
  `src\vcxproj\!populate_build_dir.cmd:115-116` (robocopy, top-level files
  only — no subdirectories).

## R2. What feature 028 already does (and the gap)

- `ThemeAdjustBitmapForDarkMode()` (src/themes.cpp:373-467) remaps the
  **legacy raster sheet only**, and runs **before** SVG compositing
  (toolbar4.cpp:770 + explicit comment). SVG glyphs and shell icons are
  untouched → the visible dark-theme defect.
- Enabled-state SVG shapes render in source colors; only the disabled
  branch recolors (svg.cpp:128-138, theme-aware via `GetSVGSysColor`).
- Image-list backgrounds already follow `ThemeSysColor(COLOR_BTNFACE)`
  (salamdr1.cpp:2537-2538).

## R3. Live theme switching — no extra work needed

- **Decision**: rely on the existing rebuild pipeline.
- **Rationale**: `SetThemeMode()` (themes.cpp:529) →
  `ColorsChanged(TRUE, FALSE, TRUE)` (salamdr1.cpp:3100) →
  `ReleaseGraphics()` + `InitializeGraphics()` → the toolbar image lists
  (salamdr1.cpp:2400-2538) are recreated from scratch, and every toolbar
  gets `OnColorsChanged()` (mainwnd1.cpp:3122-3214, finddlg2.cpp:440,
  stswnd.cpp:2431). Any theme-dependent logic inside
  `RenderSVGImage`/`CreateToolbarBitmaps` is therefore automatically
  re-evaluated on switch — FR-005 satisfied by construction.
- **Alternatives considered**: dedicated icon-rebuild hook — rejected,
  redundant with the existing pipeline.

## R4. Chosen adaptation strategy (ties to spec clarifications)

- **Decision**: automatic per-shape SVG recolor in dark mode (variant B of
  the analysis) + per-icon hand-tuned override files (variant C mechanism),
  legacy raster keeps the existing 028 pixel transform (variant A already
  in place).
- **Rationale**: covers 100% of buttons with zero new mandatory assets;
  vector-quality result; preserves color accents (clarification #2:
  "keep colors — lighten only dark/neutral strokes"); override gives full
  artistic control where automation falls short (clarification #1).
- **Alternatives considered**:
  - Post-composite bitmap transform for everything — rejected: the
    heuristic would also distort shell32 object icons unless compositing
    order is reworked, and raster-level adaptation loses vector quality.
  - Complete hand-authored dark set now (~63 files) — rejected by
    clarification #1 (incremental refinement instead).
  - Monochrome restyle — rejected by clarification #2.

## R5. Color math to reuse

- **Decision**: extract the exact per-color rules from
  `ThemeAdjustBitmapForDarkMode` into a pure inline helper in
  `src/common/themes_palette.h`:
  neutral (`max-min < 32`) with `max < 140` → `v = 220 - max*80/140`
  (monotonic [0,140) → (140,220]); saturated `max < 120` → channels scaled
  by `170/max` (hue preserved); otherwise unchanged.
- **Rationale**: single source of truth; identical visual language between
  legacy raster glyphs and SVG glyphs in dark mode; the header is already
  shared with saltests (feature 028 pattern) → directly unit-testable,
  including the SC-002 3:1 contrast bound against dark `COLOR_BTNFACE`
  RGB(45,45,45).
- **nanosvg color format**: `NSVGpaint.color` is 0xAABBGGRR (ABGR; see
  `GetSVGSysColor`, svg.cpp:35-45). The adapter converts ABGR→RGB→ABGR and
  preserves the alpha byte; only `NSVG_PAINT_COLOR` paints are touched
  (gradients don't occur in the shipped icon set).

## R6. Known asset defects to fix

- `src\res\toolbars\CilpboardCut.svg` is misspelled; the button table uses
  `"ClipboardCut"` (toolbar4.cpp:182) → silent fallback to raster glyph.
  Fix by renaming the file (git mv). Stale `CilpboardCut.svg` may remain
  in existing build outputs — harmless (never referenced).
- Many buttons have no SVG at all (Select/Unselect/…): out of scope to
  author them; they stay on the raster path which already gets the 028
  dark transform (FR-007 satisfied).

## R7. Deploy of the override directory

- **Decision**: add a second `:mycopy_dir` call for
  `..\res\toolbars\dark` in `!populate_build_dir.cmd` (same helper and
  error-handling semantics as the existing toolbars line) and anchor the
  directory in git with `README.txt` describing the override contract.
- **Rationale**: `:mycopy_dir` robocopy is non-recursive, so the parent
  copy alone would never deploy the subdirectory; an explicit line keeps
  the change minimal and consistent. The README guarantees the source dir
  exists (git cannot track empty dirs) and gives maintainers the contract
  at the point of use.
- **Alternatives considered**: switching the parent copy to `robocopy /E`
  — rejected: `:mycopy_dir` is shared with other callers; changing shared
  semantics for one feature violates the incremental principle.
