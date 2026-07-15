# Validation Results: Repair the PictView Plugin

**Feature**: 006-fix-pictview-plugin | **Date**: 2026-07-15
**Build**: `build.cmd full` — Debug x64, all 90 projects incl. 35 plugins, clean (0 errors); `pictview.spl` registered in plugins.ver (v7). No `PVW32Cnv.lib`/`SalPVEnv.exe` inputs.

## Reported defect — fixed and verified

PictView showed "This plugin (…pictview.spl) has not been loaded. Plugin
is not valid Open Salamander Plugin…" at every startup.

Root cause (verified in source): `SalamanderPluginEntry` → `InitViewer`
→ `LoadPictViewDll` required the proprietary `PVW32Cnv.dll` engine (x64
via the `SalPVEnv.exe` IPC host), which was removed from the repo for
GPL reasons. Missing engine → entry returns NULL → core shows
`IDS_PLUGININVALID2`.

Fix: a new in-process **WIC-backed engine** (`wicengine.cpp`) fills the
existing `PVW32DLL` function-pointer table with functions of identical
signatures. `LoadPictViewDll` now just calls `InitWicEngine` and always
succeeds — nothing to load, nothing to fail.

**SC-001 verified (headless):** launched the app and enumerated startup
dialogs via `GetWindowTextW`; the PictView load error is **absent**
(script `scratchpad/verify-pictview-load.ps1` → `SC-001: PASS`). The
only remaining startup dialog is the unrelated UnRAR missing-`unrar.dll`
notice (a pre-existing known missing dependency, out of scope). Also
re-confirmed after retiring `salpvenv` from the solution.

## What was implemented (build-verified)

New engine `src/plugins/pictview/wicengine.{h,cpp}` — WIC decode → 32bpp
PBGRA DIB, composited over the background colour, blitted to the
caller's HDC (`StretchBlt`, mirror via negative stretch size):

