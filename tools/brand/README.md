# Newt Commander Brand Assets

Source of truth for the Newt Commander visual identity — the **folder-tile
icon** (feature 033: orange folder with documents on a dark navy rounded
tile) plus the wordmark/accent elements established by feature 032 — and the
packer that turns it into the checked-in runtime assets.

## Files

| File | Purpose |
|------|---------|
| `newt-commander-icon.svg` | Master icon (256 viewBox, full detail incl. soft drop shadow) — reference only, never shipped directly |
| `png/newt-commander-icon-{16,24,32,48,64,128,256,512,1024}.png` | **Authoritative rasters** of the master (approved renders; the packer only packs them, it never redraws) |
| `logo.svg` | Hand-maintained nanosvg-safe derivative shipped as RCDATA `IDB_LOGO_HAND` (`src/res/logo.svg`) — drawn by the app in About + splash |
| `newt-commander-lockup-dark.svg` / `-light.svg` | Horizontal logo lockups (live Archivo text — reference only; the app draws the wordmark with GDI) |
| `gradient-band.svg` | Blue→orange brand accent strip shipped as `src/res/gradspl.svg` and `gradabt.svg` (unchanged since 032) |
| `gen_icons.py` | PNG→ICO packer + red/green/blue hue-remap (pure Pillow) |

## Regenerating the checked-in assets

```batch
python tools\brand\gen_icons.py            :: pack all shipped .ico files
python tools\brand\gen_icons.py --verify   :: structural check, no writes
```

Requires Python 3 + Pillow on the developer machine (asset-authoring time
only — NOT a build dependency; the generated `.ico` files are committed).

Outputs (upstream file names kept so no project/rc changes are needed):

| File | Entries (32-bpp) | Source |
|------|------------------|--------|
| `src/res/salamand.ico` | 16,24,32,48,64,128,256 | png set verbatim |
| `src/res/sal_r.ico` / `sal_g.ico` / `sal_b.ico` | 16,32 | png 16/32, folder hue-remapped red/green/blue (Configuration → Main Window icon variants) |
| `src/salmon/res/salmon.ico` | 16,24,32,48,64,128,256 | png set verbatim |
| `src/setup/res/setup.ico` | 16,24,32,48,64,128,256 | png set verbatim |
| `src/setup/remove/icon1.ico` | 16,24,32,48,64,128,256 | png set verbatim |

ICO encoding rule: BMP entries ≤ 64 px, PNG entries ≥ 128 px. The 512/1024
px renders are kept for future use (web, store, docs) and are not packed.

Hue-remap: pixels with HSV hue in the orange band (15°–50°) and saturation
≥ 0.45 get their hue replaced (red 0°, green 142°, blue 213°), preserving
lightness/saturation/alpha — papers and the cream pill stay neutral.

## logo.svg — bundled-nanosvg constraints

`src/res/logo.svg` is NOT generated; edit `tools/brand/logo.svg` and copy it
over. It must stay inside what `src/common/dep/nanosvg` actually renders:

- **No `filter`/`feDropShadow`, no `clip-path`/`mask`** (unsupported; the
  master's soft shadow is deliberately dropped, its clipped tile fill is
  rebuilt from plain rounded rects).
- **`width`/`height` attributes MUST equal the viewBox size** — a mismatch
  makes nanosvg scale shapes but not gradient transforms (gradients then
  sample wrong; this bug made the 032 tile render its plate gradients flat).
- **Gradients use `gradientUnits="userSpaceOnUse"`** with explicit
  coordinates (objectBoundingBox coords are unreliable in this vintage).
- Linear/radial gradients, multi-stop, `stop-opacity`, group `transform`
  (incl. 3-arg `rotate`) and shape `opacity` are supported and verified.

## Palette (icon)

Tile edge `#02060D` · tile radial `#1B3054→#070E1A` · sheen white 6 % ·
folder `#FFB35C→#F97316→#EA6A0B` (back pocket `#D96A15→#B85306`, inner
shade to `#8A3E04` 45 %) · papers `#EAF2FB` / `#FDFEFF` · text lines
`#93C5FD` · cream pill `#FFD9AE`, dot `#FFC98F`.

Wordmark colors (GDI-drawn, no font shipped, unchanged since 032): dark bg
`Newt #EAF2FB` + `Commander #F97316`, tagline `#8FA6C4`; light bg
`Newt #0A1424` + `Commander #EA6A0B`, tagline `#5D82B8`.
