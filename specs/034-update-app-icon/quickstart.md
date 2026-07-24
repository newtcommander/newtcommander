# Quickstart: Update Application Icon to Revised Artwork (034)

## Prerequisites

- Python 3 + Pillow (asset-authoring only; NOT a build dependency)
- VS2022 + C++ workload (to build and visually verify)

## Adopt the revised artwork (once, this feature)

```batch
copy /Y temp\icon\newt-commander-icon.svg tools\brand\newt-commander-icon.svg
copy /Y temp\icon\png\*.png tools\brand\png\
```

Then re-author `tools/brand/logo.svg` from the new master (nanosvg-safe:
no filter, `width/height=256`, `userSpaceOnUse` gradients, no tile) and copy
it to `src/res/logo.svg` (keep the two in sync when editing).

## Regenerate the shipped icons

```batch
python tools\brand\gen_icons.py
```

Reads `tools/brand/png/newt-commander-icon-*.png` and writes, in place:

- `src/res/salamand.ico` (16–256 px)
- `src/res/sal_r.ico`, `sal_g.ico`, `sal_b.ico` (16+32 px, recolored folder)
- `src/salmon/res/salmon.ico`, `src/setup/res/setup.ico`,
  `src/setup/remove/icon1.ico` (16–256 px)

## Verify

1. **Structural** — ICO layout (entry sizes, 32-bpp, BMP ≤ 64 px /
   PNG ≥ 128 px) plus transparent corners (no tile remnant):

   ```batch
   python tools\brand\gen_icons.py --verify
   ```

2. **Build** — from repo root:

   ```batch
   build.cmd
   ```

3. **Visual** —
   - Explorer: build output → `newtcommander.exe` icon at all four view
     sizes; taskbar + Alt+Tab while running.
   - About: Help → About in light **and** dark theme — mark matches
     `tools/brand/png/newt-commander-icon-256.png` (minus the soft shadow)
     and stays legible with no own background tile.
   - Splash: enable splash screen, restart — folder mark top-right on the
     navy background.
   - Configuration → Main Window: cycle default/red/green/blue icons.
   - Crash reporter & installer/uninstaller executables show the revised
     icon.

## Icon cache note

If Explorer still shows the previous icon after a rebuild in the same
output path, it is the Windows icon cache — verify from a fresh copy of the
exe or clear the cache (`ie4uinit.exe -show`); not a defect.
