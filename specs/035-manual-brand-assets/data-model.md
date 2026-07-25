# Data Model: Manual Brand Asset Replacement (035)

No runtime data structures change shape; the "data" of this feature are
asset files and their validation rules.

## Entities

### Icon source set (`tools/brand/`)

| Field | Value | Validation |
|-------|-------|------------|
| `icon-master.png` | Required master artwork | PNG; square; edge ≥ 256 px (1024 recommended); decodable RGBA |
| `icon-16.png` … `icon-256.png` | Optional per-size overrides (16, 24, 32, 48, 64, 128, 256) | PNG; exactly N×N for `icon-<N>.png`; decodable RGBA |

Relationship: for each target size, override wins over master-derived
(Lanczos) rendering. Consumed only by `gen_icons.py`.

### Shipped icon files (generated, committed)

| File | Sizes | Consumer |
|------|-------|----------|
| `src/res/salamand.ico` | 16,24,32,48,64,128,256 | `IDI_SALAMANDER` — window/taskbar/exe icon |
| `src/salmon/res/salmon.ico` | same | crash reporter exe |
| `src/setup/res/setup.ico` | same | installer exe |
| `src/setup/remove/icon1.ico` | same | uninstaller exe |

Encoding invariant: 32-bpp; BMP entries ≤ 64 px, PNG entries ≥ 128 px
(checked by `--verify`). Removed: `sal_r.ico`, `sal_g.ico`, `sal_b.ico`.

### About/splash artwork

| Field | Value | Validation |
|-------|-------|------------|
| Source | `tools/brand/about.png` | PNG with alpha; recommended ≈ 512 px on the long edge |
| Shipped | `src/res/logo.png` (verbatim copy) | `IDB_LOGO_IMAGE` RCDATA |
| Display | WIC-decoded, scaled to fit reserved rect, aspect preserved | drawn via `AlphaBlend` in About + splash |

Replaces `src/res/logo.svg` / `IDB_LOGO_HAND` (deleted). The gradient
accent strips (`gradspl.svg`, `gradabt.svg`) are out of scope and remain
SVG.

### Splash copyright lines

| Field | Value |
|-------|-------|
| `VERSINFO_COPYRIGHT1` | `"Copyright © 1997-2026 Open Salamander Authors"` |
| `VERSINFO_COPYRIGHT2` | `"© 2026 Newt Commander Authors"` |
| `VERSINFO_COPYRIGHT` | unchanged concatenation — VERSIONINFO block only |

Invariant: `COPYRIGHT1 + ", " + COPYRIGHT2 == COPYRIGHT` (year-rule updates
from CLAUDE.md touch all three together).

## State transitions

### Main-window icon configuration (simplified)

- `MainWindowIcons[]`: 4 entries → 1 entry (`{IDI_SALAMANDER, IDS_SALAMANDERICON_DEFAULT}`)
- `Configuration.MainWindowIconIndex` / `...Forced` (registry
  `Main window icon index`, `-i N` command line, tasklist reports):
  any value ≥ 1 → clamped to 0 by existing `GetMainWindowIconIndex()`
  bounds check. No migration, no error surfaced.
