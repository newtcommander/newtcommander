# Implementation Plan: Restore Image Thumbnails in Thumbnail View

**Branch**: `048-fix-image-thumbnails` | **Date**: 2026-08-02 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/048-fix-image-thumbnails/spec.md`

## Summary

Panel thumbnail view (Alt+5) shows only file icons because the feature-006
WIC engine stubbed out `PVSaveImage` — the entry point the pictview
thumbnail path uses to stream decoded pixel rows into the core's
`CSalamanderThumbnailMaker`. Registration, mask matching, the icon-reader
thread, and the painting path are all verified intact
([research-thumbnail-chain.md](research-thumbnail-chain.md)).

The fix implements the exact `PVSaveImage` subset the thumbnail path
needs inside the WIC engine (`WicSaveImage`): RAW 32bpp
(`PVF_RAW`/`PV_COLOR_TC32`) export at natural size through the
caller-supplied `WriteFunc`, reusing the engine's existing `DecodeFrame`
(which already handles cancellation and alpha compositing over the panel
background). A second, independent one-line fix makes
`CPluginInterfaceForThumbLoader::LoadThumbnail` return the real result
code instead of unconditional `TRUE`, so failures become diagnosable
instead of silently terminating the loader chain. Every other
`PVSaveImage` caller (Save As, printing, batch JPEG thumbnails) keeps its
current behavior via strict input guards.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Pure WinAPI + Windows Imaging Component
(`wincodec.h`, already the pictview engine since feature 006); no new
external dependencies
**Storage**: N/A (no configuration changes; existing registry values
`IgnoreThumbnails`, `MaxThumbImgSize`, thumbnail size in core config are
untouched and continue to apply)
**Testing**: Build gates (`build.cmd` Debug x64) + manual validation per
`quickstart.md` (the project has no automated UI test harness)
**Target Platform**: Windows 11+, x64
**Project Type**: Desktop application — plugin-internal fix
(`src/plugins/pictview/`), zero core changes
**Performance Goals**: Panel stays interactive while thumbnails load
(work runs on the existing icon-reader thread; chunked row delivery with
cancellation checks preserves the legacy responsiveness contract)
**Constraints**: Plugin ABI (interface version 105) unchanged; no
behavior change for any `PVSaveImage` caller other than the thumbnail
path; `MaxThumbImgSize` megapixel guard still bounds worst-case decode
**Scale/Scope**: 2 files touched (`wicengine.cpp` + `thumbs.cpp`),
~100 lines added, one new engine capability

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Verdict | Notes |
|-----------|---------|-------|
| I. Build Reproducibility | PASS | No build-system changes; standard `build.cmd` builds the plugin. |
| II. Backward Compatibility | PASS | Restores baseline behavior lost in feature 006; plugin ABI 105 untouched; no core changes; no config format changes. Strict guards keep all other `PVSaveImage` callers behaving exactly as today. |
| III. Incremental Modernization | PASS | Change confined to the stub being replaced plus a one-line return-code fix at the call site; no refactoring of adjacent code. |
| IV. Windows Platform Commitment | PASS | WIC is the OS-native imaging component, already in use. |
| V. Plugin Architecture Preservation | PASS | Implements the missing thumbnail capability inside the plugin using the OS component — precisely the "PictView replacement" resolution path the constitution prescribes. |
| VI. UI Consistency | PASS | No UI changes; thumbnails render through the existing core painting path. |

**Post-design re-check (after Phase 1)**: PASS — the contract
(`contracts/wicsaveimage-raw-subset.md`) confirms no ABI surface changes
and no behavior change outside the thumbnail path.

## Project Structure

### Documentation (this feature)

```text
specs/048-fix-image-thumbnails/
├── spec.md                        # Feature specification
├── research-thumbnail-chain.md    # Pre-plan root-cause analysis (specify phase)
├── plan.md                        # This file
├── research.md                    # Phase 0 output — design decisions D1–D7
├── data-model.md                  # Phase 1 output — pixel-flow data model
├── quickstart.md                  # Phase 1 output — manual validation scenarios
├── contracts/
│   └── wicsaveimage-raw-subset.md # Phase 1 output — engine contract
└── tasks.md                       # Phase 2 output (/speckit.tasks)
```

### Source Code (repository root)

```text
src/plugins/pictview/
├── wicengine.cpp     # WicSaveImage: stub → RAW/user-defined-output subset
│                     # (decode via existing DecodeFrame, stream rows to
│                     #  WriteFunc in cancellable chunks)
└── thumbs.cpp        # LoadThumbnail: `return TRUE` → `return code == PVC_OK`
```

No other files change. The core (`src/thumbnl.*`, `src/fileswn*.cpp`,
`src/icncache.*`) is verified correct and stays untouched.

**Structure Decision**: Plugin-internal fix in the two files above. The
engine gains the capability where feature 006 removed it (`wicengine.cpp`);
the call site gains honest error propagation (`thumbs.cpp`). This is the
follow-up that feature 006's own validation report anticipated
("the WIC decoder can fill the RAW 32bpp buffer directly").

## Complexity Tracking

No constitution violations — table not needed.

## Scope Notes (deliberate exclusions)

- **EXIF auto-rotation of thumbnails**: the WIC engine has never set
  `PVFF_EXIF` (feature 006), so the EXIF orientation branch in
  `thumbs.cpp:923` is dormant — exactly as it is for the viewer's
  auto-rotate (`render1.cpp:1646`), which is gated on the same flag.
  Thumbnails will therefore match what the viewer shows today.
  Restoring EXIF orientation product-wide (viewer + thumbnails together)
  is a separate follow-up feature; fixing it only for thumbnails would
  make the two views disagree.
- **Print preview / Save As**: `print.cpp` needs `PVSaveImage` scaling +
  cropping; `saveas.cpp` needs file-format encoders. Both remain
  unsupported (unchanged behavior) and out of scope.
