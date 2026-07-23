# Newt Commander Brand Assets

Source of truth for the "Split Disc — Extruded" visual identity (feature 032)
and the generator that turns it into the checked-in runtime assets.

## Files

| File | Purpose |
|------|---------|
| `newt-commander-icon.svg` | Master icon (96 viewBox), full-detail variant |
| `newt-commander-lockup-dark.svg` / `-light.svg` | Horizontal logo lockups (live Archivo text — reference only, not shipped; the app draws the wordmark with GDI) |
| `logo.svg` | nanosvg-compatible icon copy shipped as RCDATA `IDB_LOGO_HAND` (`src/res/logo.svg`) |
| `gradient-band.svg` | Blue→orange brand accent strip shipped as `src/res/gradspl.svg` and `gradabt.svg` |
| `gen_icons.py` | Rasterizer/ICO packer (pure Pillow, no external tools) |

## Regenerating the checked-in assets

```batch
python tools\brand\gen_icons.py src\res
```

Requires Python 3 + Pillow on the developer machine (asset-authoring time only —
NOT a build dependency; the generated `.ico`/`.svg` files are committed).

Outputs into `src/res/` (file names kept from upstream so no project/rc changes
are needed):

- `salamand.ico` — application icon: 16 px favicon variant, 24/32 px simplified,
  48/64/128/256 px full (32-bpp; BMP entries ≤ 64 px, PNG ≥ 128 px)
- `sal_r.ico`, `sal_g.ico`, `sal_b.ico` — tray state icons (16+32 px, plates
  tinted red/green/blue)

## Size variants (per visual-style guidance)

- **≥ 48 px** full: radial disc light, plate edges + gradients, listing rows
- **24–48 px** simplified: flat colors `#3B82F6` / `#F97316`, flat navy disc, rows
- **≤ 16 px** favicon: flat, no rows, wider centre gap

## Palette

Navy `#0A1424` · disc edge `#02060D` · disc radial `#1B3054→#070E1A` ·
blue plate `#6FA5FF→#2E6BE0` (edge `#1E4FB8`, flat `#3B82F6`) ·
orange plate `#FFB35C→#EA6A0B` (edge `#B85306`, flat `#F97316`) ·
rows white 50 % · light blue `#93C5FD` · off-white `#EAF2FB`

Wordmark colors (GDI-drawn, no font shipped): dark bg `Newt #EAF2FB` +
`Commander #F97316`, tagline `#8FA6C4`; light bg `Newt #0A1424` +
`Commander #EA6A0B`, tagline `#5D82B8`.
