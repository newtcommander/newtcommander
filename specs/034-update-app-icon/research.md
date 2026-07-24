# Research: Update Application Icon to Revised Artwork (034)

All unknowns resolved. Facts were gathered from the repository (033
artifacts, `gen_icons.py`, `logo.cpp`, resource scripts) and from the
delivered assets in `temp/icon/`. Where a 033 decision still applies it is
carried forward and only the delta is researched.

## R1. Design delta between the 033 master and the revised master

**Decision**: Treat the revision as *tile removal + folder enlargement*.
Byte-compare and visual inspection of `temp/icon/newt-commander-icon.svg` vs
`tools/brand/newt-commander-icon.svg` show:

- the `clipPath`-clipped dark tile (edge `#02060D`, radial `#1B3054→#070E1A`,
  6 %-white sheen) is **removed entirely** — background is transparent;
- the folder group is enlarged and slightly stretched:
  `translate(128 133) scale(1.28 1.38)` vs 033's `scale(1.16)`;
- folder, papers, text lines, cream pill/dot keep **identical** colors and
  geometry; the `feDropShadow` soft shadow remains in the master.

Delivered PNGs verified with Pillow: all 9 sizes present and exact, RGBA,
corners and edges fully transparent (16 px has a ≤ 4 % alpha shadow spill at
the top edge — part of the approved render, kept as delivered).

**Rationale**: Knowing the delta is what confirms the rest of the 033
pipeline survives unchanged (R2–R4) and pinpoints the only re-authored
asset (`logo.svg`, R3).

## R2. How to produce the shipped ICO files

**Decision**: **Reuse `tools/brand/gen_icons.py` unchanged** (logic-wise):
replace the committed rasters in `tools/brand/png/` with the revised set and
re-run the packer. Only the module docstring / comments that describe the
artwork ("folder on a dark navy rounded tile", "feature 033") are refreshed
to describe the revised design and reference 034.

**Rationale**: Feature 033 already converted the generator into a pure
PNG→ICO packer precisely so artwork revisions become input swaps (033
research R1). The packing convention (32-bpp, BMP ≤ 64 px, PNG ≥ 128 px,
size sets per target) is independent of the artwork.

**Alternatives considered**: none warranted — the packer's contract is
"pack whatever authoritative rasters sit in `png/`".

## R3. In-app mark for About dialog and splash screen (`logo.svg`)

**Decision**: Re-author the nanosvg-safe derivative from the revised master:

- keep the folder group exactly as in the master, including the non-uniform
  `translate(128 133) scale(1.28 1.38) translate(-128 -133)` group transform
  and the `rotate(±4°)` document transforms (all supported by the bundled
  nanosvg, same transform kinds as the verified 033 file);
- drop the `feDropShadow` filter (no nanosvg filter support — spec edge case
  explicitly allows this);
- **delete** the 033 tile reconstruction (edge rect + inset radial rect +
  sheen) — nothing replaces it; the document root has no background;
- set `width="256" height="256"` equal to the viewBox (the delivered master
  says `width="1024"` — shipping that verbatim would trigger the known
  nanosvg gradient-scaling bug documented in `tools/brand/README.md`);
- convert the four folder gradients to `gradientUnits="userSpaceOnUse"` with
  explicit local coordinates (fold y 104→192, back y 74→124, shade y
  104→192, gloss y 104→148) — the proven 033 pattern; the tile radial
  gradient is no longer needed.

Verification: rasterize the new `logo.svg` with the bundled nanosvg at
256 px and pixel-diff against `temp/icon/png/newt-commander-icon-256.png`
(shadow-masked), as done for 033.

**Rationale**: `logo.cpp` scales the square SVG by height (splash, ~218) or
into a rect (About, ~445); the canvas stays 256×256 square, so no C++
change. The splash always paints the brand navy background (`logo.cpp:208`,
`NC_COLOR_NAVY`), so the tile-less orange folder stays legible there; in
About the mark now sits directly on the theme background — orange folder
with white papers reads well on both light and dark (to be confirmed by the
screenshot smoke, SC-002).

**Alternatives considered**:
- *Ship the master SVG unchanged* — rejected: `filter` unsupported and the
  width/viewBox mismatch breaks gradient scaling in the bundled nanosvg.
- *Keep the 033 tile in-app for contrast* — rejected: would contradict the
  approved revision; About/splash must read as the same artwork as the exe
  icon (spec US2, acceptance 3).

## R4. Red / green / blue main-window icon variants

**Decision**: Keep the 033 hue-remap approach and tuning verbatim
(`ORANGE_BAND = 15°–50°`, `SAT_THRESHOLD = 0.45`, targets red 0° / green
142° / blue 213°) applied to the revised 16/32 px renders. Re-check the
result visually: with the navy tile gone, *all* opaque pixels are folder,
paper, or pill/dot — papers (`#EAF2FB`/`#FDFEFF`, near-zero saturation) and
the cream pill/dot (below the saturation threshold — unchanged colors,
verified by the 033 tuning) stay neutral.

**Rationale**: The folder palette is byte-identical to 033, so the tuned
band/threshold transfer directly; the variants become *more* distinguishable
since recolored pixels now dominate the icon area.

**Alternatives considered**: re-tuning the band — only if the visual check
finds strays; not expected.

## R5. Scope sweep — consumers unchanged since 033

**Decision**: The consumer list is identical to 033 (verified — no commits
touched `*.rc*`, `logo.cpp`, or `dialogs5.cpp` since): `src/salamand.rc2`
(IDI_SALAMANDER + RED/GREEN/BLUE + IDB_LOGO_HAND), `src/salmon/salmon.rc`,
`src/setup/setup.rc`, `src/setup/remove/remove.rc2`. File-type/function
icons in `src/res/` and the gradient accent / wordmark assets stay out of
scope (FR-010).

## R6. Delivered 512/1024 px renders

**Decision**: Carried forward from 033 R6 — commit to `tools/brand/png/`,
do not pack into any ICO.

## R7. Verification approach

**Decision**: Same three layers as 033 (R7), with one addition targeting
the design delta:

1. **Structural**: `python tools\brand\gen_icons.py --verify` (entry
   counts/sizes/bpp/encoding) **plus** a transparency spot-check that ICO
   frames keep fully transparent corners (no tile remnant — SC-001).
2. **Build**: `build.cmd` Debug x64 must succeed with unchanged resource IDs.
3. **Visual**: Explorer check of `newtcommander.exe` (+ salmon, setup,
   remove) icons; headless `-l`/`-r` + `WM_COMMAND` About screenshots in
   light **and** dark theme (mark legibility without own tile is the new
   risk); splash screenshot; Configuration → Main Window variant cycling;
   nanosvg pixel-diff of `logo.svg` vs the 256 px master render.
