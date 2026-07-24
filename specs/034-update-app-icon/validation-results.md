# Validation Results: Update Application Icon to Revised Artwork

**Feature**: 034-update-app-icon | **Date**: 2026-07-24
**Build**: `build.cmd` → Debug x64, BUILD SUCCEEDED (twice: after ICO
regeneration, after `logo.svg` replacement); `setup.vcxproj` and
`remove.vcxproj` built individually (Debug x64); `salmon.exe` built within
the solution
**Method**: adopted the revised rasters, regenerated all ICOs with the
(unchanged) 033 packer, structurally verified every ICO plus per-frame
corner transparency, rasterized the shipped `logo.svg` through the repo's
own nanosvg (the exact renderer About/splash use) and compared pixels
against the delivered master render, then launched the app headlessly and
captured splash, About (light + dark), main window (light + dark), and the
`-i 0..3` title-bar icon variants via `PrintWindow`. The user's existing
`HKCU\Software\Newt Commander` configuration was exported before the GUI
runs and re-imported afterwards (test processes were killed, never allowed
to save config).

Screenshots are in [`screenshots/`](screenshots/).

## Success criteria

| ID | Criterion | Result | Evidence |
|----|-----------|--------|----------|
| SC-001 | OS-shell surfaces show the revised icon, no artifacts, no tile remnant | **PASS** | `salamand.ico` packs 16/24/32/48/64/128/256 px 32-bpp (`--verify` OK); every frame of every shipped ICO has fully transparent corner pixels (decoded-frame check); `SHDefExtractIcon` on the built `newtcommander.exe` returns the tile-less folder at 16/32/48/256 px (`exe-icons.png`, row 1); window caption shows it live (`titlebar-variants.png`, row 1); taskbar/Alt+Tab consume the same `IDI_SALAMANDER` resource |
| SC-002 | About + splash marks faithful to master, light + dark | **PASS** | `about-light.png`, `about-dark.png`, `splash.png` show the revised mark, legible on white and navy backgrounds; the repo-nanosvg harness renders `logo.svg` with an exact silhouette (0 pixels painted outside the master's coverage) and interior color delta mean 1.42, p95 = 4, max = 5 per channel vs `png/newt-commander-icon-256.png` (soft drop shadow intentionally absent per spec edge case; residual ≤5 delta is gradient-LUT quantization in the low-opacity shade band) |
| SC-003 | Zero surfaces still show the 033 tile icon or older art | **PASS** | All 7 shipped icon files regenerated + `logo.svg` replaced; consumer sweep of `src/**/*.rc*` confirms the app icon is referenced only by `salamand.rc2`, `salmon.rc`, `setup.rc`, `remove.rc2` (the `sfx7zip/setup.ico` generic-installer art and `zip/selfextr/icon.ico` SFX-archive art are file-type icons, out of scope per spec assumption, unchanged since before 032); extracted icons of built `salmon.exe`/`setup.exe`/`remove.exe` show the revised design (`exe-icons.png`) |
| SC-004 | Four window-icon choices selectable and distinguishable | **PASS** | `titlebar-variants.png`: default orange / red / green / blue captured live via `-i 0..3` at caption size — all four instantly distinguishable; the 033 hue-remap tuning transferred without adjustment (papers/cream pill stayed neutral at 16+32 px) |
| SC-005 | Regeneration reproduces shipped assets | **PASS** | `python tools\brand\gen_icons.py` run twice back-to-back → SHA256-identical outputs for all 7 ICOs; `--verify` passes; packer consumes only the committed `tools/brand/png/` renders |
| SC-006 | Icon recognizable at 16 px on light and dark | **PASS** | 16 px frame clearly reads as the orange folder (`exe-icons.png` left column, `titlebar-variants.png` on both light captions and the dark About/main-window captures); delivered 16/24/32 px renders shipped verbatim per spec assumption |

## Design-delta findings

- **Tile-less silhouette**: the icon's outline is now the folder shape; all
  ICO frames and both in-app surfaces were explicitly checked for tile
  remnants/halos (corner-alpha check, About/splash screenshots) — none.
- **`logo.svg` simplification**: only the `feDropShadow` was dropped; unlike
  033 no tile reconstruction is needed, so the shipped SVG is smaller than
  before. The master's non-uniform group scale (`scale(1.28 1.38)`) and the
  `rotate(±4°)` document transforms render correctly through the bundled
  nanosvg (silhouette pixel-exact vs the master render).
- **Delivered PNGs verified against SVG math**: the 1024 px render's folder
  gradient matches the master SVG's gradient + gloss + shade composition
  analytically (sampled profile), confirming the PNG set and the SVG master
  describe the same artwork.

## Notes / scope boundaries confirmed

- **No code changes**: the diff is assets only (`.ico`, `.svg`) plus doc
  text in `tools/brand/` (`gen_icons.py` docstring, `README.md`); resource
  IDs, file names, `.rc`/`.vcxproj` files, and C++ sources untouched
  (FR-010, plan structure decision).
- **Splash**: still paints on the always-navy brand background
  (`logo.cpp:208`), so the tile-less orange folder keeps contrast; the mark
  bleeds off the right edge by design (032/033 brand language). Captured at
  ~716 ms after process start (`splash.png`).
- **Companion outputs**: `setup.exe`/`remove.exe` are not part of the
  default `build.cmd` solution filter; built directly from their projects
  for verification. The benign pre-existing `pwsh.exe not recognized`
  post-build warning appeared again (unrelated to icons).
- **512/1024 px renders** committed under `tools/brand/png/` for future
  use, not packed into any ICO.
- **User registry preserved**: the pre-existing user configuration was
  backed up (`reg export`) and restored (`reg delete` + `reg import`) after
  the GUI smoke; test instances were force-killed so no settings (theme
  switches, panel paths) leaked into the user's config.
- **Icon cache**: fresh shell extraction shows the revised icon; stale
  Explorer caches on upgraded machines are expected Windows behavior
  (quickstart.md note).
