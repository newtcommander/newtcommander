# Phase 0 Research: Restore Image Thumbnails

**Feature**: 048-fix-image-thumbnails · **Date**: 2026-08-02

Root-cause evidence lives in
[research-thumbnail-chain.md](research-thumbnail-chain.md); this file
records the design decisions derived from it. No NEEDS CLARIFICATION
markers existed in the spec; the decisions below resolve the remaining
implementation choices.

## D1: Where to restore pixel production

**Decision**: Implement the missing capability in the engine —
`WicSaveImage` (`wicengine.cpp`) gets a real implementation for the
RAW/user-defined-output subset. The thumbnail flow in `thumbs.cpp`
(`LoadThumbnail` → `CreateThumbnail` → `PVW32DLL.PVSaveImage`) stays
structurally untouched.

**Rationale**:
- This is the exact follow-up feature 006's validation report anticipated
  ("the WIC decoder can fill the RAW 32bpp buffer directly").
- `CreateThumbnail` (`thumbs.cpp:1026-1043`) already does everything else
  right: it calls `thumbMaker->SetParameters(imgWidth, imgHeight, flags)`
  with the *original* dimensions — the core's `CShrinkImage` does the
  shrinking, so the engine only needs to export natural-size rows.
- The engine owns the decoded DIB (`CWicImage.DibBits`, 32bpp top-down
  BGRX composited over `BkColor`); exporting from there requires no new
  decode path and no copies.

**Alternatives considered**:
- *Bypass `PVSaveImage` in `thumbs.cpp` using `PVGetHandles2` row
  pointers*: works, but moves engine knowledge into the caller, leaves
  the stub advertising a capability gap it no longer has, and spreads the
  fix across the very flow that is already correct.
- *New dedicated engine entry point (e.g. `WicExportRows`)*: changes the
  internal function-table surface for no benefit over implementing the
  entry that all existing callers already use.

## D2: Return-code honesty in LoadThumbnail

**Decision**: `CPluginInterfaceForThumbLoader::LoadThumbnail` returns
`code == PVC_OK` instead of unconditional `TRUE` (`thumbs.cpp:1023`).

**Rationale**: The unconditional `TRUE` makes the icon reader break out
of the loader chain (`fileswn1.cpp:914`) believing the thumbnail was
delivered — the mechanism that made this regression silent. Returning
FALSE on failure matches the documented interface contract ("pokud vrati
FALSE, Salamander zkusi nacist thumbnail pomoci jineho pluginu") and
makes any future engine failure visible as a fallthrough instead of a
mystery.

**Alternatives considered**: also calling `thumbMaker->SetError()` on
decode errors — rejected as unnecessary: the core's `ThumbnailReady()`
gate already discards incomplete thumbnails, and `SetError` would only
duplicate that signal.

## D3: Completion semantics of the write loop

**Decision**: `WicSaveImage` treats a short write from `WriteFunc` as
normal termination and returns `PVC_OK` for it. Real failures (decode
error, invalid parameters, OOM) return their real codes.

**Rationale**: `MyWriteFunc` (`thumbs.cpp:120-125`) returns
`ProcessBuffer(...) * Size`, and `ProcessBuffer` returns FALSE both on
cancellation **and on successful completion of the final rows**
("tvorba thumbnailu je hotova (byl zpracovan cely obrazek)",
`spl_thum.h:59-62`). A short write on the last chunk is therefore the
*success* path, and cancellation is indistinguishable from it at the
engine level — the core's `ThumbnailReady()` gate is the arbiter. Any
other mapping (e.g. returning `PVC_CANCELED` on short write) would make
D2's `code == PVC_OK` return FALSE on every successful thumbnail.

## D4: EXIF orientation is out of scope

**Decision**: Do not restore EXIF-based rotation of thumbnails in this
feature.

**Rationale**: The orientation branch (`thumbs.cpp:923`) is gated on
`PVFF_EXIF`, which the WIC engine never sets (`FillInfo` reports
`Flags = 0`). The viewer's auto-rotate (`render1.cpp:1646`) and the EXIF
menu enabler (`pictview.cpp:2309`) are gated on the same flag — setting
it for thumbnails would re-activate viewer paths far beyond this
feature's scope and test surface. Keeping it unset keeps thumbnails
consistent with what the viewer displays today. Product-wide EXIF
restoration is a separate follow-up.

**Alternatives considered**: setting `PVFF_EXIF` for JPEG in `FillInfo`
(blast radius into viewer behavior); applying orientation inside the
engine's decode (would double-rotate if the viewer's auto-rotate is
later restored).

