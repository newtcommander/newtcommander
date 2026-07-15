# Data Model: Repair the PictView Plugin

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)

No business/persistent data changes. The "data model" here is (a) the
engine boundary the WIC engine implements, and (b) the per-open-image
runtime state it holds.

## Encoding / representation contract (from research D2/D5)

| Aspect | Value |
|--------|-------|
| Decoded pixel format | 32bpp premultiplied BGRA (`GUID_WICPixelFormat32bppPBGRA`), top-down |
| In-memory bitmap | one DIB section per open image (current frame), for `StretchDIBits`/`AlphaBlend` to a caller HDC |
| Open input | UTF-8 path → UTF-16 (`\\?\`-extended for long paths, per 004) → WIC decoder from wide filename or `IStream`; or attach-to-HBITMAP; or user Read/Seek callback stream |
| Background compositing | `PVSetBkHandle(COLORREF)` colour used to flatten alpha when the caller wants an opaque draw |
| Stretch state | target width/height (signed → mirror) + StretchBlt mode from `PVSetStretchParameters` |

## Entities

### 1. `CPVW32DLL` table (existing — target of the swap)

The global `PVW32DLL` (pictview.cpp:121; struct pictview.h:40-65). This
feature changes only **which functions it points at** — from
`GetProcAddress`/IPC stubs to the in-process WIC engine. Shape and
member signatures are unchanged (see
[contracts/pv-engine-contract.md](contracts/pv-engine-contract.md)).

### 2. `PVImageInfo` (existing type, filled by the engine)

`lib/PVW32DLL.h:308-336` (`#pragma pack(4)`). Fields the WIC engine
populates on open/get-info:

| Field | Source from WIC | Notes |
|-------|-----------------|-------|
| `cbSize` | echo caller | validate |
| `Width`, `Height` | frame `GetSize` | pixels |
| `BytesPerLine` | `Width * 4` | 32bpp stride |
| `Colors` | `PV_COLOR_TC32` | always truecolor after conversion |
| `Format` | mapped from container GUID | `PVF_JPG`/`PVF_PNG`/`PVF_BMP`/`PVF_GIF`/`PVF_TIFF`… |
| `Flags` | `PVFF_*` | `PVFF_IMAGESEQUENCE` **not** set in v1 (D4) |
| `ColorModel` | `PVCM_RGB` | |
| `NumOfImages` | `GetFrameCount` | multi-page/frame |
| `CurrentImage` | selected frame index | 0-based |
| `HorDPI`, `VerDPI` | `GetResolution` | aspect correction |
| `Info1..3`, `Comment` | best-effort/empty | optional |
| `FSI` | NULL (v1) | GIF/ANI union unused when sequence off |

### 3. Per-image engine context (new — `CWicImage`)

Held behind each `LPPVHandle` returned by `PVOpenImageEx`; freed by
`PVCloseImage`.

| Member | Purpose |
|--------|---------|
| `IWICBitmapDecoder*` | the open decoder (kept for multi-frame `PVGetImageInfo`/`PVReadImage2` by index) |
| `IStream*` (optional) | when opened from callbacks/attach, the backing stream |
| current-frame DIB (`HBITMAP` + bits ptr + `BITMAPINFO`) | decoded 32bpp PBGRA for the selected frame |
| `PVImageInfo` snapshot | last-filled info for the current frame |
| stretch state | target W/H (signed=mirror), StretchBlt mode |
| background `COLORREF` | from `PVSetBkHandle` |
| frame count, current index | multi-frame navigation |

### 4. Supported-format set (guaranteed members)

JPEG, PNG, BMP, GIF, TIFF (WIC built-in codecs). ICO and, where the OS
provides codecs, HEIF/WebP are best-effort. A container WIC cannot
decode → the engine returns an error `PVCODE` and the viewer shows
`IDS_UNSUPPORTED_IMAGE_TYPE` (graceful degradation).

### 5. Disabled feature set (out of scope — D6)

Commands removed from menu/toolbar/accelerators and backstopped in
handlers: `CMD_SAVEAS`, `CMD_CROP`, `CMD_ROTATE180/LEFT/RIGHT`,
`CMD_MIRROR_HOR/VERT`, `CMD_SCAN`/`CMD_SCAN_SOURCE`, `CMD_CAPTURE*`,
`CMD_PRINT`/`CMD_PAGE_SETUP`, `CMD_PASTE`. Their config fields in `G`
are retained so config load/save stays compatible (FR-008).

## Invariants

1. Every `PVW32DLL` slot is non-NULL after load (real fn or graceful
   stub) → init guard (pictview.cpp:1473) passes; no NULL-call crash.
2. `PVOpenImageEx`→…→`PVCloseImage` is balanced; no GDI/WIC/COM leak
   per open/close cycle.
3. A decode failure never aborts the plugin or the app — it returns a
   clean `PVCODE` and yields a localized message (FR-005).
4. Config written by the previous PictView loads without error and the
   plugin still registers (FR-008).
5. No proprietary or non-GPLv2 component is linked or shipped (FR-002).
