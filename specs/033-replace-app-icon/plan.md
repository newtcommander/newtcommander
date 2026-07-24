# Implementation Plan: Replace Application Icon

**Branch**: `033-replace-app-icon` | **Date**: 2026-07-24 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/033-replace-app-icon/spec.md`

## Summary

Replace the feature 032 "Split Disc — Extruded" icon with the newly prepared
folder-tile artwork (master SVG + pre-rendered PNG set 16–1024 px, delivered
in gitignored `temp/icon/`) on every surface that shows the application icon:

- `src/res/salamand.ico` (exe / main window / taskbar / Alt+Tab, 7 sizes),
- `src/res/sal_r.ico`, `sal_g.ico`, `sal_b.ico` (red/green/blue window-icon
  variants selectable in Configuration → Main Window, 16+32 px),
- `src/res/logo.svg` (`IDB_LOGO_HAND` RCDATA, the icon tile drawn by the
  nanosvg-based `CSVGSprite` on the splash screen and in the About dialog),
- `src/salmon/res/salmon.ico`, `src/setup/res/setup.ico`,
  `src/setup/remove/icon1.ico` (crash reporter, installer, uninstaller —
  still carrying the original Open Salamander icon).

Technical approach: adopt the new artwork into `tools/brand/` as the source
of truth (master SVG + committed PNG renders), rewrite `gen_icons.py` from a
procedural redrawing rasterizer into a **PNG→ICO packer** (the new artwork
uses SVG filters that cannot be reproduced procedurally; the delivered PNGs
are authoritative), generate the red/green/blue variants by hue-remapping the
saturated orange folder pixels, and hand-author a nanosvg-safe `logo.svg`
variant (no `feDropShadow`, no `clipPath`). All shipped file names and
resource IDs stay unchanged, so **no `.rc`/`.vcxproj`/C++ changes are
needed** (asset-only change; `logo.cpp` scales the square tile by aspect).

## Technical Context

**Language/Version**: C++ (C++20, MSVC v143) for the app — no code changes
expected; Python 3 + Pillow 12.1 for the asset pipeline (asset-authoring
time only, NOT a build dependency — generated files are committed)
**Primary Dependencies**: nanosvg-based `CSVGSprite` (in-app SVG rendering of
`IDB_LOGO_HAND`); Win32 ICON resources; existing `write_ico` packer logic in
`tools/brand/gen_icons.py`
**Storage**: N/A (static assets committed to the repository)
**Testing**: `build.cmd` (Debug x64); Python structural verification of ICO
contents (entry count/sizes/bit depth); headless GUI smoke via `-l`/`-r` +
`WM_COMMAND` to open the About dialog with screenshot capture; Explorer
visual check of exe icons
**Target Platform**: Windows 11+
**Project Type**: Desktop application — visual asset replacement, no
behavioral change
**Performance Goals**: no startup regression — splash SVG stays a small flat
document parsed by nanosvg (comparable node count to the current one)
**Constraints**: `logo.svg` must stay inside the nanosvg feature subset
(linear/radial gradients and shape transforms OK; filters, `clipPath`,
masks NOT supported); ICO entries 32-bpp, BMP-encoded ≤ 64 px and
PNG-encoded ≥ 128 px (current convention); shipped file names and resource
IDs must not change (constitution: no piecemeal identity churn, minimal diff)
**Scale/Scope**: 6 regenerated `.ico` files, 1 replaced shipped `.svg`,
1 rewritten generator script, brand assets + README in `tools/brand/`,
0 C++ / resource-script changes

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Verdict | Notes |
|-----------|---------|-------|
| I. Build Reproducibility | PASS | All shipped assets are committed; regeneration is one documented command (`python tools\brand\gen_icons.py`); Pillow remains an authoring-time-only dependency exactly as established by feature 032. |
| II. Backward Compatibility | PASS | Pure visual asset swap under the Newt Commander 0.1.0 identity; the red/green/blue window-icon option keeps working; no registry/IPC/plugin surface touched. |
| III. Incremental Modernization | PASS | Small, independently revertible change; no adjacent refactoring; no C++ code modified. |
| IV. Windows Platform Commitment | PASS | No new runtime dependencies; pure WinAPI resources. |
| V. Plugin Architecture Preservation | PASS | Plugins untouched. |
| VI. UI Consistency | PASS | No dialog/control changes; About/splash keep their layout — only the tile artwork changes, applied application-wide as one deliberate decision. |

**Post-Phase-1 re-check**: PASS — design introduces no new projects,
dependencies, or code paths; contracts section not applicable (no external
interface changes).

## Project Structure

### Documentation (this feature)

```text
specs/033-replace-app-icon/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output (asset inventory & mapping)
├── quickstart.md        # Phase 1 output (regenerate + verify)
├── checklists/
│   └── requirements.md  # Spec quality checklist (pass)
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (repository root)

```text
tools/brand/                          # brand-asset source of truth
├── newt-commander-icon.svg           # REPLACED: new master SVG (from temp/icon/)
├── png/
│   └── newt-commander-icon-{16,24,32,48,64,128,256,512,1024}.png
│                                     # NEW: authoritative rasters (committed)
├── logo.svg                          # REPLACED: nanosvg-safe variant of new icon
├── gen_icons.py                      # REWRITTEN: PNG→ICO packer + RGB hue-remap
├── README.md                         # UPDATED: new pipeline + palette
├── newt-commander-lockup-*.svg       # unchanged (wordmark lockups)
└── gradient-band.svg                 # unchanged (accent strip)

src/res/
├── salamand.ico                      # regenerated: 16,24,32,48,64,128,256 (32bpp)
├── sal_r.ico / sal_g.ico / sal_b.ico # regenerated: 16+32, hue-remapped folder
└── logo.svg                          # replaced: copy of tools/brand/logo.svg

src/salmon/res/salmon.ico             # regenerated (was original Open Salamander art)
src/setup/res/setup.ico               # regenerated (was original Open Salamander art)
src/setup/remove/icon1.ico            # regenerated (was original Open Salamander art)
```

**Structure Decision**: Single project, in-place replacement of committed
binary/vector assets. Upstream file names are deliberately preserved
(`salamand.ico`, `sal_*.ico`, `logo.svg`, `salmon.ico`, `setup.ico`,
`icon1.ico`) so that no `.rc2`/`.rc`/`.vcxproj` file changes and the diff
stays asset-only. Consumers verified: `src/salamand.rc2` (lines 19–22, 48),
`src/salmon/salmon.rc` (line 9), `src/setup/setup.rc` (line 73),
`src/setup/remove/remove.rc2` (line 13), `src/logo.cpp` (splash line ~218,
About line ~445), `src/dialogs5.cpp` (`MainWindowIcons[]`, line ~2685).

## Complexity Tracking

No constitution violations — table not applicable.