## D5: Chunked delivery and cancellation cadence

**Decision**: Stream the DIB to `WriteFunc` in bounded row batches
(target ~256 KB per call, minimum 1 row), checking the `Progress`
callback between batches (`Progress` returns **TRUE to cancel** — the
polarity documented at `wicengine.cpp:445-449` and fixed in commit
`61c0dab`).

**Rationale**: `spl_thum.h:64-69` requires larger images to be delivered
in parts with the `ProcessBuffer` return value checked, because a path
change in the panel blocks until the loader yields. One giant write would
honor the letter of the API but defeat cancellation. Chunking from the
already-decoded DIB costs nothing (no copies — `WriteFunc` reads the DIB
memory directly).

## D6: Accepted `PVSaveImageInfo` subset (guards)

**Decision**: `WicSaveImage` accepts a request only when ALL hold:
`pSii->Format == PVF_RAW`, `pSii->Colors == PV_COLOR_TC32`,
`pSii->Flags & PVSF_USERDEFINED_OUTPUT` with a non-NULL `WriteFunc`,
requested `Width`/`Height` are 0 (natural size), and
`CropWidth`/`CropHeight` are 0 (no crop). `PVSF_SUPERFAST` is accepted
and ignored (decode-speed hint); `PVSF_FLIP_VERT` is accepted and honored
by emitting rows bottom-up (trivial, keeps the streamed-RAW contract
complete). Everything else — file-format encoding (Save As, batch JPEG
thumbnails), scaling/cropping (print preview) — returns
`PVC_UNSUP_OUT_PARAMS` exactly as today.

**Rationale**: The thumbnail path (`thumbs.cpp:978-1016`) requests
precisely this subset (`memset`-zeroed `sii`, RAW + TC32 + user output;
`PVSF_SUPERFAST` for ≥8× oversize; `PVSF_FLIP_VERT` only behind
`PVFF_BOTTOMTOTOP`, which the WIC engine never reports). The guards make
"no behavior change outside thumbnails" a structural property rather
than a testing obligation — `print.cpp:78-79` always requests scaling
(`sii.Width/Height` = preview size), so printing cannot slip through.

**Byte-order note**: RAW `PV_COLOR_TC32` output is Windows 32bpp DIB
layout (B,G,R,X per pixel) — proven by `print.cpp:63-81`, which pours the
same output straight into a `CreateDIBSection` buffer. `CWicImage.DibBits`
holds exactly that layout (32bppPBGRA composited to opaque BGRX), so rows
are exported as-is, no conversion.

## D7: Reuse of DecodeFrame (background compositing, cancellation)

**Decision**: `WicSaveImage` obtains pixels via the existing
`DecodeFrame(img, ImageIndex, Progress, AppSpecific)`.

**Rationale**: `LoadThumbnail` calls
`PVSetBkHandle(hPVImage, G.rgbPanelBackground)` before exporting
(`thumbs.cpp:1005`), and `DecodeFrame` composites alpha over that color
(`CompositeOverBackground`) — transparent PNGs/GIFs get the panel
background exactly like the viewer, with zero new code. `DecodeFrame`
also already honors the `Progress` cancel polarity and caches the decoded
frame (`DecodedFrame`), so a thumbnail retry after a same-frame decode is
free. COM/WIC per-thread init (`EnsureComOnThisThread`) already covers
the icon-reader thread the same way it covers viewer threads.
