# Validation Results: Restore Image Thumbnails in Thumbnail View

**Feature**: 048-fix-image-thumbnails · **Date**: 2026-08-02
**Build**: Debug x64 via `build.cmd` / `build.cmd full` (default
`OPENSAL_BUILD_DIR` = `.\build\`), all runs **BUILD SUCCEEDED, 0 errors**
**Method**: automated GUI drive (SendKeys with foreground verification) +
`PrintWindow` screenshots of the real `tandemcommander.exe`, on generated
test data (`%TEMP%\thumbtest`: JPEG/PNG-alpha/GIF/BMP/TIFF/ICO, corrupt
JPEG, text-as-.jpg, zero-byte PNG, plain .txt, 6000×4000 JPEG, and
`many\` with 100 × 1920×1080 JPEGs)

## Build gates

| Gate | Result |
|------|--------|
| Baseline `build.cmd` before changes (T001) | ✅ SUCCEEDED |
| `build.cmd` with fix (T004) | ✅ SUCCEEDED (`pictview.spl` relinked) |
| `build.cmd full` (runtime tree: 8 languages, `plugins.ver`) | ✅ SUCCEEDED |
| Final `build.cmd` after clang-format (T009) | ✅ SUCCEEDED |

## Scenario outcomes (quickstart.md)

| Scenario | Result | Evidence |
|----------|--------|----------|
| S1 previews for all formats | ✅ PASS | `photo.jpg`, `image.bmp`, `scan.tif`, `anim.gif` (frame 0), `big.jpg` (24 MP), `icon.ico` all render content previews; `graphic.png` alpha composited over panel background (not black) |
| S2 silent fallback | ✅ PASS | `fake.jpg`, `empty.png`, `notes.txt` keep standard icons; **zero error dialogs** during all runs; `corrupt.jpg` renders its decodable upper half with neutral fill — the upstream `HandleIncompleteImages` behavior (partial thumbnails were always by design; the truncated stream decodes partially, so this is content, not garbage) |
| S3 responsiveness | ✅ PASS | Immediate screenshot after entering `many\` (100 photos) caught the progressive state (first files per row thumbnailed, rest still icons, panel fully drawn); after 3.5 s all visible thumbnails rendered; `End` jumped instantly to `p100.jpg` with all 100 rendered; leaving mid-load via Backspace returned instantly with the parent's thumbnails intact and no stale entries |
| S4 both panels + switching | ✅ PASS | Both panels on the same folder show identical previews; Alt+3 → Alt+5 round-trip re-renders thumbnails |
| S5 thumbnail size config | ✅ PASS | Registry `Configuration\Thumbnail Size` 94 → 48: both panels render visibly smaller cells with correct previews; value restored afterwards |
| S6 fresh config first run | ✅ PASS | `HKCU\Software\Tandem Commander` exported + deleted; first run started clean (no import wizard, per feature 046), navigation + Alt+5 showed previews for every image **with zero manual configuration** — pictview auto-registered via `plugins.ver` and `Connect()` populated `ThumbnailMasks` (FR-002). Original registry restored from backup and verified |
| S7 unrelated PVSaveImage callers | ✅ PASS (viewer verified live; rest structural) | PictView viewer opens `photo.jpg` at 100 % (title reports 1600×1200×16.7M), image renders correctly. Save As / print preview / batch-JPEG-thumbnails remain rejected by the acceptance predicate (`PVC_UNSUP_OUT_PARAMS` — print always requests scaling, encoders request `PVF_JPG` etc.), so their behavior is unchanged by construction; the viewer File-menu screenshot was not capturable (`PrintWindow` cannot render popup menus) |
| S8 side-by-side parity | ⚠️ NOT EXECUTED | No Open Salamander installation available in the test environment. Parity is grounded in the code trace instead: the restored path feeds the identical `CSalamanderThumbnailMaker` pipeline (same shrinker, same painting) that upstream used; deliberate difference remains EXIF auto-rotation (dormant since feature 006 for viewer *and* thumbnails alike — see plan.md Scope Notes) plus formats WIC has no codec for (MNG/CDR/…) |

## Success criteria

- **SC-001** ✅ — 7 of 7 common-format images in the mixed folder and
  100 of 100 JPEGs in `many\` display content previews on a default
  (and on a completely fresh) installation.
- **SC-002** ✅ — panel navigable immediately after Alt+5 in the
  100-photo folder; previews fill progressively; no perceptible freeze.
- **SC-003** ✅ — mixed folder incl. corrupt image produced zero error
  dialogs; every non-previewable file kept its standard icon.
- **SC-004** ✅ (by construction; see S8) — same thumbnail pipeline as
  upstream; size/aspect/overlay handling is the untouched core code.

## Code changes validated

- `src/plugins/pictview/wicengine.cpp` — `WicSaveImage` implements the
  RAW/`PV_COLOR_TC32`/user-defined-output subset (natural size, no crop;
  `PVSF_SUPERFAST` ignored, `PVSF_FLIP_VERT` honored) by streaming the
  decoded DIB in bounded whole-row batches; short write and progress
  cancel are normal termination (`PVC_OK`), per
  `contracts/wicsaveimage-raw-subset.md`.
- `src/plugins/pictview/thumbs.cpp` — `LoadThumbnail` returns
  `code == PVC_OK` instead of unconditional `TRUE`.

## Notes & deviations

- `anim.gif` in the test set is single-frame (System.Drawing cannot
  author animated GIFs); multi-frame decoding is nevertheless covered by
  `icon.ico` (multi-image ICO → frame 0 rendered) and by the engine's
  `DecodeFrame(frame 0)` path shared with the viewer.
- `wicengine.cpp` has no UTF-8 BOM — pre-existing state inherited from
  feature 006, unchanged (constitution III: don't touch adjacent
  concerns); the file is pure ASCII.
- clang-format 17.0.3 (VS2022, repo `.clang-format`) run over both
  touched files produced no changes outside the feature's own hunks.
- Test data retained at `%TEMP%\thumbtest` for manual re-verification;
  registry config backup at the session scratchpad (`tc_backup.reg`),
  already restored and verified (`Thumbnail Size` = 94).
