# Implementation Plan: Update Application Icon to Revised Artwork

**Branch**: `034-update-app-icon` | **Date**: 2026-07-24 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/034-update-app-icon/spec.md`

## Summary

Repeat the feature 033 icon swap with the revised artwork delivered in
gitignored `temp/icon/` (master SVG + pre-rendered PNG set 16–1024 px). The
design delta: the dark navy rounded tile is **gone** — the icon is now the
orange folder with documents alone, enlarged (`scale(1.28 1.38)` vs the old
`1.16`), on a fully transparent background. The folder/papers/pill palette is
unchanged.

Surfaces to update (identical list to 033):

- `src/res/salamand.ico` (exe / main window / taskbar / Alt+Tab, 7 sizes),
- `src/res/sal_r.ico`, `sal_g.ico`, `sal_b.ico` (red/green/blue window-icon
  variants, 16+32 px),
- `src/res/logo.svg` (`IDB_LOGO_HAND` RCDATA — About dialog + splash screen),
- `src/salmon/res/salmon.ico`, `src/setup/res/setup.ico`,
  `src/setup/remove/icon1.ico` (crash reporter, installer, uninstaller).

Technical approach: the 033 pipeline is **reused as-is** — `gen_icons.py` is
already a pure PNG→ICO packer whose inputs are the committed rasters in
`tools/brand/png/`, so adopting the new PNG set and re-running it regenerates
all six ICOs (the hue-remap tuning survives because the folder palette is
identical). The only authored asset is a new nanosvg-safe `logo.svg`
derivative of the revised master (drop `feDropShadow`, fix `width`/`height`
to the 256 viewBox, `userSpaceOnUse` gradients — same constraints as 033;
simpler now, since no tile/`clipPath` reconstruction is needed). All shipped
file names and resource IDs stay unchanged, so **no `.rc`/`.vcxproj`/C++
changes** (asset-only diff again). Delivered PNGs verified: correct sizes,
RGBA, fully transparent corners/edges.

## Technical Context

**Language/Version**: C++ (C++20, MSVC v143) for the app — no code changes
expected; Python 3 + Pillow for the asset pipeline (asset-authoring time
only, NOT a build dependency — generated files are committed)
**Primary Dependencies**: nanosvg-based `CSVGSprite` (in-app rendering of
`IDB_LOGO_HAND`); Win32 ICON resources; existing PNG→ICO packer
`tools/brand/gen_icons.py` (reused unchanged except doc text)
**Storage**: N/A (static assets committed to the repository)
**Testing**: `python tools\brand\gen_icons.py --verify` (ICO structural
check); `build.cmd` (Debug x64); headless GUI smoke via `-l`/`-r` +
`WM_COMMAND` to open About with screenshot capture (SendInput is blocked);
Explorer visual check of exe icons
**Target Platform**: Windows 11+
**Project Type**: Desktop application — visual asset replacement, no
behavioral change
**Performance Goals**: no startup regression — the new `logo.svg` is a
*smaller* nanosvg document than 033's (tile rects removed, same folder group)
**Constraints**: `logo.svg` must stay inside the bundled nanosvg subset
(no `filter`/`feDropShadow`/`clipPath`/`mask`; `width`/`height` MUST equal
the viewBox size; gradients `userSpaceOnUse` — see `tools/brand/README.md`);
ICO convention 32-bpp, BMP-encoded ≤ 64 px, PNG-encoded ≥ 128 px; shipped
file names and resource IDs must not change; splash always paints on the
brand navy background (`logo.cpp:208`) and About must stay legible in both
themes now that the mark has no own background tile
**Scale/Scope**: 6 regenerated `.ico` files, 1 re-authored shipped `.svg`,
replaced master SVG + 9 PNGs in `tools/brand/`, doc-text updates in
`gen_icons.py`/`README.md`, 0 C++ / resource-script changes

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Verdict | Notes |
|-----------|---------|-------|
| I. Build Reproducibility | PASS | All shipped assets stay committed; regeneration remains one documented command (`python tools\brand\gen_icons.py`); Pillow stays authoring-time-only. |
| II. Backward Compatibility | PASS | Pure visual asset swap under the Newt Commander 0.1.0 identity; red/green/blue window-icon options keep working; no registry/IPC/plugin surface touched. |
| III. Incremental Modernization | PASS | Small, independently revertible, asset-only change; no adjacent refactoring. |
| IV. Windows Platform Commitment | PASS | No new dependencies; pure WinAPI resources. |
| V. Plugin Architecture Preservation | PASS | Plugins untouched. |
| VI. UI Consistency | PASS | No dialog/control changes; the mark artwork changes application-wide as one deliberate decision. |

**Post-Phase-1 re-check**: PASS — design introduces no new projects,
dependencies, or code paths; contracts not applicable (no external interface
changes).

## Project Structure

### Documentation (this feature)

```text
specs/034-update-app-icon/
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
├── newt-commander-icon.svg           # REPLACED: revised master SVG (from temp/icon/)
├── png/
│   └── newt-commander-icon-{16,24,32,48,64,128,256,512,1024}.png
│                                     # REPLACED: revised authoritative rasters
├── logo.svg                          # RE-AUTHORED: nanosvg-safe variant (no tile)
├── gen_icons.py                      # UNCHANGED logic; docstring text refreshed
├── README.md                         # UPDATED: design description + palette
├── newt-commander-lockup-*.svg       # unchanged (wordmark lockups)
└── gradient-band.svg                 # unchanged (accent strip)

src/res/
├── salamand.ico                      # regenerated: 16,24,32,48,64,128,256 (32bpp)
├── sal_r.ico / sal_g.ico / sal_b.ico # regenerated: 16+32, hue-remapped folder
└── logo.svg                          # replaced: copy of tools/brand/logo.svg

src/salmon/res/salmon.ico             # regenerated
src/setup/res/setup.ico               # regenerated
src/setup/remove/icon1.ico            # regenerated
```

**Structure Decision**: Single project, in-place replacement of committed
binary/vector assets, exactly as in 033. Upstream file names preserved so no
`.rc2`/`.rc`/`.vcxproj` change is needed. Consumers (verified unchanged since
033): `src/salamand.rc2` (19–22, 48), `src/salmon/salmon.rc` (9),
`src/setup/setup.rc` (73), `src/setup/remove/remove.rc2` (13),
`src/logo.cpp` (splash ~218 — navy background, About ~445 — theme
background), `src/dialogs5.cpp` (`MainWindowIcons[]`, ~2685).

## Complexity Tracking

No constitution violations — table not applicable.
