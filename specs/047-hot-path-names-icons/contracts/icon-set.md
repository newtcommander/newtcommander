# Contract: Hot Path Icon Gallery

**Feature**: 047-hot-path-names-icons

## Gallery definition

`HOT_PATH_ICON_COUNT = 10`. Indices are **stable and append-only**: the
persisted `Icon` registry value references this order, so entries may be added
at the end in future features but never removed, reordered, or repurposed.

| Index | Identity | Source | Notes |
|-------|----------|--------|-------|
| 0 | Default | shell32.dll resource 319, loaded at runtime (existing `HFavoritIcon`) | Exact current icon; never shipped by us |
| 1 | Red | `src/res/hotpath1.ico` | |
| 2 | Orange | `src/res/hotpath2.ico` | |
| 3 | Yellow | `src/res/hotpath3.ico` | |
| 4 | Green | `src/res/hotpath4.ico` | |
| 5 | Teal | `src/res/hotpath5.ico` | |
| 6 | Blue | `src/res/hotpath6.ico` | |
| 7 | Purple | `src/res/hotpath7.ico` | |
| 8 | Pink | `src/res/hotpath8.ico` | |
| 9 | Gray | `src/res/hotpath9.ico` | |

## Asset requirements (indices 1–9)

- **Original artwork** (GPLv2-compatible): a bookmark/star motif on the theme of
  the default icon but NOT derived from Microsoft artwork.
- One master (`tools/brand/hotpath-master.png`) + tint table in
  `tools/brand/gen_icons.py`; variants differ **only in hue**.
- `.ico` frames: 16, 20, 24, 32 px (covers the 16 px base size at 100–200 %
  display scale).
- Legible on light and dark backgrounds (midtone fill + contrasting outline);
  all nine mutually distinguishable at 16 px (SC-005) — verified in quickstart.
- Committed to the repository; regeneration is developer-side only
  (`python tools/brand/gen_icons.py`), never part of the build.

## Runtime contract

- Resource IDs `IDI_HOTPATH_1 … IDI_HOTPATH_9` in `src/salamand.rh`, declared in
  `src/salamand.rc2` per the existing `IDI_* ICON "res\\*.ico"` convention.
- `HICON HHotPathIcons[HOT_PATH_ICON_COUNT]` (`src/consts.h` extern,
  `src/salamdr1.cpp` lifecycle): index 0 aliases `HFavoritIcon` (not destroyed
  twice); 1–9 loaded via `LoadImage` at `IconSizes[ICONSIZE_16]`; destroyed and
  reloaded wherever `HFavoritIcon` is today (startup, color/DPI change,
  shutdown).
- Consumers receive raw `HICON`s (toolbar `TLBI_MASK_ICON`, menu `mii.HIcon`,
  drive list `drv.HIcon` with `DestroyIcon = FALSE`); the settings ListView
  builds a private `HIMAGELIST` copy at dialog init.
- Selection out of range at any point renders as index 0 (defensive clamp).
