# Contract: WicSaveImage — RAW/User-Defined-Output Subset

**Feature**: 048-fix-image-thumbnails · **Component**:
`src/plugins/pictview/wicengine.cpp` (`WicSaveImage`, wired as
`PVW32DLL.PVSaveImage`) · **Consumer**: `CreateThumbnail`
(`src/plugins/pictview/thumbs.cpp:1026-1043`)

## Signature (unchanged — existing function-table slot)

```c
static PVCODE WINAPI WicSaveImage(LPPVHandle Img, const char* OutFName,
                                  LPPVSaveImageInfo pSii,
                                  TProgressProc Progress, void* AppSpecific,
                                  int ImageIndex);
```

## Acceptance predicate

The call is **in subset** iff all of the following hold:

1. `Img` is a valid `CWicImage*` (else `PVC_INVALID_HANDLE`)
2. `pSii != NULL`, `pSii->Flags & PVSF_USERDEFINED_OUTPUT`,
   `pSii->WriteFunc != NULL`
3. `pSii->Format == PVF_RAW` and `pSii->Colors == PV_COLOR_TC32`
4. `pSii->Width == 0 && pSii->Height == 0` (natural size; no scaling)
5. `pSii->CropWidth == 0 && pSii->CropHeight == 0` (no cropping)

Out-of-subset calls return `PVC_UNSUP_OUT_PARAMS` **without side
effects** — the exact current behavior for Save As (`saveas.cpp`),
print preview (`print.cpp`, always requests scaling), and batch JPEG
thumbnails (`thumbs.cpp:370`, requests `PVF_JPG`).

Flag handling inside the subset:

| Flag | Behavior |
|------|----------|
| `PVSF_USERDEFINED_OUTPUT` | required; `OutFName` is ignored |
| `PVSF_SUPERFAST` | accepted, ignored (decode-speed hint) |
| `PVSF_FLIP_VERT` | accepted; rows emitted bottom-to-top |
| any other `PVSF_*` bit | out of subset → `PVC_UNSUP_OUT_PARAMS` |

## Guaranteed behavior (in subset)

1. **Decode**: the frame `ImageIndex` (clamped: `<0` → 0) is decoded via
   the engine's `DecodeFrame`, i.e. 32bppPBGRA composited over the
   current `BkColor` into an opaque top-down BGRX DIB. Decode failures
   map to real codes (`PVC_OOM`, `PVC_READING_ERROR`,
   `PVC_NO_MORE_IMAGES`, `PVC_CANCELED`).
2. **Row stream**: exactly `Height` rows of `Width * 4` bytes are
   offered to `WriteFunc(AppSpecific, ptr, size)` in one or more batches;
   every batch is a whole number of consecutive rows (`size` is always a
   multiple of `Width * 4`); batch size is bounded (~256 KB target) so
   cancellation stays responsive. Rows are emitted top-down, or
   bottom-up when `PVSF_FLIP_VERT` is set. The pointer passed to
   `WriteFunc` is valid only for the duration of that call.
3. **Short write = normal stop**: if `WriteFunc` returns anything other
   than the offered `size`, the export stops immediately and the call
   returns `PVC_OK`. (Rationale: the thumbnail adapter `MyWriteFunc`
   returns 0 both on cancellation and on successful consumption of the
   final rows — see research.md D3. The core's `ThumbnailReady()` gate
   is the arbiter of completeness.)
4. **Progress**: when `Progress != NULL` it is invoked between batches
   with a 0–100 percentage; a **TRUE return cancels** (PictView
   polarity), the export stops and returns `PVC_OK` (indistinguishable
   from a consumer stop by design).
5. **No seeks**: `SeekFunc` is never called (sequential RAW stream).
6. **Reentrancy/threading**: callable from the icon-reader thread; COM
   init per-thread is handled internally (`EnsureComOnThisThread` via
   `DecodeFrame`). No global state is mutated beyond the per-image
   decoded-frame cache.

## Postconditions

- Success (`PVC_OK`): consumer received either the full image (its
  `ProcessBuffer` chain reached `NextLine == OriginalHeight` →
  `ThumbnailReady()` TRUE) or stopped early by its own choice.
- Failure (non-`PVC_OK`): consumer received zero or partial rows; the
  core discards the partial thumbnail (icon fallback). The image handle
  remains valid and reusable (e.g. viewer opening the same handle).

## Consumer-side contract change (thumbs.cpp)

`CPluginInterfaceForThumbLoader::LoadThumbnail` returns
`code == PVC_OK` (was: unconditional `TRUE`), where `code` is the
`PVW32DLL.CreateThumbnail` result. Effects:

- open failure (`PVOpenImageEx != PVC_OK`) → FALSE (unchanged)
- oversize guard (`MaxThumbImgSize`) → FALSE (unchanged)
- `SetParameters` OOM (`PVC_OOM`) or export failure → **FALSE** (new:
  core may try other loaders / falls back to icon; previously TRUE
  silently ended the chain)
- successful or consumer-stopped export (`PVC_OK`) → TRUE (unchanged)

## Verification hooks

- `Plugins Manager → PictView` shows the thumbnail mask list
  (`IDC_PLUGINTHUMBNAILS`) — registration precondition.
- `TRACE` output: decode failures emit existing `TRACE_E` paths in
  `wicengine.cpp`; no new logging is required by this contract.
