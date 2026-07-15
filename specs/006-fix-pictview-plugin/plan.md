# Implementation Plan: Repair the PictView Plugin So It Loads and Views Images

**Branch**: `006-fix-pictview-plugin` | **Date**: 2026-07-15 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/006-fix-pictview-plugin/spec.md`

## Summary

PictView refuses to load because its startup requires the proprietary
`PVW32Cnv.dll` engine (x64: hosted by `SalPVEnv.exe` via IPC), which was
removed from the repository for GPL reasons — the entry point returns
NULL and the core reports "plugin not loaded." Every image operation in
the plugin is dispatched through a single function-pointer table
(`CPVW32DLL PVW32DLL`). This feature replaces the engine behind that
table with a new **in-process, WIC-backed image engine** of identical
signatures, so the viewer code is unchanged: the plugin loads with no
error and actually displays the common raster formats (JPEG, PNG, BMP,
GIF, TIFF) with zoom, scroll, and next/previous navigation. Features
that genuinely depended on the removed engine (Save As/convert, image
editing, TWAIN scan, screen capture, print, clipboard paste) are cleanly
hidden/disabled (D6). The out-of-process envelope and IPC layer are
retired and the build no longer needs `salpvenv`/`PVW32Cnv.lib` (D7).
Using WIC — a Windows OS component — keeps the plugin GPLv2-clean (D8).

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: Windows Imaging Component (WIC — `windowscodecs.lib`, `ole32.lib`), pure WinAPI GDI (`StretchDIBits`/`AlphaBlend`); existing plugin SDK (interface 104); `exif.dll` (GPL-clean, unchanged, optional). No third-party redistributable decode library.
**Storage**: Windows Registry for the plugin's `G` config (unchanged); NTFS files as image inputs (Unicode/long-path per features 004/005)
**Testing**: No automated UI test infra. Headless-scriptable US1 (plugin loads, no error box — feature-005 technique). US2 image display via a WIC decode-and-compare host + manual quickstart matrix on desktop
**Target Platform**: Windows 11+ x64 (primary). WIC codecs: JPEG/PNG/BMP/GIF/TIFF/ICO built-in; HEIF/WebP where OS codecs are installed
**Project Type**: Native desktop application plugin — one of 35 bundled `.spl` plugins in the 90-project MSBuild solution
**Performance Goals**: A common-size photo (≤ ~24 MP) opens and paints without perceptible delay; zoom/scroll stay interactive; very large images either scale or report a clear limit (no hang)
**Constraints**: GPLv2 — no non-free component may be linked/shipped (WIC/GDI+ are OS components, allowed under the system-library exception); plugin interface 104 unchanged; viewer source (render1/render2/thumbs) call shape unchanged (only the table implementation changes); config load/save stays compatible
**Scale/Scope**: ~1 new engine translation unit (`wicengine.cpp` + header) implementing ~10 real + ~7 stub functions; `pictview.cpp` load/init/menu/toolbar edits; `pictview.vcxproj` build edits; retire `salpvenv.vcxproj`. Viewer/render code substantially untouched

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| # | Principle | Evaluation | Status |
|---|-----------|------------|--------|
| I | Build Reproducibility | Removes an **unbuildable** proprietary dependency (`PVW32Cnv.lib`) and its envelope; the plugin builds from source with only OS libs — strictly improves reproducibility; single-command build preserved | PASS |
| II | Backward Compatibility | The plugin is currently 100% non-functional (refuses to load); any working state is a strict improvement. Config load/save kept compatible (D6/keep `G` fields); interface 104 unchanged; other plugins/core untouched | PASS |
| III | Incremental Modernization | Delivered as independently buildable increments (see sequencing); the change is contained to the engine-behind-the-table + build + menu gating; no rewrite of the viewer; legacy code touched only where load/menu wiring requires | PASS |
| IV | Windows Platform Commitment | WIC and GDI are pure Windows platform APIs; Windows 11+; no cross-platform layer; no new third-party dependency | PASS |
| V | Plugin Architecture Preservation | Preserves and **improves** the plugin: removes the license-violating binary, replaces it with the platform's imaging, keeps the SDK interface; engine boundary documented before modification ([contracts/pv-engine-contract.md](contracts/pv-engine-contract.md)) | PASS |

**GPLv2 note (constitution Technical Constraints)**: WIC/GDI+ are
Windows OS components → GPL "System Libraries" exception; no non-free
code shipped. This is the feature's core license fix (D8).

**Post-Phase-1 re-check**: design artifacts introduce no new
violations. GATE: PASS.

## Architecture Decisions (summary)

Authoritative rationale in [research.md](research.md):

- **D1** In-process WIC-backed engine fills the existing `PVW32DLL` table
- **D2** Decode to a 32bpp PBGRA DIB section; blit to caller HDC (StretchDIBits/AlphaBlend)
- **D3** Implement VIEW-CRITICAL fully; graceful non-crashing stubs for out-of-scope exports (init guard passes)
- **D4** Multi-frame via WIC frames; animation degrades to frame 0 in v1
- **D5** Open by real Unicode/long-path name (drop ANSI down-conversion), via wide filename or IStream
- **D6** Hide/disable Save As / edit / scan / capture / print / paste; keep config fields
- **D7** Retire envelope + IPC; build without salpvenv/PVW32Cnv.lib; keep `lib/PVW32DLL.h` + exif
- **D8** WIC is a Windows OS component → GPLv2-clean
- **D9** COM/WIC init per decoding thread; shared agile factory

### Increment Sequencing (constitution III)

1. **Load without the engine** — `LoadPictViewDll` (pictview.cpp:1431)
   populates `PVW32DLL` with in-process pointers (real VIEW-CRITICAL +
   graceful stubs, initially minimal), never `LoadLibrary`/
   `InitPVEXEWrapper`; fix version text (pictview.cpp:1532-1536);
   de-wire `ReleasePVEXEWrapper`. Build edit to drop salpvenv/IPC and
   add `wicengine.cpp` + link `windowscodecs.lib`. **Delivers US1**
   (SC-001): no error box, plugin registers. Verify headless.
2. **View common formats (WIC engine core)** — implement
   `PVOpenImageEx`/`PVGetImageInfo`/`PVReadImage2`/`PVDrawImage`/
   `PVSetStretchParameters`/`PVSetBkHandle`/`PVCloseImage`/
   `PVGetErrorText` over WIC + DIB (D2), incl. Unicode/long-path open
   (D5) and multi-frame/first-frame (D4). **Delivers US2** (SC-002/
   SC-003/SC-005): JPEG/PNG/BMP/GIF/TIFF display + zoom/scroll/
   next-previous.
3. **Graceful degradation** — map decode failures to
   `IDS_UNSUPPORTED_IMAGE_TYPE`/`PVGetErrorText`; hide/disable Save As,
   edit, scan, capture, print, paste (D6); backstop their handlers.
   **Delivers US3** (SC-004).
4. **Build & solution cleanup** — remove `salpvenv.vcxproj` from the
   solution; confirm `build.cmd full` produces `pictview.spl` and
   registers it (plugins.ver) with no proprietary inputs (SC-006).
5. **Hardening** — large-image handling, animation-first-frame, the
   quickstart verification matrix, config load/save compatibility, and
   a regression pass that other plugins/core still build and run.

Each increment leaves the solution buildable; increment 1 already
removes the reported error.

## Project Structure

### Documentation (this feature)

```text
specs/006-fix-pictview-plugin/
├── plan.md              # This file
├── spec.md              # Feature specification (clarified: full viewer)
├── research.md          # Phase 0 — root cause, decisions D1–D9
├── data-model.md        # Phase 1 — engine boundary + per-image context model
├── quickstart.md        # Phase 1 — build, fixtures, verification matrix
├── contracts/
│   └── pv-engine-contract.md   # PVW32DLL table subset the WIC engine implements
└── tasks.md             # Phase 2 (/speckit.tasks — not created here)
```

### Source Code (repository root)

```text
src/plugins/pictview/
├── wicengine.h                 # NEW: in-process engine interface (fills PVW32DLL)
├── wicengine.cpp               # NEW: WIC-backed PV* implementations + graceful stubs (D1–D5)
├── pictview.cpp                # MODIFIED: LoadPictViewDll fills table in-process (1431);
│                               #   version text (1532-1536); ReleaseViewer de-wire (1618);
│                               #   MenuTemplate[]/ToolBarButtons[] gating (187-378); handler backstops
├── pictview.h                  # MODIFIED (if needed): CPVW32DLL wiring / prototypes
├── precomp.h                   # MODIFIED: retire PICTVIEW_DLL_IN_SEPARATE_PROCESS use (22-23)
├── render1.cpp, render2.cpp    # MODIFIED: open call site passes UTF-8 name (D5); disabled-cmd backstops
├── lib/PVW32DLL.h              # KEPT: shared PVImageInfo / error-code types
├── PVEXEWrapper.cpp/.h,        # REMOVED from build (IPC layer retired, D7)
│   PVMessage.cpp/.h,
│   PVMessageWrapper.cpp
├── exif/                       # KEPT: GPL-clean metadata helper (unchanged)
└── vcxproj/
    ├── pictview.vcxproj        # MODIFIED: drop salpvenv ref (258-261) + IPC sources (148-153);
    │                           #   add wicengine.cpp; link windowscodecs.lib + ole32.lib
    └── salpvenv.vcxproj        # RETIRED from solution (links removed PVW32Cnv.lib)
```

**Structure Decision**: single plugin under `src/plugins/pictview/`;
the change is concentrated in one new engine translation unit plus
load/menu/build edits. The viewer/render code keeps its call shape
because the engine is swapped behind the existing `PVW32DLL` table.

## Complexity Tracking

> No constitution violations require justification. The one notable
> deletion (retiring `salpvenv`/IPC) removes complexity rather than
> adding it. No entries.
