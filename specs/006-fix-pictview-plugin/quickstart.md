# Quickstart: Repair the PictView Plugin

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

How to build and verify that PictView loads without the proprietary
engine and displays common image formats.

## Prerequisites

- Windows 11, Visual Studio 2022 with C++ Desktop workload
- Repo cloned, `OPENSAL_BUILD_DIR` set (optional; defaults to `.\build\`)
- No `PVW32Cnv.dll` / `SalPVEnv.exe` present (the normal state) — the
  whole point is to work without them

## Build

```batch
build.cmd full           :: complete Debug x64 build incl. plugins + runtime data
```

Expected: `pictview.spl` is produced and registered (plugins.ver) with
**no** `salpvenv`/`PVW32Cnv.lib` input. `salpvenv.vcxproj` is no longer
in the solution.

## Test fixtures

```powershell
$img = "$env:TEMP\salamander-test\images"
New-Item -ItemType Directory -Force $img | Out-Null
# Create one file per format from a generated bitmap (uses .NET/WIC-independent GDI+):
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap 320,240
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::CornflowerBlue)
$g.FillEllipse([System.Drawing.Brushes]::Gold, 40,40,240,160)
$g.Dispose()
$bmp.Save("$img\test.png",[System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Save("$img\test.jpg",[System.Drawing.Imaging.ImageFormat]::Jpeg)
$bmp.Save("$img\test.bmp",[System.Drawing.Imaging.ImageFormat]::Bmp)
$bmp.Save("$img\test.gif",[System.Drawing.Imaging.ImageFormat]::Gif)
$bmp.Save("$img\test.tif",[System.Drawing.Imaging.ImageFormat]::Tiff)
$bmp.Dispose()
# Unicode + long-path name (feature 004/005 territory):
Copy-Item "$img\test.png" "$img\$([char]0x010D)-obrázek.png" -Force  # č-obrázek.png (create dir first if needed)
# A deliberately unsupported/corrupt file:
Set-Content -LiteralPath "$img\broken.jpg" -Value "not really a jpeg" -Encoding ascii
```

## Verification walkthrough

| # | Action | Expected | SC |
|---|--------|----------|----|
| 1 | Start Salamander | **No** "PictView not loaded" error box | SC-001 |
| 2 | Plugins menu → Plugin Manager | PictView listed as loaded/enabled, viewer association present | SC-001 |
| 3 | Focus `test.jpg`, press F3 (or Ctrl+PgDn into it) | Image displays correctly (compare with Windows Photos) | SC-002 |
| 4 | Repeat for `test.png`, `test.bmp`, `test.gif`, `test.tif` | Each renders at correct size/colour | SC-002 |
| 5 | In the viewer: zoom fit / 1:1 / in / out; scroll/pan | View updates correctly, no crash | SC-003 |
| 6 | Next/previous image in the folder | Adjacent image loads | SC-003 |
| 7 | Open `č-obrázek.png` (and one at a long path) | Opens; correct name in title/status | SC-005 |
| 8 | Open `broken.jpg` | Clear "unsupported/cannot display" message; viewer stays responsive | SC-004 |
| 9 | Check the viewer menu/toolbar | Save As, edit (crop/rotate/mirror), scan, capture, print, paste are hidden/disabled — not broken | SC-004 |
| 10 | Open a multi-page TIFF (if available) | First page shows; page-next works or is absent without error | SC-002 |
| 11 | Open an animated GIF | At least the first frame renders | SC-002 |
| 12 | Restart Salamander (config round-trip) | Loads fine; no error; prior config honored | SC-006 |
| 13 | Full build + other plugins/core | Everything builds and runs; no regression | SC-006 |

## Headless / CI-friendly checks

- **SC-001 without a desktop**: launch the app, enumerate top-level
  windows for a `#32770` titled with the PictView error text — assert
  it is **absent** (inverse of the feature-005 notice check); optionally
  confirm `plugins.ver` lists pictview. This is scriptable in the
  headless environment.
- **US2 decode correctness without F3**: a tiny host that calls the WIC
  engine's `PVOpenImageEx`/`PVReadImage2(NULL DC)` on each fixture and
  checks the returned `PVImageInfo.Width/Height` and a few decoded
  pixels against expected values (the DIB is directly inspectable),
  covering formats headless. Full on-screen viewing is the desktop
  matrix above.

## Build sanity

```batch
build.cmd full release   :: LTO/WPO clean build before merge
```

Confirm the output plugins directory contains `pictview.spl` and
`exif.dll` and does **not** require `PVW32Cnv.dll` or `SalPVEnv.exe`.
