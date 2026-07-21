# Implementation Plan: Theme-Adaptive Toolbar Icons (Dark Icon Set)

**Branch**: `029-dark-toolbar-icons` | **Date**: 2026-07-21 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/029-dark-toolbar-icons/spec.md`

## Summary

In the Dark theme (feature 028) the toolbar button glyphs still render in
their light-theme colors: the SVG glyphs that are stamped over the legacy
raster sheet at startup are never recolored, so dark outlines/fills sink
into the dark background. Fix: (1) extract the per-color dark-adaptation
math of `ThemeAdjustBitmapForDarkMode()` into a pure shared helper,
(2) apply it to SVG shape colors at rasterization time when the Dark theme
is active — hues of colored accents are preserved, dark/neutral strokes are
lightened, (3) add a per-icon override: `toolbars\dark\<Name>.svg`, when
present, is used verbatim in the Dark theme instead of the auto-adapted
standard SVG, (4) fix the `CilpboardCut.svg` filename typo so the Cut icon
uses the SVG path at all, (5) deploy the `dark` override directory in the
build. Default theme renders through unchanged code paths. Live theme
switching already rebuilds all toolbar bitmaps (`SetThemeMode` →
`ColorsChanged` → `ReleaseGraphics`/`InitializeGraphics`), so no extra
switching work is needed. Pure color math gets saltests coverage incl. the
3:1 contrast criterion (SC-002).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI; nanosvg (embedded, `src/common/dep/nanosvg`) for SVG parse/rasterize; existing theme engine (`src/themes.*`, `src/common/themes_palette.h`)
**Storage**: N/A (no config change; icon variant keyed off `Configuration.ThemeMode`)
**Testing**: `src/saltests` console exe (CHECK macro harness, exit code = failures), currently 482 checks
**Target Platform**: Windows 11+, x64 (x86 also builds)
**Project Type**: Desktop app (monolithic exe + plugins); changes confined to main app + build data deploy
**Performance Goals**: Icon preparation happens at startup and on theme switch only; added per-shape color math is O(shapes) — imperceptible (<1 ms total for ~63 SVGs)
**Constraints**: Default theme must be pixel-identical (dark-only code paths); no new user configuration; no new external dependencies
**Scale/Scope**: ~63 SVG assets, 1 shared header, ~4 source files touched, 1 build script line, +1 saltests suite

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Assessment |
|-----------|------------|
| I. Build Reproducibility | PASS — deploy of `toolbars\dark\` added to the existing automated `!populate_build_dir.cmd` step; no manual steps. |
| II. Backward Compatibility | PASS — all new behavior is gated on `IsDarkThemeActive()`; Default theme renders through untouched code paths. The `CilpboardCut.svg` rename restores intended (SVG) rendering for Cut in both themes — a bug fix, visually the same motif. |
| III. Incremental Modernization | PASS — small, isolated change set; the refactor extracts existing math into a shared helper without altering it; no adjacent refactoring. |
| IV. Windows Platform Commitment | PASS — pure WinAPI + embedded nanosvg; no new dependencies. |
| V. Plugin Architecture Preservation | PASS — plugin icon paths untouched (explicitly out of scope per spec FR-008). |
| VI. UI Consistency | PASS — application-wide visual change implemented centrally in the theme/icon pipeline, not per-module. |

*Post-design re-check (after Phase 1): unchanged — PASS on all six.*

## Project Structure

### Documentation (this feature)

```text
specs/029-dark-toolbar-icons/
├── spec.md                       # Feature specification (clarified)
├── analysis-toolbar-icons.md     # Czech technical analysis (user request)
├── plan.md                       # This file
├── research.md                   # Phase 0 output
├── data-model.md                 # Phase 1 output
├── quickstart.md                 # Phase 1 output
├── contracts/
│   └── dark-icon-override.md     # Per-icon dark override contract
└── tasks.md                      # Phase 2 output (/speckit-tasks)
```

### Source Code (repository root)

```text
src/
├── common/
│   └── themes_palette.h          # + ThemeDarkAdaptColor() pure inline helper
├── themes.cpp                    # ThemeAdjustBitmapForDarkMode() refactored onto helper
├── svg.cpp                       # RenderSVGImage(): dark override load + auto-adaptation
├── res/toolbars/
│   ├── ClipboardCut.svg          # renamed from CilpboardCut.svg (typo fix)
│   └── dark/
│       └── README.txt            # override mechanism doc (also anchors dir for deploy)
├── vcxproj/
│   └── !populate_build_dir.cmd   # + deploy of res\toolbars\dark → output toolbars\dark
└── saltests/
    └── saltests.cpp              # + TestDarkIconColorAdaptation()
