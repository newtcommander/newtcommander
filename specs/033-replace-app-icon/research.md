# Research: Replace Application Icon (033)

All unknowns from the Technical Context resolved. Facts were gathered
directly from the repository (resource scripts, `logo.cpp`, `gen_icons.py`,
ICO binary headers) and from the delivered assets in `temp/icon/`.

## R1. How to produce the shipped ICO files from the new artwork

**Decision**: Rewrite `tools/brand/gen_icons.py` from a procedural
rasterizer into a **PNG→ICO packer**. The delivered PNG renders
(16–1024 px) become the committed, authoritative rasters under
`tools/brand/png/`; the script loads them with Pillow and packs the needed
sizes into each ICO using the existing `write_ico` convention (32-bpp,
BMP-encoded entries ≤ 64 px, PNG-encoded entries ≥ 128 px).

**Rationale**: The current script redraws the *old* "Split Disc" geometry in
pure Pillow — left unchanged it would silently resurrect the old design on
the next regeneration (spec FR-009). The new master SVG uses `feDropShadow`,
multi-stop gradients, and rotated document shapes; reproducing that
procedurally would be a large, error-prone re-implementation with no
benefit, while the prepared PNGs are pixel-exact by definition (spec
assumption: delivered renders are authoritative, including small sizes —
verified legible at 16/32 px).

**Alternatives considered**:
- *Procedural redraw of the new design* — rejected: high effort, cannot
  faithfully reproduce the SVG filter effects, permanent maintenance burden.
- *Rasterize the SVG at generation time (resvg/cairosvg/Inkscape)* —
  rejected: adds a new external tool dependency for asset authoring;
  the delivered PNGs already exist and are approved.
- *`Pillow Image.save(..., format="ICO")`* — rejected: less control over
  per-entry encoding (BMP vs PNG) than the existing proven `write_ico`.

## R2. In-app icon tile for About dialog and splash screen

**Decision**: Hand-author a **nanosvg-safe variant** of the new icon and
ship it as `src/res/logo.svg` (`IDB_LOGO_HAND`), with the same file kept in
`tools/brand/logo.svg`. Simplifications relative to the master SVG:
- drop the `feDropShadow` filter (nanosvg has no filter support),
- eliminate `clipPath` by drawing the tile as plain rounded rects (edge
  rect + inset radial-gradient rect, same construction the 032 logo.svg
  used) and dropping/approximating the 6 %-opacity top-half sheen,
- keep linear/radial gradients and the `rotate(±4°)` document transforms —
  both supported by nanosvg.

**Rationale**: `logo.cpp` loads `IDB_LOGO_HAND` via `CSVGSprite` (nanosvg)
and scales it by height (splash, line ~218) or into a rect (About, line
~445); the artwork is square in both old (96 viewBox) and new (256 viewBox)
versions, so **no C++ change is needed** — only the asset. The spec's edge
case explicitly allows a simplified variant without unsupported effects as
long as shapes, colors, and proportions match the master.

**Alternatives considered**:
- *Ship the master SVG unchanged* — rejected: nanosvg ignores/breaks on
  `filter` and `clipPath`, producing an unfaithful render.
- *Render the tile from a raster PNG resource* — rejected: requires C++
  changes in two places and loses resolution independence on high-DPI.

## R3. Red / green / blue main-window icon variants

**Decision**: Generate `sal_r.ico`, `sal_g.ico`, `sal_b.ico` (16 + 32 px,
same sizes as today) inside the packer by **hue-remapping the saturated
orange pixels** of the base renders: pixels whose HSV hue falls in the
orange band (≈ 15°–50°) *and* whose saturation exceeds a threshold (≈ 0.45,
so the pale cream pill/dot and paper highlights stay untouched) get their
hue replaced by the target (red ≈ 0°, green ≈ 130°, blue ≈ 215°), keeping
saturation/value — the folder gradient shading survives the remap. Exact
band/thresholds tuned during implementation against the 16/32 px renders.

