# Contract: In-Process Image Engine (`PVW32DLL` table)

**Feature**: [../spec.md](../spec.md) | **Plan**: [../plan.md](../plan.md)

The WIC engine fills the existing `CPVW32DLL PVW32DLL` table
(pictview.h:40-65) with in-process functions. Signatures are **exactly**
those in `lib/PVW32DLL.h` (stdcall `WINAPI`, `#pragma pack(4)` structs,
`PVCODE` return, `PVC_OK == 0`). Callers in render1/render2/thumbs are
unchanged. Do not alter the table shape or `lib/PVW32DLL.h`.

## Must implement (VIEW-CRITICAL)

| Function | Behavior the engine must provide |
|----------|----------------------------------|
| `PVOpenImageEx(LPPVHandle* Img, LPPVOpenImageExInfo, LPPVImageInfo, int cbSize)` | Open from `FileName` (UTF-8→UTF-16, `\\?\` long-path), or attached HBITMAP (`PVOF_ATTACH_TO_HANDLE`), or user `ReadFunc`/`SeekFunc` stream (`PVOF_USERDEFINED_INPUT`). Create WIC decoder, allocate context, fill `PVImageInfo` for frame 0. Return `PVC_OK` or an error code. |
| `PVGetImageInfo(LPPVHandle, LPPVImageInfo, int cbSize, int ImageIndex)` | Re-fill `PVImageInfo` for frame `ImageIndex` (multi-page/frame). |
| `PVReadImage2(LPPVHandle, HDC PaintDC, RECT* pDRect, TProgressProc, void* AppSpecific, int ImageIndex)` | Decode frame `ImageIndex` to the context DIB (32bpp PBGRA); if `PaintDC != NULL`, `StretchDIBits`/`AlphaBlend` into `PaintDC` clipped to `pDRect`, honoring stretch state and background colour; call `Progress` ≥ once (respect cancel). `PaintDC == NULL` = decode only. |
| `PVDrawImage(LPPVHandle, HDC PaintDC, int X, int Y, LPRECT rect)` | Blit the already-decoded, already-stretched current frame into `PaintDC` at `(X,Y)` clipped to `rect`. No re-decode. |
| `PVSetStretchParameters(LPPVHandle, DWORD Width, DWORD Height, DWORD Mode)` | Store target size (negative encodes mirror) and StretchBlt mode; applied by the next `PVReadImage2`/`PVDrawImage`. |
| `PVSetBkHandle(LPPVHandle, COLORREF BkColor)` | Store the background colour used to flatten alpha on opaque draws. (Plugin typedef uses `COLORREF`; treat the arg as a colour.) |
| `PVCloseImage(LPPVHandle)` | Release DIB + WIC/COM objects + context. Idempotent-safe. |
| `PVGetErrorText(DWORD ErrorCode) → const char*` | Map engine codes to a human string (reuse `IDS_DLL+code` block or a small internal table). |
| `PVGetDLLVersion(void) → DWORD` | Return a synthetic version for the built-in engine (used only for the About/info text). |
| `PVSetParam(LPPVHandle …)` | Stub returning `PVC_OK`. Init requires it non-NULL (pictview.cpp:1473); it only registers a text callback — no-op is fine. |

## Graceful stubs (out of scope — must exist, must not crash)

Return a clean "unsupported / not available" `PVCODE` (or a benign
success where the caller only checks non-NULL). These are hard-required
non-NULL by the init guard (pictview.cpp:1473) or are only reached from
features being disabled (D6):

`PVSaveImage`, `PVChangeImage`, `PVCropImage`, `PVIsOutCombSupported`
(return 0 = "no output combination supported"), `PVGetHandles2`
(optionally real: expose the DIB `pLines`/`Palette` so pipette +
histogram keep working), `PVLoadFromClipboard`, `PVReadImageSequence`
(return "no sequence" so the viewer shows a still frame).

## Format mapping (WIC container GUID → `PVImageInfo.Format`)

`GUID_ContainerFormatJpeg → PVF_JPG`, `…Png → PVF_PNG`,
`…Bmp → PVF_BMP`, `…Gif → PVF_GIF`, `…Tiff → PVF_TIFF`,
`…Ico → PVF_ICO`. Unknown/undecodable container → open fails with an
error the viewer maps to `IDS_UNSUPPORTED_IMAGE_TYPE`.

## Threading / COM (D9)

- COM initialized (`CoInitializeEx`) on each thread that calls the
  engine (viewer window thread; thumbnail loader thread).
- One shared agile `IWICImagingFactory`; per-image decoders are used
  only on the thread that owns the image.

## Compatibility

- No change to the plugin SDK (interface 104), to `lib/PVW32DLL.h`, or
  to the viewer call sites.
- No proprietary component is linked or loaded; WIC (`windowscodecs`)
  and GDI are Windows OS components (GPLv2 system-library exception).
- `exif.dll` remains a separate, optional, non-fatal metadata helper.
