# Contract: gen_icons.py CLI (035)

## Invocation

```
python tools/brand/gen_icons.py            # regenerate all shipped assets
python tools/brand/gen_icons.py --verify   # structural check only, no writes
```

Paths resolved relative to the script; cwd-independent.

## Inputs

- `tools/brand/icon-master.png` — REQUIRED
- `tools/brand/icon-<N>.png` for N ∈ {16, 24, 32, 48, 64, 128, 256} — optional
- `tools/brand/about.png` — REQUIRED

## Outputs (generate mode)

- `src/res/salamand.ico`, `src/salmon/res/salmon.ico`,
  `src/setup/res/setup.ico`, `src/setup/remove/icon1.ico` — each with
  entries 16, 24, 32, 48, 64, 128, 256 px, 32-bpp; BMP encoding ≤ 64 px,
  PNG encoding ≥ 128 px; per-size source = override if present else
  Lanczos-downscaled master
- `src/res/logo.png` — verbatim copy of `tools/brand/about.png`
- One `OK: wrote <path>` line per output on stdout; exit code 0

## Validation errors (generate mode)

Exit code ≠ 0, single `error: …` line naming the offending file and the
expected property, no outputs written for the failing run:

| Condition | Message names |
|-----------|--------------|
| `icon-master.png` missing | the missing path |
| master not square or < 256 px | actual size + requirement |
| `icon-<N>.png` not exactly N×N | file, actual size, expected N×N |
| any input not decodable as PNG | file + decode failure |
| `about.png` missing | the missing path |

## Verify mode

`--verify` re-checks committed outputs structurally (entry count, sizes,
32 bpp, BMP/PNG encoding rule, `logo.png` present and PNG-signed) without
writing; prints `<path>: OK|FAIL: …` per target; exit 0 iff all pass.

## Removed vs. feature 033/034 contract

- `sal_r.ico` / `sal_g.ico` / `sal_b.ico` targets and all hue-remap
  parameters (`ORANGE_BAND`, `SAT_THRESHOLD`, `HUE_*`) — deleted.
- Input set `png/newt-commander-icon-<size>.png` — replaced by
  master + overrides layout above.
