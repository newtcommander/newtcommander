# Quickstart: Validating Restored Image Thumbnails

**Feature**: 048-fix-image-thumbnails · **Date**: 2026-08-02

## Build

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd full          :: Debug x64 + runtime data + plugins.ver
```

Run `%OPENSAL_BUILD_DIR%\salamand\Debug_x64\tandemcommander.exe`
(default `OPENSAL_BUILD_DIR` is `.\build\` if unset).

## Test data

Create a folder (e.g. `%TEMP%\thumbtest`) containing:

1. `photo.jpg` — any JPEG photo (ideally > thumbnail size)
2. `graphic.png` — PNG **with transparency** (checks panel-background compositing)
3. `anim.gif` — animated GIF (checks multi-frame → frame 0)
4. `image.bmp` — BMP
5. `scan.tif` — TIFF
6. `icon.ico` — ICO
7. `big.jpg` — a large photo (≥ 12 MP) for responsiveness
8. `corrupt.jpg` — a JPEG with the second half truncated
9. `fake.jpg` — a text file renamed to `.jpg`
10. `empty.png` — zero-byte file
11. `notes.txt` — plain non-image file
12. 100+ photos in a subfolder `many\` (copy of a photo collection) for
    the responsiveness scenario

PowerShell one-liner for the corrupt file:
`$b=[IO.File]::ReadAllBytes('photo.jpg'); [IO.File]::WriteAllBytes('corrupt.jpg', $b[0..([int]($b.Length/2))])`

## Scenarios

### S1 — Previews appear (US1 / SC-001)  [PRIMARY]

1. Navigate a panel to the test folder; press **Alt+5**.
2. **Expect**: `photo.jpg`, `graphic.png`, `anim.gif`, `image.bmp`,
   `scan.tif`, `icon.ico`, `big.jpg` show image content previews, aspect
   ratio preserved. `graphic.png` transparency shows the panel
   background color, not black.
3. Navigate away and back — previews reappear (cache path).

### S2 — Silent fallback (US2 / SC-003)

1. Same folder, thumbnail view.
2. **Expect**: `fake.jpg`, `empty.png`, `notes.txt` show standard icons;
   `corrupt.jpg` shows either a partial-image-free icon (decode rejected)
   — **no error dialog at any point**, panel keeps browsing normally.

### S3 — Responsiveness (US3 / SC-002)

1. Open `many\` (100+ photos), press Alt+5.
2. **Expect**: panel is immediately scrollable/navigable; previews fill
   in progressively; leaving the folder mid-load is instant (no freeze,
   no stale thumbnails in the next folder).

### S4 — Second panel + view switching

1. Both panels on the test folder, both Alt+5. **Expect**: previews in
   both, no flicker/conflicts.
2. Toggle Alt+3 (details) → Alt+5. **Expect**: previews return.

### S5 — Thumbnail size configuration (FR-006)

1. Options → Configuration → Panels: change thumbnail size (e.g. 48 →
   96), OK.
2. **Expect**: thumbnails re-render at the new size.

### S6 — Fresh-config first run (FR-002 / SC-001)

1. Export & delete `HKCU\Software\Tandem Commander\0.1` (or run on a
   machine/user without it).
2. Start the app, Alt+5 on the test folder.
3. **Expect**: previews appear with zero manual configuration (pictview
   auto-registered via `plugins.ver`).

### S7 — Regression guard: unrelated PVSaveImage callers

1. Open `photo.jpg` in the internal viewer (Enter / F3 → PictView).
2. **Expect**: viewer displays the image as before; **Save As** remains
   disabled/unsupported exactly as before this feature; **Print** dialog
   preview behaves exactly as before this feature (unchanged).
3. Viewer still zooms/rotates (Ctrl+L/Ctrl+R) correctly.

### S8 — Parity spot-check (SC-004)

If an Open Salamander 5.0 installation is available: same folder,
thumbnail view side-by-side — same set of files gets previews (minus
EXIF rotation, deliberately out of scope, and formats WIC lacks codecs
for, e.g. MNG/CDR — see plan.md Scope Notes).

## Diagnostic aids

- **Plugins Manager** (Plugins → Plugin Manager → PictView): the
  "Thumbnails" field must list the mask string (`*.jpg;*.png;...`).
  If it shows "none", the defect is registration, not pixel production.
- Debug build TRACE output shows `Unable to use plugin ... as thumbnail
  loader.` (load failure) or WIC decode `TRACE_E` lines.
- `MaxThumbImgSize` (PictView config, default megapixel cap) legitimately
  excludes extremely large images from thumbnailing — not a defect.
