# Quickstart: Replace Application Icon (033)

## Prerequisites

- Python 3 + Pillow (asset-authoring only; NOT a build dependency)
- VS2022 + C++ workload (to build and visually verify)

## Regenerate the shipped icons

```batch
python tools\brand\gen_icons.py
```

Reads `tools/brand/png/newt-commander-icon-*.png` and writes, in place:

- `src/res/salamand.ico` (16–256 px)
- `src/res/sal_r.ico`, `sal_g.ico`, `sal_b.ico` (16+32 px, recolored folder)
- `src/salmon/res/salmon.ico`, `src/setup/res/setup.ico`,
  `src/setup/remove/icon1.ico` (16–256 px)

`src/res/logo.svg` is not generated — it is a hand-maintained nanosvg-safe
copy of `tools/brand/logo.svg` (keep the two in sync when editing).

## Verify

1. **Structural** — ICO layout matches the data model (entry sizes, 32-bpp,
   BMP ≤ 64 px / PNG ≥ 128 px):

   ```batch
   python tools\brand\gen_icons.py --verify
   ```

2. **Build** — from repo root:

   ```batch
   build.cmd
   ```

3. **Visual** —
   - Explorer: navigate to the build output, check `newtcommander.exe` icon
     at all four view sizes; check taskbar + Alt+Tab while running.
   - About: Help → About in light and dark theme — tile matches
     `tools/brand/png/newt-commander-icon-256.png` (minus the soft shadow).
   - Splash: enable splash screen, restart, check the tile top-right.
   - Configuration → Main Window: cycle default/red/green/blue icons.
   - Crash reporter & installer/uninstaller executables show the new icon.

## Icon cache note

If Explorer still shows the old icon after a rebuild in the same output
path, it is the Windows icon cache — verify from a fresh copy of the exe or
clear the cache (`ie4uinit.exe -show`); not a defect.