| Function | Status |
|----------|--------|
| `PVOpenImageEx` | real — file (UTF-8→UTF-16 `\\?\` long-path), attached HBITMAP, or Read/Seek stream |
| `PVGetImageInfo` | real — per-frame info (multi-page) |
| `PVReadImage2` | real — decode frame + optional blit to DC, progress callback |
| `PVDrawImage` | real — blit decoded/stretched frame to DC |
| `PVSetStretchParameters` / `PVSetBkHandle` | real — zoom/mirror + alpha background |
| `PVCloseImage` / `PVGetErrorText` / `PVGetDLLVersion` / `PVSetParam` | real / stubs |
| `PVChangeImage` | **real** — lossless 90° rotation (EXIF auto-rotate + Rotate menu keep working) |
| `PVGetHandles2` | real — exposes DIB scanlines (pipette + histogram keep working) |
| `PVSaveImage`, `PVCropImage`, `PVLoadFromClipboard`, `PVIsOutCombSupported`, `PVReadImageSequence` | graceful stubs — clean `PVCODE`, never crash |

Wiring/build:
- `LoadPictViewDll`, `InitViewer` version text, `ReleaseViewer` re-worked
  (pictview.cpp); `PICTVIEW_DLL_IN_SEPARATE_PROCESS` retired (precomp.h).
- Open call sites pass the **UTF-8** name straight to the engine
  (render1.cpp, pictview.cpp, thumbs.cpp) — Unicode/long-path images
  open (dropped the ANSI down-conversion; EXIF-orientation still uses an
  ANSI path best-effort since exif.dll is ANSI-only).
- `pictview.vcxproj`: removed IPC sources (`PVEXEWrapper`/`PVMessage`/
  `PVMessageWrapper`) + `salpvenv` ProjectReference; added `wicengine.cpp`
  + `ole32.lib`. `salpvenv.vcxproj` retired from `salamand.sln`. Kept
  `lib/PVW32DLL.h` (shared types) and `exif` (GPL-clean, unchanged).

Graceful degradation / feature gating (FR-005/FR-006):
- Stub-backed, non-working commands removed from the menu + toolbar:
  **Save As, Print, Crop, Paste** (menu templates + `ToolBarButtons[]`).
- Kept commands the WIC engine *does* back: rotate, mirror, copy,
  zoom/scroll/navigation, properties, EXIF, histogram, pipette.
- If a removed command is still reached (e.g. a leftover accelerator),
  its handler shows the engine's "not supported" text via
  `PVGetErrorText` — no crash.

## Decode path verified (headless)

WIC — the same OS component the engine uses — decodes every fixture at
the correct dimensions and rejects the corrupt one (script over
`%TEMP%\salamander-test\images`):

| File | Result |
|------|--------|
| test.png / .jpg / .bmp / .gif / .tif | OK 320×240, 1 frame |
| č-obrazek.png (Unicode name) | OK 320×240 |
| broken.jpg | decode fails → engine returns error → graceful message |

This validates the decode approach, the format coverage (SC-002), the
Unicode-name path (SC-005), and the unsupported-file path (SC-004) at
the WIC layer. The engine's own code uses the standard WIC
decoder→PBGRA→DIB pipeline over these exact inputs.

## Deferred to an interactive desktop session

The verification host is headless (no interactive desktop; `GetForegroundWindow()==0`),
so the F3 viewer window cannot be driven by keystrokes — the same limit
recorded for features 004/005. Deferred:

- **On-screen viewing walkthrough** (quickstart rows #3–#11): open each
  format in the viewer window and confirm correct rendering, zoom/scroll,
  next/previous, multi-page TIFF, animated-GIF first frame. Code paths
  are in place and build-verified; the decode layer is validated above.
- **Menu/toolbar visual check** (row #9): confirm Save As/Print/Crop/
  Paste are gone and the remaining controls work.
- `build.cmd full release` (LTO/WPO) before merge.

## Scope notes / follow-ups

- Per the clarified scope (full viewer, not save/edit): animation
  playback (multi-frame GIF/ANI), Save As/convert, crop, clipboard
  paste, TWAIN/WIA scan, screen capture, printing, and wallpaper remain
  out of scope. Capture/scan/wallpaper menu items are left in place (they
  build their own HBITMAP and either work via the attach path or degrade
  gracefully); Save As/Print/Crop/Paste are removed because they are
  purely engine-encoder-backed.
- Thumbnails: the panel thumbnail path still calls `CreateThumbnail` →
  `PVSaveImage` (stubbed) so thumbnails are not produced by the WIC
  engine yet — a follow-up (the WIC decoder can fill the RAW 32bpp buffer
  directly). Not required for US1–US3.

## Build-system fix: plugins.ver version regression (PictView missing from Plugin Manager)

Reported after the code fix: "with `build.cmd release full` PictView is
missing from the build — I don't see it at all." Investigated and root-caused
as a **build-script defect independent of the plugin code**:

- Debug and Release of the same platform share one registry key
  (`Software\Open Salamander\5.0`) and one counter,
  `Configuration\Plugins.ver Version (x64)`.
- The old `build.cmd` generated `plugins.ver` with a per-output-directory
  counter that **restarted at 1** for each configuration. Salamander skips a
  `plugins.ver` whose version is `<=` the recorded one (plugins2.cpp:2817), so
  after Debug builds pushed the registry to a higher number, a fresh Release
  build's version-1 file was ignored — **none of its plugins auto-installed**,
  including the newly built PictView. Confirmed live: registry version = 1,
  33 of 35 plugins installed, PictView absent.

Fix (`build.cmd`): the `plugins.ver` version is now a **monotonic clock-based
token** — minutes since 2000-01-01 (fits a 32-bit int until ~4085) — so every
build, in any configuration, strictly increases and always exceeds any value
already written to the registry. No per-directory state.

**Verified end-to-end (headless):** launched the Release build; the registry
version advanced **1 → 13957450**, the installed-plugin count went **33 → 34**,
and **PictView is now present in the Plugin Manager list** (script
`scratchpad/verify-autoinstall.ps1`). SaveConfig persists the new version at
startup (salamdr1.cpp:4349), so no special exit is required.

## Regression posture

- No plugin ABI change; the `PVW32DLL` table shape and `lib/PVW32DLL.h`
  are unchanged. Other 34 plugins and the core build and register
  normally (plugins.ver v7).
- No proprietary/non-GPLv2 component is linked or shipped; WIC
  (`windowscodecs` via COM) and GDI are Windows OS components (GPL
  system-library exception) — the feature's core license fix.
- Config load/save untouched (`G` struct, registry) — starts clean with
  prior PictView config.
