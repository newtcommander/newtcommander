# Research: Repair the PictView Plugin

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)
**Date**: 2026-07-15

## Root cause (verified in source)

`SalamanderPluginEntry` (pictview.cpp:557) calls `InitViewer` →
`LoadPictViewDll` (pictview.cpp:1431). That routine requires the
proprietary decode engine `PVW32Cnv.dll` (on x64 hosted out-of-process
by `SalPVEnv.exe` via `InitPVEXEWrapper`, PVEXEWrapper.cpp:75). The
engine and its build inputs were **removed for GPL reasons**
(`lib/readme.txt`: *"We have removed PVW32Cnv.lib because its source
code is not available, which violates the GPL license"*). On the
default x64 build `CreateProcess("SalPVEnv.exe")` (PVEXEWrapper.cpp:497)
fails; on x86 `LoadLibrary("PVW32Cnv.dll")` (pictview.cpp:1443) fails.
Either way `InitViewer` returns FALSE, the entry point returns NULL,
and the core loader shows `IDS_PLUGININVALID2` (plugins1.cpp:2467) —
the exact reported message. **Not** a version mismatch (104 == 104),
**not** a static-import failure (`pictview.spl` has no import of
`PVW32Cnv.dll`; the engine was only ever `LoadLibrary`'d or reached via
IPC).

## The pivotal architectural fact

Every image operation in the plugin is dispatched through **one global
function-pointer table**, `CPVW32DLL PVW32DLL` (pictview.cpp:121;
struct pictview.h:40-65). The x86 back-end fills it via `GetProcAddress`
(pictview.cpp:1451-1471); the x64 back-end fills it with IPC stubs
(PVEXEWrapper.cpp:77). Every caller in render1/render2/thumbs/saveas/
print invokes `PVW32DLL.PVReadImage2(...)` etc. **So a replacement
engine only needs to fill this same table with in-process functions of
identical signature — the call shape at every viewer site is
unchanged.**

## Decisions

### D1 — Replace the proprietary engine with an in-process, WIC-backed engine

**Decision**: Add a new in-process image engine (new source
`wicengine.{h,cpp}`) that implements the `PVW32Cnv` function set over
the **Windows Imaging Component (WIC)**, and have `LoadPictViewDll`
populate `PVW32DLL.*` with those functions instead of loading any DLL
or launching the envelope.
**Rationale**: WIC is a Windows platform component (present since
Vista, extended on Win10/11) with built-in codecs for JPEG, PNG, BMP,
GIF, TIFF, ICO, and (via OS-provided codecs) HEIF/WebP; it is
thread-safe with an agile factory and decodes to arbitrary pixel
formats. The repository's own TODO names WIC. Keeps the viewer code
untouched (fills the existing table).
**Alternatives considered**: (a) **GDI+** — simpler API but older,
documented thread-safety caveats, fewer formats, and `Gdiplus` startup
lifetime management; rejected as the primary engine (may be a fallback
for a format WIC lacks). (b) **Bundle a third-party decode library**
(libjpeg/libpng/giflib/libtiff) — more code, more maintenance, and
duplicates what the OS already provides; the plugin already ships
libjpeg inside `exif.dll` for metadata only. (c) **Keep the
out-of-process envelope with an open-source engine** — unnecessary
complexity; the 32-bit envelope existed only to host the 32-bit
proprietary DLL.

### D2 — Pixel model: decode to a 32bpp premultiplied BGRA DIB section; blit to the caller's HDC

**Decision**: Per open image, the engine holds a top-down 32bpp BGRA
(`GUID_WICPixelFormat32bppPBGRA`) DIB section for the current frame,
plus the current stretch state. `PVReadImage2(dc≠NULL)` decodes then
blits (`StretchDIBits`/`AlphaBlend` over the background colour from
`PVSetBkHandle`); `PVReadImage2(dc==NULL)` decodes only; `PVDrawImage`
blits the already-decoded frame to `(X,Y)` clipped to `rect`.
**Rationale**: The viewer draws exclusively by handing the engine an
HDC (render1.cpp:1602/1734); it never consumes raw bits for display.
A 32bpp DIB gives correct alpha compositing and is trivially
StretchBlt-able at any zoom/mirror. Matches how `PVDrawImage` is used
for clipboard/print too (render1.cpp:3907, 3640).
**Alternatives**: keeping the original + a separate stretched DDB (as
the proprietary engine did) — more state, no benefit with StretchDIBits.

### D3 — Minimum viable table + graceful stubs for the rest

**Decision**: Implement the VIEW-CRITICAL set fully: `PVOpenImageEx`,
`PVGetImageInfo`, `PVReadImage2`, `PVDrawImage`, `PVSetStretchParameters`,
`PVSetBkHandle`, `PVCloseImage`, `PVGetErrorText`, `PVGetDLLVersion`,
`PVSetParam` (stub). Provide **non-crashing stubs** returning a clean
error `PVCODE` for the out-of-scope exports (`PVSaveImage`,
`PVChangeImage`, `PVCropImage`, `PVIsOutCombSupported`, `PVGetHandles2`,
`PVLoadFromClipboard`, `PVReadImageSequence`). Every table slot is
non-NULL so the init guard at pictview.cpp:1473 passes unchanged.
**Rationale**: The guard hard-requires several pointers non-NULL;
stubs satisfy it and make any not-yet-hidden feature degrade instead of
crashing (FR-005/FR-006). `PVGetHandles2` may optionally return
`pLines` from the DIB so the pipette/histogram keep working — decided
during implementation.
**Alternatives**: relaxing the guard — unnecessary once stubs exist.

### D4 — Multi-frame via WIC frames; animation degrades to frame 0

**Decision**: `NumOfImages` = WIC frame count; `PVGetImageInfo(…,
ImageIndex)` and `PVReadImage2(…, ImageIndex)` select the frame. For
GIF/ANI, do NOT set `PVFF_IMAGESEQUENCE` in v1 — the viewer then shows
frame 0 as a still (FR US2 scenario 4). Multi-frame animation playback
is a follow-up enhancement.
**Rationale**: Multi-page TIFF and "first frame of animation" are
straightforward with WIC frames; full animation compositing
(disposal methods, per-frame delay timer) is a larger, separable
effort.

### D5 — Unicode / long-path open path

**Decision**: The engine opens by the **real Unicode name**. Today the
plugin down-converts the open path to ANSI for the proprietary
ANSI-only DLL (`U8ToDLLPathAlloc`, used to fill
`PVOpenImageExInfo.FileName`). The WIC engine instead receives the
UTF-8 path and converts it to UTF-16 (`\\?\`-extended for long paths,
per feature 004) to create the decoder from a wide filename or an
`IStream` over the file. The open call site is adjusted to pass the
UTF-8 name through instead of ANSI.
**Rationale**: FR-007 requires Unicode/long-path images to open;
ANSI down-conversion would drop non-ANSI names. Consistent with
features 004/005.
**Alternatives**: open via the existing `TPVReadFunc/TPVSeekFunc`
user-input callback path (the plugin already has one for ADS/mem) —
viable and avoids touching `FileName`; the engine will support an
`IStream` wrapper over those callbacks so both paths work.

### D6 — Disable out-of-scope features cleanly (no broken controls)

**Decision**: Remove the menu/toolbar/accelerator rows for Save As
(CMD_SAVEAS), edit (CMD_CROP/ROTATE*/MIRROR*), TWAIN scan
(CMD_SCAN*), screen capture (CMD_CAPTURE*), print (CMD_PRINT/
PAGE_SETUP), and clipboard paste (CMD_PASTE) from `MenuTemplate[]`
(pictview.cpp:187-333) and `ToolBarButtons[]` (pictview.cpp:343-378);
short-circuit their `render1.cpp` WM_COMMAND handlers with an
`IDS_UNSUPPORTED_IMAGE_TYPE`-style message as a defensive backstop.
**Rationale**: FR-006 — features with no engine backing must be
hidden/disabled, not presented as broken. Menu/toolbar templates are
the single source; removing rows also removes accelerators.
**Alternatives**: leaving them enabled but erroring — rejected (poor
UX, violates FR-006). Keep the config fields (`G.Save.*`, capture
config) so config load/save stays compatible (FR-008).

### D7 — Retire the envelope/IPC layer and fix the build

**Decision**: In `pictview.vcxproj`, remove the `salpvenv.vcxproj`
ProjectReference (pictview.vcxproj:258-261) and the IPC sources
(`PVEXEWrapper.cpp`, `PVMessage.cpp`, `PVMessageWrapper.cpp`,
pictview.vcxproj:148-153); add `wicengine.cpp`; link
`windowscodecs.lib` + `ole32.lib`. Retire `salpvenv.vcxproj` from the
solution (it cannot link without the removed `PVW32Cnv.lib`). Ignore
`PICTVIEW_DLL_IN_SEPARATE_PROCESS` (precomp.h:22). **Keep**
`lib/PVW32DLL.h` (defines `PVImageInfo`/error codes used everywhere)
and `exif.vcxproj` (GPL-clean, already built, loaded non-fatally).
**Rationale**: The build must not require the removed proprietary lib
(constitution I, FR-002). Nothing else references `PVW32Cnv.lib`.

### D8 — GPLv2 clearance for WIC

**Decision**: Using WIC (and, if needed, GDI+) is GPLv2-compatible
because they are **Windows operating-system components** and fall under
the GPL "System Libraries" exception (major components of the OS on
which the program runs). No non-free code is linked or shipped.
**Rationale**: This is the whole point — replace the license-violating
proprietary binary with the platform's own imaging APIs. Resolves
FR-002 with zero third-party redistribution.

### D9 — COM/WIC threading

**Decision**: Initialize COM (`CoInitializeEx`, apartment-agnostic;
the WIC factory is free-threaded) on each thread that decodes — the
viewer window thread and the thumbnail loader thread — and create/reuse
an `IWICImagingFactory`. Guard against double-init and uninit on thread
exit.
**Rationale**: WIC is COM; the plugin already spins viewer and
thumbnail threads. The agile factory can be shared, but COM must be
initialized per thread that calls it.

## Engine boundary summary (what the WIC engine implements)

Full detail in [contracts/pv-engine-contract.md](contracts/pv-engine-contract.md).
VIEW-CRITICAL (implement fully): `PVOpenImageEx`, `PVGetImageInfo`,
`PVReadImage2`, `PVDrawImage`, `PVSetStretchParameters`,
`PVSetBkHandle`, `PVCloseImage`, `PVGetErrorText`, `PVGetDLLVersion`,
`PVSetParam`(stub). GRACEFUL STUBS (out of scope): `PVSaveImage`,
`PVChangeImage`, `PVCropImage`, `PVIsOutCombSupported`, `PVGetHandles2`
(optionally real for pipette), `PVLoadFromClipboard`,
`PVReadImageSequence`.

## Verification research

- The plugin-load result is observable **without interactive input**
  (as feature 005 proved): read the Plugin Manager / absence of the
  startup error box via window enumeration. So US1 (SC-001) is
  scriptable headless.
- Actual image *display* (US2) needs the viewer window; where the
  headless session cannot drive F3, verification falls back to (a)
  decoding the same test images through the WIC engine in a tiny host
  and comparing dimensions/pixels to a reference, and (b) the manual
  quickstart matrix on a desktop.
- Build check: `build.cmd full` must produce `pictview.spl` **without**
  `salpvenv`/`PVW32Cnv.lib` and register it (plugins.ver).
