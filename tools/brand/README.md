# Newt Commander Brand Assets — How to Swap the Graphics

This directory is the single place to change the application's graphics.
Every swap is: **replace a file here → run one command → rebuild.** No
source-code, resource-script, or project-file edits are ever needed, and no
knowledge of the app's internals is required.

## What can be replaced

| File | Where it appears | Format & size |
|------|------------------|---------------|
| `icon-master.png` | Application icon everywhere: main-window top-left corner, taskbar, `newtcommander.exe` in Explorer, crash reporter (salmon), installer, uninstaller | PNG, **square**, edge ≥ 256 px (1024×1024 recommended). All icon sizes are derived from it automatically (high-quality Lanczos downscale). |
| `icon-16.png`, `icon-24.png`, `icon-32.png`, `icon-48.png`, `icon-64.png`, `icon-128.png`, `icon-256.png` | Optional per-size overrides of the master | PNG, exactly N×N for `icon-<N>.png`. If present, the file wins over the master-derived rendering at that size — useful for hand-tuning tiny sizes. **Delete them when swapping the icon wholesale**, otherwise the old artwork stays at those sizes. |
| `about.png` | Artwork in the About dialog (Help → About) and on the startup splash screen | PNG, alpha supported, any size (≈ 512 px on the long edge recommended). The app scales it to fit at draw time, aspect ratio preserved — no distortion, no cropping. |

## How to swap

1. Replace the file(s) above with your new artwork.
2. Regenerate the shipped assets:

   ```batch
   python tools\brand\gen_icons.py
   ```

   This rewrites `src/res/salamand.ico`, `src/salmon/res/salmon.ico`,
   `src/setup/res/setup.ico`, `src/setup/remove/icon1.ico` and copies
   `about.png` to `src/res/logo.png`. Inputs are validated first — a bad or
   missing file stops the run with an `error:` line naming the file and the
   expected property, and nothing is written.
3. Rebuild and check the result:

   ```batch
   build.cmd
   ```

   Then look at: window top-left icon, taskbar, the exe in Explorer,
   Help → About, and the splash screen (enable it under Options →
   Configuration → Main Window if turned off).

Commit the changed files under `tools/brand/`, `src/res/`,
`src/salmon/res/` and `src/setup/`.

Optional structural self-check of the committed outputs (no writes):

```batch
python tools\brand\gen_icons.py --verify
```

**Requirements**: Python 3 with Pillow (`pip install pillow`) on the
developer machine only — the build itself never runs Python; the generated
files are committed.

## What is NOT replaceable here (drawn by code)

- The "Newt Commander" wordmark in About/splash — GDI-drawn text
  (`src/logo.cpp`, `NCDrawWordmark`), no font shipped.
- The blue→orange accent strips in About/splash —
  `src/res/gradspl.svg` / `gradabt.svg` (source: `gradient-band.svg`).
- Toolbar, panel, and file-type icons.

## Reference files (not shipped)

| File | Purpose |
|------|---------|
| `newt-commander-icon.svg` | Master vector of the current folder icon (feature 034) — the PNG renders were exported from it |
| `newt-commander-lockup-dark.svg` / `-light.svg` | Horizontal logo lockups (live Archivo text) |
| `gradient-band.svg` | Source of the shipped accent strips |

## Technical notes

- ICO layout: entries 16, 24, 32, 48, 64, 128, 256 px, all 32-bpp;
  BMP-encoded ≤ 64 px, PNG-encoded ≥ 128 px (Windows convention).
- `about.png` is decoded at runtime by Windows WIC and alpha-blended, so
  any valid PNG (including full alpha) works; there are no other
  constraints on its content.
- The red/green/blue main-window icon variants were removed in feature 035;
  the icon pipeline no longer generates `sal_r/g/b.ico`.
