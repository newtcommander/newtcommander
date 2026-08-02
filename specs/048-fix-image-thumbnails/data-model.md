# Data Model: Restore Image Thumbnails

**Feature**: 048-fix-image-thumbnails · **Date**: 2026-08-02

No persistent data changes. The model below describes the in-memory
pixel flow and the states that govern it.

## 1. Pixel source — `CWicImage` (existing, `wicengine.cpp:83-105`)

| Field | Role in this feature |
|-------|---------------------|
| `HDib` / `DibBits` | 32bpp **top-down** DIB, opaque BGRX (alpha already composited over `BkColor`); the sole pixel source for export |
| `Width`, `Height` | Natural decoded dimensions; define row stride (`Width * 4`) and total export size |
| `DecodedFrame` | Cache key: export decodes only if the requested frame is not already held |
| `BkColor` | Panel background (`G.rgbPanelBackground`), set by `LoadThumbnail` via `PVSetBkHandle` *before* export — transparent images composite correctly |

Invariant: after a successful `DecodeFrame`, `DibBits` holds exactly
`Height` rows of `Width * 4` bytes, row 0 = top of image.

## 2. Export request — accepted `PVSaveImageInfo` subset

| Field | Accepted value | Meaning |
|-------|---------------|---------|
| `Format` | `PVF_RAW` | Raw pixel export, no container |
| `Colors` | `PV_COLOR_TC32` | 32bpp BGRX rows (Windows DIB layout) |
| `Flags` | must contain `PVSF_USERDEFINED_OUTPUT`; may contain `PVSF_SUPERFAST` (ignored hint), `PVSF_FLIP_VERT` (emit rows bottom-up) | |
| `WriteFunc` | non-NULL | Receives row batches; short write = stop |
| `SeekFunc` | ignored | Legacy artifact; RAW export is sequential |
| `Width`, `Height` | 0 | Natural size only (scaling not in subset) |
| `CropLeft/Top/Width/Height` | 0 | No cropping in subset |

Any other combination → `PVC_UNSUP_OUT_PARAMS` (unchanged behavior for
Save As, print preview, batch JPEG thumbnails).

## 3. Consumer — `CSalamanderThumbnailMaker` (existing, core, untouched)

State machine driven by the exported rows:

```text
Clear() ──SetParameters(picW, picH, flags)──▶ receiving
  receiving ──ProcessBuffer(rows)──▶ receiving        (NextLine < OriginalHeight, returns TRUE)
  receiving ──ProcessBuffer(final rows)──▶ complete   (NextLine == OriginalHeight, returns FALSE)
  receiving ──ProcessBuffer after cancel──▶ aborted   (returns FALSE)
  any ──SetError()──▶ error
```

- `ThumbnailReady()` == `complete` and only then does the icon cache
  accept the thumbnail (flag 4 → 5/6); `aborted`/`error`/partial keep
  flag 4 → the panel paints the plain icon (FR-003's silent fallback).
- `MyWriteFunc` (`thumbs.cpp:120-125`) is the adapter: WriteFunc bytes →
  `ProcessBuffer(pData, Size / bytesperline)`; returns `Size` to
  continue, `0` to stop. **A `0` return is the success path on the final
  batch** (see research.md D3).

## 4. Result — icon cache entry (existing, core, untouched)

| Flag | Meaning | Painted as |
|------|---------|-----------|
| 4 | thumbnail not loaded / rejected | shell icon (current defect state for all images) |
| 5 | thumbnail ready | thumbnail bitmap |
| 6 | old/lower-quality thumbnail | thumbnail bitmap (refresh queued) |

The fix's observable success = image files transition 4 → 5/6 again.

## 5. Orientation flags (pass-through, unchanged)

`pictureFlags` (SSTHUMB_*) computed in `LoadThumbnail:896-977` from
`pvii.Flags`. The WIC engine reports `Flags = 0`, so no mirror/rotate
transforms trigger and rows are consumed as delivered (top-down). The
EXIF branch stays dormant by design (research.md D4).
