# Contract: Replaceable Asset Layout (035)

The interface this feature exposes to its "user" (the maintainer swapping
graphics). `tools/brand/README.md` is the user-facing rendering of this
contract and MUST stay in sync with it.

## Replaceable files

| File | Appears as | Format | Constraint |
|------|-----------|--------|------------|
| `tools/brand/icon-master.png` | App icon everywhere (window top-left, taskbar, exe in Explorer, crash reporter, installer, uninstaller) | PNG | square, ≥ 256 px (1024 recommended) |
| `tools/brand/icon-<N>.png` (N = 16, 24, 32, 48, 64, 128, 256) | Overrides the master-derived rendering at exactly that icon size | PNG | exactly N×N; optional — delete to fall back to master |
| `tools/brand/about.png` | Artwork in About dialog and splash screen | PNG (alpha supported) | any size; ≈ 512 px long edge recommended; scaled to fit, aspect preserved |

## Replacement procedure (normative)

1. Replace the file(s) above.
2. Run `python tools/brand/gen_icons.py` (regenerates the four `.ico`
   files and copies `about.png` → `src/res/logo.png`).
3. Rebuild (`build.cmd`) and commit the changed files under `tools/brand/`,
   `src/res/`, `src/salmon/res/`, `src/setup/`.

Step 2 is the ONLY tool invocation; it requires Python 3 + Pillow on the
developer machine and is never part of the build.

## Non-replaceable (out of scope, fixed by code)

- "Newt Commander" wordmark (GDI-drawn text, `logo.cpp`)
- Blue→orange gradient accent strips (`src/res/gradspl.svg`, `gradabt.svg`)
- Toolbar/panel/file-type icons

## Stability guarantees

- File names and locations in this contract are stable; renaming any of
  them is a breaking change to the guide and requires updating
  `tools/brand/README.md` + `gen_icons.py` together.
- The four generated `.ico` paths and their size sets do not change
  (no `.rc`/`.vcxproj` edits ever needed for a swap).
