# Data Model: Replace Application Icon (033)

This feature has no runtime data; the "model" is the static asset inventory,
its provenance, and its consumers.

## Entities

### Master icon artwork (source of truth, `tools/brand/`)

| Asset | Path | Provenance | Notes |
|-------|------|------------|-------|
| Master SVG | `tools/brand/newt-commander-icon.svg` | copied from `temp/icon/newt-commander-icon.svg` | 256 viewBox; uses `feDropShadow` + `clipPath` — reference/master only, never shipped directly |
| Raster set | `tools/brand/png/newt-commander-icon-{16,24,32,48,64,128,256,512,1024}.png` | copied from `temp/icon/png/` | authoritative pixel renders; 16–256 shipped via ICOs, 512/1024 kept for future use |
| In-app tile SVG | `tools/brand/logo.svg` | hand-authored nanosvg-safe derivative of master | no filters/clipPath; gradients + rotations only |
| Generator | `tools/brand/gen_icons.py` | rewritten | PNG→ICO packer + red/green/blue hue-remap |
| Docs | `tools/brand/README.md` | updated | pipeline, palette, size table |

### Shipped icon assets (generated / copied, committed)

| Asset | Sizes (px, 32-bpp) | Generated from | Consumer |
|-------|--------------------|----------------|----------|
| `src/res/salamand.ico` | 16,24,32,48,64,128,256 | png set, verbatim | `salamand.rc2:19` → `IDI_SALAMANDER` |
| `src/res/sal_r.ico` | 16,32 | png 16/32, hue→red | `salamand.rc2:20` → `IDI_SALAMANDER_RED` |
| `src/res/sal_g.ico` | 16,32 | png 16/32, hue→green | `salamand.rc2:21` → `IDI_SALAMANDER_GREEN` |
| `src/res/sal_b.ico` | 16,32 | png 16/32, hue→blue | `salamand.rc2:22` → `IDI_SALAMANDER_BLUE` |
| `src/res/logo.svg` | vector | copy of `tools/brand/logo.svg` | `salamand.rc2:48` → `IDB_LOGO_HAND` (RCDATA) |
| `src/salmon/res/salmon.ico` | 16,24,32,48,64,128,256 | png set, verbatim | `salmon.rc:9` → icon `1` |
| `src/setup/res/setup.ico` | 16,24,32,48,64,128,256 | png set, verbatim | `setup.rc:73` → `EXE_ICON` |
| `src/setup/remove/icon1.ico` | 16,24,32,48,64,128,256 | png set, verbatim | `remove.rc2:13` → `IDI_ICON1` |

ICO encoding rule (per entry): BMP (BGRA bottom-up + empty AND mask) for
sizes ≤ 64 px, embedded PNG for sizes ≥ 128 px.

### Code consumers (read-only — MUST NOT change)

| Site | Role |
|------|------|
| `src/logo.cpp` ~218 | splash: `CSVGSprite::Load(IDB_LOGO_HAND, -1, GradientY-12, …)` — height-fit, aspect preserved |
| `src/logo.cpp` ~445 | About: `CSVGSprite::Load(IDB_LOGO_HAND, w, h, …)` — rect-fit |
| `src/dialogs5.cpp` ~2685 | `MainWindowIcons[]`: default/red/green/blue option list |
| `src/salamdr1.cpp`, `src/resource.rh2` | icon resource ID plumbing |

## Validation rules

- Every shipped ICO: expected entry count, exact dimensions, 32-bpp,
  encoding rule respected (structural check script).
- `logo.svg`: square viewBox; element whitelist = `rect`, `circle`, `g`,
  `linearGradient`, `radialGradient`, `stop` + `transform`/`opacity`
  attributes; forbidden = `filter`, `clip-path`, `mask`, `feDropShadow`.
- Hue-remap: only pixels with S > threshold and H within the orange band
  change hue; alpha channel untouched.
- File names and resource IDs identical to current tree (zero `.rc`,
  `.rc2`, `.vcxproj`, `.cpp`, `.h` diffs).

## State transitions

None (static assets). The only lifecycle rule: regenerating assets
(`python tools\brand\gen_icons.py`) MUST be idempotent — running it on a
clean checkout reproduces the committed ICO files.
