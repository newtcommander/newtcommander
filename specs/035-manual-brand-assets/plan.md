# Implementation Plan: Manual Brand Asset Replacement

**Branch**: `035-manual-brand-assets` | **Date**: 2026-07-25 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/035-manual-brand-assets/spec.md`

## Summary

Make every brand graphic hand-swappable without AI assistance: (1) rework
`tools/brand/gen_icons.py` so all shipped `.ico` files are derived from **one
master PNG** with optional per-size override PNGs (Lanczos downscale), and
drop the red/green/blue hue-remap outputs; (2) remove the red/green/blue
main-window icon variants as a product feature (config combo, resources,
`sal_r/g/b.ico`); (3) replace the hand-authored nanosvg-constrained
`src/res/logo.svg` (About + splash artwork) with a plain **PNG resource**
decoded at draw time via WIC and alpha-blended with preserved aspect ratio;
(4) split the splash copyright into two lines (Open Salamander part / Newt
Commander part) by adding a second static line to `IDD_SPLASH` and drawing
two strings, leaving `VERSINFO_COPYRIGHT` (VERSIONINFO block) untouched;
(5) rewrite `tools/brand/README.md` as the complete self-service asset guide.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022); Python 3 + Pillow for the asset packer (dev-machine only, not a build dependency)
**Primary Dependencies**: Pure WinAPI; WIC (`windowscodecs.lib`, inbox on Windows 11) for PNG decode — precedent: pictview WIC engine (feature 006); existing `CSVGSprite`/nanosvg stays for toolbar icons and the gradient accent strips
**Storage**: Committed binary assets in `src/res/`, `src/salmon/res/`, `src/setup/res/`, `src/setup/remove/`; source art in `tools/brand/`; registry value `Main window icon index` (HKCU) becomes inert
**Testing**: `gen_icons.py --verify` structural ICO check; Debug x64 build via `build.cmd`; manual run verification (splash, About, Explorer icon) per quickstart
**Target Platform**: Windows 11+, x64
**Project Type**: Desktop application (existing monolithic WinAPI app + asset tooling)
**Performance Goals**: Splash must not open measurably slower — PNG decode of a ~512 px image via WIC is single-digit ms (was: nanosvg rasterization)
**Constraints**: No new external dependencies; GPLv2-compatible only; generated `.ico` committed (build reproducibility — build never runs Python); UTF-8-BOM sources; resource scripts stay MSVC-rc compatible
**Scale/Scope**: ~10 source files touched, 1 Python tool rewritten, 4 `.rc`/`.rh` files, 3 deleted `.ico`, 1 new committed PNG asset, 1 README rewrite

## Constitution Check

*GATE: evaluated pre-Phase 0 and re-checked post-Phase 1 — PASS (one justified deprecation, see Complexity Tracking).*

| Principle | Verdict | Notes |
|-----------|---------|-------|
| I. Build Reproducibility | PASS | `.ico`/`.png` assets stay committed; `gen_icons.py` remains an authoring-time tool. The build pipeline is unchanged and never invokes Python. |
| II. Backward Compatibility | PASS (justified deprecation) | Removing the red/green/blue window-icon variants is a user-visible regression, explicitly requested and recorded in the spec (Clarifications 2026-07-25, FR-003). Saved configs referencing a variant silently fall back to the default icon; the registry value is still read/clamped, never crashes. Documented in Complexity Tracking. |
| III. Incremental Modernization | PASS | Four independent, individually revertible slices (packer, variant removal, PNG artwork, splash copyright). No adjacent refactoring. |
| IV. Windows Platform Commitment | PASS | WIC is an inbox Windows component; `windowscodecs.lib` links from the Windows SDK. No new third-party code. |
| V. Plugin Architecture Preservation | PASS | Plugins untouched. |
| VI. UI Consistency | PASS | `IDD_SPLASH` keeps `DIALOGEX` + `DS_SETFONT \| DS_FIXEDSYS` + `FONT 8, "MS Shell Dlg"`; only statics move/appear. Config page loses one groupbox row — standard controls only. |

## Project Structure

### Documentation (this feature)

```text
specs/035-manual-brand-assets/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   ├── asset-layout.md  # Replaceable-file contract (paths, formats, consumers)
│   └── gen-icons-cli.md # Regeneration tool CLI contract
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (repository root)

```text
tools/brand/
├── icon-master.png          # NEW: required icon source (1024×1024)
├── icon-16.png … icon-256.png  # NEW: optional per-size overrides (adopted from png/ set)
├── about.png                # NEW: About/splash artwork source (copied to src/res/logo.png)
├── gen_icons.py             # REWORK: master+override model, no hue-remap, validation
├── README.md                # REWRITE: the self-service asset guide (FR-008)
├── png/                     # REMOVED (files renamed into overrides above)
└── logo.svg                 # REMOVED (nanosvg-constrained artwork retired)

src/
├── res/
│   ├── salamand.ico         # regenerated (master+overrides)
│   ├── sal_r.ico sal_g.ico sal_b.ico   # DELETED
│   ├── logo.svg             # DELETED
│   └── logo.png             # NEW: RCDATA artwork for About + splash
├── salamand.rc2             # IDB_LOGO_IMAGE RCDATA "res\\logo.png"; drop IDI_SALAMANDER_RED/GREEN/BLUE + IDB_LOGO_HAND
├── salamand.rc              # IDD_SPLASH: +IDC_SPLASH_COPYRIGHT2, height 94→104, status shifted
├── salamand.rh              # +IDC_SPLASH_COPYRIGHT2
├── resource.rh2             # drop IDI_SALAMANDER_* variant IDs + IDB_LOGO_HAND; +IDB_LOGO_IMAGE
├── versinfo.rh2             # +VERSINFO_COPYRIGHT1/2 (display split); VERSINFO_COPYRIGHT unchanged
├── logo.cpp                 # PNG loader (WIC) instead of svgHand; 2-line copyright paint
├── pngimage.cpp / pngimage.h  # NEW: LoadPngResource → premultiplied 32-bpp DIB (WIC)
├── cfgdlg.h                 # MainWindowIcons shrink to 1 entry
├── dialogs4.cpp             # GetMainWindowIconIndex clamps to the single entry
├── dialogs5.cpp             # remove variant table entries + config combo fill/transfer code
├── lang/lang.rc             # remove "Main Window Icon" groupbox/combo from IDD_CFGPAGE_MAINWINDOW
├── lang/lang.rh             # remove IDC_TITLEBAR_ICON_INDEX (+ freed statics)
└── vcxproj/salamand.vcxproj(.filters)  # drop sal_r/g/b.ico Image items; +pngimage.cpp/logo.png
```

**Structure Decision**: Existing monolithic layout is kept; the only new
code is a small self-contained PNG-resource helper (`src/pngimage.*`)
consumed by `logo.cpp`. All asset sources consolidate directly under
`tools/brand/` with flat, predictable file names — the guide can say
"replace this file" with no directory archaeology.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Principle II deviation: red/green/blue main-window icon variants removed (user-visible feature loss) | Maintainer decision (spec Clarifications 2026-07-25): variants would require hue-remap tuning per new artwork, defeating the "swap one file by hand" goal | Keeping auto-derivation: silently produces wrong/ugly variants for artwork without an orange band; keeping manual variants: triples the artwork burden per swap — both undermine the feature's core purpose |
