# Research: Manual Brand Asset Replacement (035)

No NEEDS CLARIFICATION markers remained in the Technical Context; research
consolidates the concrete mechanism choices.

## R1 — PNG decoding for About/splash artwork

**Decision**: Decode the `IDB_LOGO_IMAGE` RCDATA PNG with WIC
(`IWICImagingFactory` → `IWICStreamCreateMemStream`-style resource stream →
`IWICFormatConverter` to `GUID_WICPixelFormat32bppPBGRA` →
`IWICBitmapScaler` with `WICBitmapInterpolationModeFant` at target size) into
a top-down premultiplied 32-bpp DIB, drawn with `AlphaBlend`
(`AC_SRC_ALPHA`), mirroring how `CSVGSprite::AlphaBlend` is consumed today.
New helper `src/pngimage.cpp/.h` exposing a `CPngImage` with
`Load(resID, maxW, maxH)` + `GetSize` + `AlphaBlend` so `logo.cpp` call
sites change minimally (`svgHand` → `pngLogo`).

**Rationale**: WIC is inbox on Windows 11 (Principle IV — no new
dependency), already the project's precedent for image decoding (pictview
WIC engine, feature 006), and its Fant scaler gives high-quality downscale
of a 512 px master at arbitrary target sizes, which GDI `StretchBlt` does
not. Scaling at decode time (WIC scaler) beats decoding native size +
`HALFTONE` StretchBlt in both quality and code size.

**Alternatives considered**: GDI+ (`Gdiplus::Image`) — works, but pulls in
GDI+ startup/shutdown lifecycle management and the project nowhere else
initializes GDI+; keeping nanosvg SVG with a validator tool — rejected by
clarification (user chose raster); stb_image — new third-party code for
something the OS ships.

## R2 — Icon pipeline: master + optional per-size overrides

**Decision**: `gen_icons.py` reads `tools/brand/icon-master.png` (required;
square; ≥ 256 px, 1024 recommended) and optional `tools/brand/icon-<N>.png`
(N ∈ 16, 24, 32, 48, 64, 128, 256; must be exactly N×N). For each target
size the override wins when present, otherwise the master is downscaled with
`Image.LANCZOS`. Packs `salamand.ico`, `salmon.ico`, `setup.ico`,
`icon1.ico` (all FULL_SIZES) with the existing encoding rule (BMP ≤ 64 px,
PNG ≥ 128 px). `sal_r/g/b.ico` targets, the hue-remap code, `ORANGE_BAND`,
and `STATE_SIZES` are deleted. Validation failures (missing master,
non-square, too small, override wrong size, undecodable PNG) exit non-zero
with a message naming the file and the expected property. `--verify` keeps
the structural ICO check.

**Rationale**: Matches the clarified UX exactly — a wholesale swap is "drop
one file, run one command"; pixel-tuning small sizes stays possible. Lanczos
is Pillow's best downscale filter. Keeping the four remaining targets and
encoding rules means zero `.rc`/`.vcxproj` churn for icon consumers.

**Alternatives considered**: per-size-only (status quo) and master-only —
both rejected by clarification; generating ICOs at build time — violates
Principle I (build must not depend on Python/Pillow).

**Migration**: current `png/newt-commander-icon-{16..256}.png` are renamed
to `icon-{16..256}.png` (overrides) and `-1024` becomes `icon-master.png`,
so the first regeneration reproduces today's shipped icons bit-identically
(same source pixels, same packer encoding); `-512` render is dropped
(never packed, master supersedes it as the future-use large raster).

## R3 — Removing the red/green/blue window-icon variants

**Decision**: Shrink `MainWindowIcons[]` to the single default entry
(`MAINWINDOWICONS_COUNT` 4 → 1). `GetMainWindowIconIndex()` already clamps
out-of-range values to 0, which now covers stale registry values
(`Main window icon index`), the `-i N` command line, and multi-instance
tasklist reports — all silently degrade to the default icon (FR-003).
Remove: `IDI_SALAMANDER_RED/GREEN/BLUE` (resource.rh2 + salamand.rc2),
`sal_r/g/b.ico` files + vcxproj `<Image>` items, the "Main Window Icon"
groupbox/label/combo/hint from `IDD_CFGPAGE_MAINWINDOW` (lang.rc) and the
combo fill/transfer/HDPI-rebuild code in `dialogs5.cpp`
(`CreateIconCombo`/Transfer block), `IDC_TITLEBAR_ICON_INDEX` from lang.rh,
`IDS_SALAMANDERICON_RED/GREEN/BLUE` strings. The registry value keeps being
written (always 0) — harmless, avoids touching save/load plumbing.

**Rationale**: Clamping already exists, so removal is subtractive and safe;
no migration code needed. Leaving the registry write avoids modifying the
config save path for zero user benefit.

**Alternatives considered**: keeping auto-derivation with fallback and
optional manual variants — both rejected by clarification (variants
removed as a product feature).

## R4 — Splash copyright on two lines

**Decision**: Add `VERSINFO_COPYRIGHT1` ("Copyright © 1997-2026 Open
Salamander Authors") and `VERSINFO_COPYRIGHT2` ("© 2026 Newt Commander
Authors") to `versinfo.rh2`, keeping `VERSINFO_COPYRIGHT` (the concatenated
string) for the VERSIONINFO block (FR-010). `IDD_SPLASH`: copyright line 1
stays at y=73, new `IDC_SPLASH_COPYRIGHT2` static at y=83, status moves to
y=93, dialog height 94 → 104 dlgunits. `logo.cpp::PrepareBitmap` paints
both lines bold; `CSplashScreen` gains `Copyright2R`. `GradientY`
(anchored to `VersionR.bottom`) and the artwork placement are unaffected.

**Rationale**: Two resource-defined statics keep the layout in the dialog
template (translator-friendly, matches how every other splash text is
placed) instead of computing line breaks at runtime; the version-info
string stays a single line as Windows file-property UI expects.

**Alternatives considered**: `DrawText` with word-wrap into a taller rect —
splits at an arbitrary word boundary, not at the authorship boundary the
user asked for; shrinking the font — reduces legibility and still one line.

## R5 — About dialog copyright

**Finding**: `IDD_ABOUT` (lang.rc) already shows the two copyright parts on
two separate lines (`IDC_STATIC_1`/`IDC_STATIC_2`) — no change needed; the
fix is splash-only, confirming the spec's scope.