```

**Structure Decision**: All changes live in the main application and shared
`src/common` header — no project-file changes are needed (svg.cpp, themes.cpp,
saltests.cpp already compile; `themes_palette.h` is already on both include
paths). One build-data deploy line is added to the existing populate script.

## Design

### D1. Shared pure color helper (new)

`src/common/themes_palette.h` gains a pure, WinAPI-free inline function with
exactly the math currently inlined in `ThemeAdjustBitmapForDarkMode()`
(src/themes.cpp:435-453):

```c
// Adapt one light-surface RGB color for dark surfaces.
// - neutral (max-min < 32) with max < 140: invert darkness,
//   [0,140) -> (140,220] monotonically (black outlines become light)
// - saturated with max < 120: brighten toward the same hue (max -> 170)
// - anything already light/bright: unchanged
inline void ThemeDarkAdaptColor(int* r, int* g, int* b);
```

`ThemeAdjustBitmapForDarkMode()` is refactored to call it per pixel
(alpha un-premultiply/re-premultiply and transparent-key handling stay in
themes.cpp — they are bitmap concerns). Behavior is bit-identical.

### D2. SVG dark adaptation + override (svg.cpp)

`RenderSVGImage()` changes, all inside `IsDarkThemeActive()`:

1. **Override probe**: build `<exe>\toolbars\dark\<name>.svg` and try
   `ReadSVGFile()` first. If it loads, rasterize it verbatim (author has
   full control; no auto recolor).
2. **Auto-adaptation**: if no override, load the standard SVG and, for the
   enabled state, walk `image->shapes`; for every shape whose fill/stroke
   paint type is a plain color, convert nanosvg ABGR → RGB, run
   `ThemeDarkAdaptColor()`, convert back (alpha byte preserved).
   The existing disabled-state recolor (COLOR_BTNSHADOW, already
   theme-aware via `GetSVGSysColor`) stays as is.

In the Default theme the function is byte-for-byte on its current path.
Legacy raster fallback buttons keep the existing (028) bitmap transform at
`toolbar4.cpp:770` — every button therefore gets a dark treatment (FR-007).
Live switching needs no work: `SetThemeMode()` already funnels into
`InitializeGraphics()` which re-runs `CreateToolbarBitmaps()`.

### D3. Asset fixes and deploy

- `git mv src\res\toolbars\CilpboardCut.svg ClipboardCut.svg` — the button
  table references `"ClipboardCut"` (toolbar4.cpp:182); today the miss
  silently falls back to the raster glyph.
- New `src\res\toolbars\dark\README.txt` documents the override contract
  (see contracts/dark-icon-override.md) and anchors the directory in git.
- `!populate_build_dir.cmd`: after the existing toolbars copy (line 116),
  add `call :mycopy_dir ..\res\toolbars\dark "...\toolbars\dark\"`.
  No hand-tuned dark SVG is required to ship (clarification #1); the
  README makes the robocopy call well-defined.

### D4. Tests (saltests)

New `TestDarkIconColorAdaptation()` (registered in `main()`):

- black/dark neutrals map into (140,220]; pure black → 220; monotonic
  ordering preserved on a sweep of [0,140)
- light neutrals (max ≥ 140) and white unchanged
- dark saturated colors: max channel → 170, channel ratios (hue) preserved
- bright saturated colors unchanged (e.g., folder yellow)
- adapted result of every neutral input has ≥ 3:1 contrast against dark
  `COLOR_BTNFACE` (45,45,45) — SC-002 executable check
- helper is pure: same input → same output (no global state)

## Phase 0: Research

All unknowns were resolved by direct code inspection (this session) and are
recorded in [research.md](research.md). No NEEDS CLARIFICATION remain.

## Phase 1: Design & Contracts

- [data-model.md](data-model.md) — icon asset kinds, image-list states and
  their theme-dependent derivation
- [contracts/dark-icon-override.md](contracts/dark-icon-override.md) — the
  per-icon override file contract (naming, location, precedence, fallback)
- [quickstart.md](quickstart.md) — build, verify, and how to add a
  hand-tuned dark icon
- Agent context updated via `.specify/scripts/bash/update-agent-context.sh claude`

## Complexity Tracking

No constitution violations — table not needed.