**Rationale**: Preserves the existing user-facing configuration feature
(Configuration → Main Window, `MainWindowIcons[]` in `dialogs5.cpp` —
resource IDs `IDI_SALAMANDER_RED/GREEN/BLUE` unchanged) with variants that
are unmistakably the same design; a 32×32 loop in pure Python/Pillow is
trivial (no numpy needed).

**Alternatives considered**:
- *Colored badge/dot overlay* — rejected: illegible at 16 px.
- *Whole-image tint* — rejected: discolors the navy tile and white papers.
- *Three hand-made SVG variants* — rejected: triples artwork maintenance.

## R4. Which sizes each ICO ships

**Decision** (current entry layouts read from the binaries):

| File | Current entries | New entries |
|------|-----------------|-------------|
| `src/res/salamand.ico` | 16,24,32,48,64,128,256 (32bpp) | same set, new art |
| `src/res/sal_r/g/b.ico` | 16,32 (32bpp) | same set, hue-remapped |
| `src/salmon/res/salmon.ico` | 32,16 (32bpp) | 16,24,32,48,64,128,256 |
| `src/setup/res/setup.ico` | 16,16,32,32 (4/8bpp legacy) | 16,24,32,48,64,128,256 |
| `src/setup/remove/icon1.ico` | 32 (8bpp legacy) | 16,24,32,48,64,128,256 |

**Rationale**: The main icon keeps its proven size set. The companion
programs get the full modern set — their current icons are legacy low-color
Open Salamander art from the initial import; a full 32-bpp set renders
correctly in Explorer at every view size and in Apps & Features, at a cost
of a few tens of KB. ICO size is irrelevant to runtime performance.

**Alternatives considered**: keeping companion icons at their old minimal
size sets — rejected: no benefit, and 16 px-only/legacy-bpp icons look bad
in modern Explorer views.

## R5. Scope sweep — no other surfaces embed the application icon

**Decision**: The complete consumer list is: `src/salamand.rc2` (IDI_SALAMANDER
+ RED/GREEN/BLUE + IDB_LOGO_HAND), `src/salmon/salmon.rc`, `src/setup/setup.rc`,
`src/setup/remove/remove.rc2`. No other `.rc`/`.rc2` references the
application icon; the remaining icons in `src/res/` (directory, archive,
plugin, viewer, cloud folders, updir, find, empty…) are file-type/function
icons, out of scope per spec. `IDB_LOGO_GRAD`/`IDB_ABOUT_GRAD` (gradient
accent) and the wordmark lockups stay unchanged per FR-010. The shell
extension has no icon resource of its own.

**Rationale**: Verified by grep across `src/**/*.rc*` for `.ico` references
and by `IDI_SALAMANDER*`/`IDB_LOGO_HAND` usage search in `*.cpp`.

## R6. Handling of the delivered 512/1024 px renders

**Decision**: Commit them to `tools/brand/png/` alongside the shipped sizes
but do not pack them into any ICO (Windows uses ≤ 256 px in ICO resources).

**Rationale**: Keeps the full approved render set in the source of truth
for future needs (store artwork, website, high-DPI docs) at negligible repo
cost; shipping them would bloat every ICO for no OS-visible benefit (spec
assumption: sizes above 256 px need not ship).

## R7. Verification approach

**Decision**: Three layers:
1. **Structural**: Python check that every regenerated ICO has the expected
   entry count, dimensions, and 32-bpp depth, and that PNG-vs-BMP encoding
   follows the ≤64/≥128 px convention.
2. **Build**: `build.cmd` Debug x64 must succeed; resources embed unchanged
   IDs.
3. **Visual**: extract the icon from the built `newtcommander.exe`
   (SHGetFileInfo-equivalent or screenshot of Explorer), and drive the app
   headlessly (`-l`/`-r` + `WM_COMMAND`, per the established debug toolkit;
   `SendInput` is blocked in this environment) to open Help → About and
   screenshot it in light and dark themes; screenshot the splash. Compare
   against `temp/icon/png/newt-commander-icon-256.png`.

**Rationale**: Mirrors the verification style used by feature 032
(`validation-results`, screenshots) and the project's headless GUI smoke
conventions.
