# Data Model: Update Application Icon to Revised Artwork (034)

No runtime data; the "model" is the static asset inventory, its provenance,
and its consumers. Identical structure to 033 — only provenance and the
`logo.svg` construction change.

## Entities

### Master icon artwork (source of truth, `tools/brand/`)

| Asset | Path | Provenance | Notes |
|-------|------|------------|-------|
| Master SVG | `tools/brand/newt-commander-icon.svg` | copied from `temp/icon/newt-commander-icon.svg` (034 revision) | 256 viewBox, `width=1024` as delivered; tile-less folder + `feDropShadow` — reference/master only, never shipped directly |
| Raster set | `tools/brand/png/newt-commander-icon-{16,24,32,48,64,128,256,512,1024}.png` | copied from `temp/icon/png/` (034 revision) | authoritative pixel renders, transparent background; 16–256 shipped via ICOs, 512/1024 kept for future use |
| In-app mark SVG | `tools/brand/logo.svg` | re-authored nanosvg-safe derivative of the 034 master | no filters; no tile; `width/height=256`; `userSpaceOnUse` gradients |
| Generator | `tools/brand/gen_icons.py` | unchanged logic; doc text refreshed | PNG→ICO packer + red/green/blue hue-remap |
| Docs | `tools/brand/README.md` | updated | design description, palette (tile colors removed) |

### Shipped icon assets (generated / copied, committed)

| Asset | Sizes (px, 32-bpp) | Generated from | Consumer |
|-------|--------------------|----------------|----------|
| `src/res/salamand.ico` | 16,24,32,48,64,128,256 | png set, verbatim | `salamand.rc2:19` → `IDI_SALAMANDER` |
| `src/res/sal_r.ico` | 16,32 | png 16/32, hue→red 0° | `salamand.rc2:20` → `IDI_SALAMANDER_RED` |
| `src/res/sal_g.ico` | 16,32 | png 16/32, hue→green 142° | `salamand.rc2:21` → `IDI_SALAMANDER_GREEN` |
| `src/res/sal_b.ico` | 16,32 | png 16/32, hue→blue 213° | `salamand.rc2:22` → `IDI_SALAMANDER_BLUE` |
| `src/res/logo.svg` | vector | copy of `tools/brand/logo.svg` | `salamand.rc2:48` → `IDB_LOGO_HAND` (RCDATA) |
| `src/salmon/res/salmon.ico` | 16,24,32,48,64,128,256 | png set, verbatim | `salmon.rc:9` → icon `1` |
| `src/setup/res/setup.ico` | 16,24,32,48,64,128,256 | png set, verbatim | `setup.rc:73` → `EXE_ICON` |
| `src/setup/remove/icon1.ico` | 16,24,32,48,64,128,256 | png set, verbatim | `remove.rc2:13` → `IDI_ICON1` |

ICO encoding rule (per entry): BMP (BGRA bottom-up + empty AND mask) for
sizes ≤ 64 px, embedded PNG for sizes ≥ 128 px.

### Code consumers (read-only — MUST NOT change)

| Site | Role |
|------|------|
| `src/logo.cpp` ~208 | splash background: always brand navy (`NC_COLOR_NAVY`) — guarantees contrast for the tile-less mark |
| `src/logo.cpp` ~218 | splash: `CSVGSprite::Load(IDB_LOGO_HAND, -1, GradientY-12, …)` — height-fit, aspect preserved |
| `src/logo.cpp` ~445 | About: `CSVGSprite::Load(IDB_LOGO_HAND, w, h, …)` — rect-fit, theme background behind |
| `src/dialogs5.cpp` ~2685 | `MainWindowIcons[]`: default/red/green/blue option list |
| `src/salamdr1.cpp`, `src/resource.rh2` | icon resource ID plumbing |

## Validation rules

- Every shipped ICO: expected entry count, exact dimensions, 32-bpp,
  encoding rule respected (`gen_icons.py --verify`).
- Every shipped ICO frame: corner pixels fully transparent (tile-less
  silhouette — no square remnant).
- `logo.svg`: square viewBox with matching `width`/`height`; element
  whitelist = `rect`, `circle`, `g`, `linearGradient`, `stop` +
  `transform`/`opacity` attributes; forbidden = `filter`, `clip-path`,
  `mask`, `feDropShadow`, `radialGradient` no longer needed (tile gone).
- Hue-remap: only pixels with S ≥ 0.45 and H within 15°–50° change hue;
  alpha channel untouched.
- File names and resource IDs identical to current tree (zero `.rc`,
  `.rc2`, `.vcxproj`, `.cpp`, `.h` diffs).

## State transitions

None (static assets). Lifecycle rule: `python tools\brand\gen_icons.py` on
a clean checkout MUST reproduce the committed ICO files byte-for-byte
(idempotent regeneration, SC-005).
