# Validation Results: Replace Application Icon

**Feature**: 033-replace-app-icon | **Date**: 2026-07-24
**Build**: `build.cmd` → Debug x64, BUILD SUCCEEDED; `salmon.vcxproj`,
`setup.vcxproj`, `remove.vcxproj` built individually (Debug x64)
**Method**: regenerated assets with the rewritten packer, structurally
verified every ICO, rasterized the shipped `logo.svg` through the repo's own
nanosvg (the exact renderer About/splash use) and compared pixels against
the master render, then launched the app headlessly and captured splash,
About (light + dark), and the `-i 0..3` title-bar icon variants via
`PrintWindow`. Verification runs created `HKCU\Software\Newt Commander`
(key did not exist before) — removed afterwards to restore pre-test state.

Screenshots are in [`screenshots/`](screenshots/).

## Success criteria

| ID | Criterion | Result | Evidence |
|----|-----------|--------|----------|
| SC-001 | OS-shell surfaces show the new icon, no artifacts | **PASS** | `salamand.ico` packs 16/24/32/48/64/128/256 px 32-bpp (structural `--verify` OK); `ExtractAssociatedIcon` on the built `newtcommander.exe` returns the new folder tile; window caption shows it live (`titlebar-variants.png`, row 1); taskbar/Alt+Tab consume the same `IDI_SALAMANDER` resource |
| SC-002 | About + splash tiles faithful to master, light + dark | **PASS** | `about-light.png`, `about-dark.png`, `splash.png` all show the new tile; the repo-nanosvg harness renders `logo.svg` within Δ≤3/channel of `png/newt-commander-icon-256.png` at 256 px (soft drop shadow intentionally absent per spec edge case) |
| SC-003 | Zero surfaces still show old "Split Disc" / Open Salamander art | **PASS** | All 7 shipped icon files regenerated (`salamand.ico`, `sal_r/g/b.ico`, `salmon.ico`, `setup.ico`, `icon1.ico`) + `logo.svg` replaced; consumer sweep of `src/**/*.rc*` confirms no other app-icon reference; extracted icons of built `salmon.exe`/`setup.exe`/`remove.exe` show the new tile (these still carried the **original Open Salamander** icon before this feature) |
| SC-004 | Four window-icon choices selectable and distinguishable | **PASS** | `titlebar-variants.png`: default orange / red / green / blue captured live via `-i 0..3` at caption size — all four instantly distinguishable; the Configuration → Main Window combo loads the same `IDI_SALAMANDER[_RED/_GREEN/_BLUE]` resources (`dialogs5.cpp` `MainWindowIcons[]`) |
| SC-005 | Regeneration reproduces shipped assets | **PASS** | `python tools\brand\gen_icons.py` run twice back-to-back → byte-identical outputs (git diff stable); `--verify` structural check passes for all 7 ICOs; packer consumes only the committed `tools/brand/png/` renders |
| SC-006 | Icon recognizable at 16 px | **PASS** | 16 px frame (dark rounded tile + orange folder) clearly reads as the Newt Commander tile; delivered 16/24/32 px renders shipped verbatim per spec assumption |

## Renderer finding (pre-existing bug, fixed by asset authoring)

The bundled nanosvg (`src/common/dep/nanosvg`) mishandles SVG gradients when
the root `width`/`height` attributes differ from the viewBox size (shapes are
rescaled, gradient transforms are not), and renders unitless
objectBoundingBox gradient coords flat. **The 032 `logo.svg` shipped with
both traits — its plate gradients had always rendered flat (last-stop
color).** The new `logo.svg` uses `width/height == viewBox` and
`gradientUnits="userSpaceOnUse"`, so the About/splash tile now renders its
gradients correctly — verified by a pixel comparison through the repo's own
nanosvg + rasterizer. Constraints documented in `tools/brand/README.md`.

## Notes / scope boundaries confirmed

- **No code changes**: the diff is assets only (`.ico`, `.svg`) plus the
  brand pipeline (`tools/brand/`); resource IDs, file names, `.rc`/`.vcxproj`
  files, and C++ sources untouched (FR-010, plan structure decision).
- **Splash capture**: the splash is a `#32770` dialog (420×152, ~300 ms
  lifetime) — captured by polling `PrintWindow` during startup. Its tile
  bleeds off the right edge by design (same 032 brand language as About).
- **Companion outputs**: `setup.exe`/`remove.exe` are not part of the
  default `build.cmd` solution filter; built directly from their projects
  for verification. A benign pre-existing `pwsh.exe not recognized`
  post-build warning appears in salmon/setup builds (unrelated to icons).
- **512/1024 px renders** committed under `tools/brand/png/` for future use,
  not packed into any ICO (Windows uses ≤ 256 px).
- **Icon cache**: fresh Explorer views show the new icon; stale caches on
  upgraded machines are expected Windows behavior (quickstart.md note).
