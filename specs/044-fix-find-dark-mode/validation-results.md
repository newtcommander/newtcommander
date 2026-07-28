# Validation Results: Fix Find Window Dark-Mode Rendering

**Feature**: 044-fix-find-dark-mode · **Date**: 2026-07-28
**Binary under test**: Debug x64 + Release x64 built from this branch.
Evidence screenshots in `temp/` (gitignored): `dark_after_*.png`,
`light_after_*.png`, `light_baseline_*.png`, `light_release_after_*.png`.
Reference defect capture: `temp/dark_find_window.png`.

## Build & test gates

| Gate | Result |
|---|---|
| `build.cmd` (Debug x64) | **BUILD SUCCEEDED** |
| `build.cmd full release` (Release x64) | **BUILD SUCCEEDED** (19 plugins, 180 language modules) |
| saltests (incl. new `TestFindDarkModeSurfaces`) | **1133 checks, 0 failed** |
| clang-format on all touched files | applied, builds green afterwards |

Note: builds must be invoked through PowerShell (`cmd /c` from the Git
Bash tool mangles `/c` into `C:\` and silently does nothing).

## SC-001 — dark walkthrough, per-defect pixel evidence

Automated captures of the Debug build (`Theme Mode = 1` in the registry,
window driven via `WM_COMMAND`), measured at the exact coordinates where
the defects were measured before the fix:

| Surface | Before (measured) | After (measured) | Verdict |
|---|---|---|---|
| Separator under menu bar (400,54) | 255,255,255 | **16,16,16** (dark bevel) | fixed |
| Separator beside "Search file content" (500,154) | 255,255,255 | **16,16,16** | fixed |
| Status bar body (400,510) | 240,240,240 | **45,45,45** with 240-gray text | fixed |
| Advanced-box frame (95,186) | 236–251 white | **45,45,45** (dark `DarkMode_CFD` frame) | fixed |
| Results area (430,470) | — | 32,32,32 (`COLOR_WINDOW`, unchanged) | no regression |

Visual review of the captures additionally confirms: "Found Items: (N)"
light text; header labels ("Name"/"Path"/"Size"/"Date"/"Time"/"Attr")
light on the dark header; disabled "Focus" caption flat readable gray;
dark edge above the results list; dark size grip.

States covered: initial window, "Search file content" expanded
(`dark_after_find_expanded.png` — revealed fields all dark/readable),
running search over `C:\Windows` (`dark_after_find_searching_long.png` —
owner-drawn "Searching: <path>" light-on-dark, disabled controls
readable), completed search with results and selection summary
(`dark_after_find_results.png`).

## SC-002 — contrast

Asserted programmatically in saltests (`TestFindDarkModeSurfaces`):
`COLOR_BTNTEXT`(240) on `COLOR_BTNFACE`(45) ≥ 4.5:1 (actual ≈ 11.6:1);
`COLOR_GRAYTEXT`(150) on `COLOR_BTNFACE` ≥ 3:1 (actual ≈ 4.6:1); bevel
pair distinct and dark; progress bar/track distinct.

## SC-003 — light-mode regression (zero visual differences)

Apples-to-apples comparison: Release binary built **before** the change
(preserved as `temp/baseline_release_newtcommander.exe`) vs Release
binary built from this branch, same machine, same session, Default
theme, full-resolution pixel diff of the Find window (882×528):

- **0 differing pixels in the entire client area.**
- 211 differing pixels confined to DWM title-bar rows y=10–19 —
  instance-to-instance caption rendering noise (present between any two
  launches), not app drawing.

A Debug-vs-Release comparison showed small differences in disabled
toolbar icon rasterization (y≈210–225); the Release-vs-Release diff
proves these are build-flavor rendering variance, not this feature.

## SC-004 / FR-006 / FR-008 — live switching and multi-instance

Scripted stress on the Debug build, Find window open throughout:

- 10 consecutive Default ↔ Dark switches (via `CM_THEME_*` to the main
  window), including one Dark → Default → Dark cycle **while a search
  was running**: no crash, window valid, process alive, final capture
  fully dark with no half-themed element.
- Mid-search switch to Default captured
  (`light_after_find_midsearch_switch.png`): every fixed surface
  (separators, status bar, advanced box, header) reverted to the native
  light rendering — the un-darkening path works.
- Second simultaneous Find window opened and captured
  (`dark_after_find_second_instance.png`): renders identically dark.

## Items verified by code path, not by scripted GUI run

- **Windows High Contrast (FR-007)**: not toggled on the test machine
  (system-wide accessibility setting; toggling it would disrupt the
  user's session). Guarantee is structural: every new paint path is
  gated on `IsDarkThemeActive()`, which returns FALSE under High
  Contrast (`themes.cpp`, unchanged 028 logic), making all fixes
  passthrough — identical to the verified Default-theme behavior.
- **Progress bar during duplicate search (R9)**: the plain search
  completes without showing the progress child; it appears only for
  duplicate-content searches. The dark coloring
  (`UpdateProgressBarTheme`) uses the documented classic-renderer
  `PBM_SETBKCOLOR`/`PBM_SETBARCOLOR` technique, is applied at creation
  and re-applied in `OnColorsChange`, and the Default branch restores
  the native theme. Recommended one-time manual check: Find duplicates
  over a large folder in the Dark theme.

## Side-effect surfaces (spot-checked)

- Find window etched separators fixed by the same central code that now
  covers every dialog's `SS_ETCHED*` static (Find Settings/Advanced
  dialogs included by construction — same subclass, same enum pass).
- `src/packac.cpp` status bar inherits the central subclass (themed the
  next time its dialog's `ThemeApplyToDialog` runs while it exists).
- Disabled toolbar items app-wide now draw flat gray in dark (shared
  `CToolBar` code), emboss preserved in light — visible in the light
  regression diff as zero change.
