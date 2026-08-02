# Research: Panel Thumbnail Chain — Root Cause Analysis

**Feature**: 048-fix-image-thumbnails
**Date**: 2026-08-02
**Method**: full-source trace of the thumbnail pipeline (panel → icon reader
→ plugin thumbnail loader), verification of plugins.ver/registry capability
propagation, and git/spec history review.
**Status**: root cause CONFIRMED (documented as a known follow-up in
feature 006's own validation report).

## Summary

The whole thumbnail chain is intact except the last step inside the
pictview plugin: the feature-006 WIC engine stubbed out the encoder entry
point (`PVSaveImage`) that the thumbnail path uses to deliver pixel rows.
The thumbnail maker therefore never receives a single scanline, the core
rejects the empty thumbnail, and the panel silently paints the shell icon.

## (a) The chain: panel thumbnail view → icon reader → plugin loader

1. **View mode selection (Alt+5)** — `src/fileswn2.cpp:1086-1088` maps
   index 4 to `newViewMode = vmThumbnails`.
2. **Directory read decides whether thumbnails are wanted** —
   `src/fileswn3.cpp:105` `BOOL readThumbnails = (GetViewMode() == vmThumbnails);`
   lines 111-126: `Plugins.AddThumbLoaderPlugins(thumbLoaderPlugins)`; empty
   list ⇒ `readThumbnails = FALSE`, `UseThumbnails = readThumbnails` (l. 126).
3. **Provider qualification** — `src/plugins2.cpp:1060-1081`
   `CPlugins::AddThumbLoaderPlugins()` selects plugins with non-empty
   `ThumbnailMasks` and `!ThumbnailMasksDisabled` (l. 1069-1070); requires
   `MainWindow != NULL && !MainWindow->DoNotLoadAnyPlugins` (l. 1064).
4. **Per-file match + on-demand plugin load** — `src/fileswn3.cpp:715-800`:
   mask match `p->ThumbnailMasks.AgreeMasks(file.Name, file.Ext)` (l. 725);
   lazy `InitDLL` (l. 742); failure drops the plugin from the list
   (l. 751-757). On success the icon-cache entry gets name/size/FILETIME +
   NULL-terminated array of `CPluginInterfaceForThumbLoaderEncapsulation*`
   (l. 780-798) and `iconData.SetFlag(4)` = thumbnail-not-yet-read (l. 800).
   Flag semantics: `src/icncache.h:15-29` (4 = not loaded, 5 = ok,
   6 = old/low quality).
5. **Icon reader thread calls the plugin** — `src/fileswn1.cpp:889-925`;
   l. 910: `(*loader)->LoadThumbnail(path, thumbnailSize, thumbnailSize,
   &thumbMaker, wanted == 4)`. On `TRUE` it sets flag 5/6, calls
   `thumbMaker.HandleIncompleteImages()` and **breaks** — no other plugin
   is tried (l. 912-914). Encapsulation: `src/plugins.h:350-395`
   (`NotEmpty()` l. 367, `LoadThumbnail()` l. 387-393), per-plugin storage
   `src/plugins.h:2510`, accessor `src/plugins.h:2530`.
6. **Result acceptance gate** — `src/fileswn1.cpp:1029-1067`: thumbnail is
   stored only if `thumbMaker.ThumbnailReady()` (l. 1031); only then
   `iconData->SetFlag(thumbnailFlag)` (l. 1042). `src/thumbnl.cpp:428-431`:
   `ThumbnailReady()` = `OriginalHeight != 0 && NextLine >= OriginalHeight
   && !Error`; `NextLine` only advances in `ProcessBuffer()`.
7. **Painting fallback** — `src/fileswn4.cpp:1342-1400`: bitmap drawn only
   when flag is 5 or 6 (l. 1366); a file stuck at flag 4 paints its shell
   icon.

## (b) pictview as thumbnail provider — what the WIC rewrite did

**Declaration survived intact:**
- `src/plugins/pictview/pictview.h:211-217` —
  `CPluginInterfaceForThumbLoader : CPluginInterfaceForThumbLoaderAbstract`.
- `src/plugins/pictview/pictview.cpp:29` singleton;
  `pictview.cpp:1245-1249` `GetInterfaceForThumbLoader()` returns it.
- `src/plugins/pictview/pictview.cpp:1176-1188` — `Connect()` calls
  `salamander->SetThumbnailLoader("*.mng;…;*.png;…;*.jpg;…;*.gif;…;*.bmp;…")`.
- pictview is the **only shipping** thumbnail loader: the other two
  (`demoplug.cpp:784`, `demoview.cpp:265`) are `off` in `plugins.cfg`.

**Implementation is present but functionally gutted:**
- `src/plugins/pictview/thumbs.cpp:835-1024` `LoadThumbnail`: opens the
  image fine (`PVOpenImageEx` l. 875 → `WicOpenImageEx`,
  `wicengine.cpp:619-620`), computes orientation, then produces pixels via
  `PVW32DLL.CreateThumbnail(...)` (l. 1015) — and **returns `TRUE`
  unconditionally at l. 1023, ignoring the result code**.
- `thumbs.cpp:1026-1042` `CreateThumbnail()`: `SetParameters(...)` OK, sets
  `PVSF_USERDEFINED_OUTPUT` with `MyWriteFunc`/`MySeekFunc`, then delegates
  all pixel production to **`PVW32DLL.PVSaveImage(...)`** (l. 1038).
  `MyWriteFunc` (`thumbs.cpp:119-125`) is what calls
  `thumbMaker->ProcessBuffer(...)`.

**The WIC rewrite replaced `PVSaveImage` with a no-op stub:**
- `src/plugins/pictview/wicengine.cpp:911-921` — `WicSaveImage` returns
  `PVC_UNSUP_OUT_PARAMS` immediately; banner comment "Graceful stubs -
  operations with no engine backing (feature 006 scope)"
  (`wicengine.cpp:897-899`); wired at `wicengine.cpp:973`.
- Plugin-side shrinker `Thumbnailer.cpp` is compiled out —
  `src/plugins/pictview/Thumbnailer.cpp:17` guards on
  `PICTVIEW_DLL_IN_SEPARATE_PROCESS || BUILD_ENVELOPE`, both dead after
  feature 006 retired salpvenv.
- `PVIsOutCombSupported` also returns 0 (`wicengine.cpp:923-930`) — the
  entire encoder side was intentionally disabled; thumbnails were
  collateral damage.

## (c) Capability propagation (plugins.ver / registry) — healthy

- `plugins.ver` carries **no** thumbnail flag by design. Generation:
  `build.cmd:405-433` (first line = monotonic integer, then
  `<ver>:<relative\path\plugin.spl>` lines). Parser
  `src/plugins2.cpp:2836-2929` (`SearchForAddedSPLs`) matches the format
  exactly.
- Auto-install `src/plugins2.cpp:2942-3020` (`ReadPluginsVer`) adds new
  `.spl`s (l. 2993) and force-loads every plugin (`InitDLL`, l. 3007);
  invoked from `src/salamdr1.cpp:4509`.
- Capability set at load time in `Connect()`: `src/plugins1.cpp:2365-2366`
  clears `ThumbnailMasks`; `CSalamanderConnect::SetThumbnailLoader`
  (`plugins1.cpp:1256-1283`) fills it, gated on
  `GetPluginInterfaceForThumbLoader()->NotEmpty()` (l. 1266);
  encapsulation initialized at `plugins1.cpp:2287`.
- Persistence: registry value `"ThumbnailMasks"` (`src/mainwnd2.cpp:569`),
  write `plugins2.cpp:1654-1657`, read `plugins2.cpp:1420-1428`.
- **Rebrand (fresh registry) ruled out as cause**: single root
  `Software\Tandem Commander\0.1` (`mainwnd2.cpp:157-160`,
  `consts.h:2119-2121`); fresh registry ⇒ `LastPluginVer == 0` ⇒
  plugins.ver always newer ⇒ all plugins auto-install and load on first
  run, and `Connect()` repopulates `ThumbnailMasks`. The earlier
  plugins.ver regression was fixed in feature 006
  (`specs/006-fix-pictview-plugin/validation-results.md:113-133`;
  explanatory comment `build.cmd:388-403`).

## (d) Root cause (confirmed)

1. `LoadThumbnail` → `CreateThumbnail` → `PVW32DLL.PVSaveImage`
   (`thumbs.cpp:1038`).
2. `PVSaveImage` = `WicSaveImage` stub → returns `PVC_UNSUP_OUT_PARAMS`
   without ever invoking `pSii->WriteFunc` (`wicengine.cpp:911-921`).
3. `MyWriteFunc` never runs ⇒ `ProcessBuffer` never called ⇒
   `NextLine == 0`.
4. `LoadThumbnail` still returns `TRUE` (`thumbs.cpp:1023`) ⇒ icon reader
   breaks out of the loader list (`fileswn1.cpp:914`) — failure is
   completely silent.
5. `ThumbnailReady()` false (`NextLine (0) < OriginalHeight`,
   `thumbnl.cpp:428-431`) ⇒ gate `fileswn1.cpp:1031` rejects ⇒ flag stays 4.
6. `fileswn4.cpp:1366` skips the bitmap path ⇒ shell icon painted.

Documented as a known regression in
`specs/006-fix-pictview-plugin/validation-results.md:108-111`:
> "Thumbnails: the panel thumbnail path still calls `CreateThumbnail` →
> `PVSaveImage` (stubbed) so thumbnails are not produced by the WIC engine
> yet — a follow-up (the WIC decoder can fill the RAW 32bpp buffer
> directly). Not required for US1–US3."

Introducing commit: `e3666ab` "[006] Full WIC viewer engine
(open/decode/draw/rotate/pixel-access), disable encoder-backed features,
retire salpvenv from solution".

## (e) Fix shape (for the plan phase)

- Implement a real WIC-backed pixel producer for the thumbnail path:
  decode + scale (e.g. `IWICBitmapScaler`, or decode full-size) and feed
  `thumbMaker->GetBuffer()/ProcessBuffer()` with BGRA32 rows until
  `NextLine == OriginalHeight`. Two viable placements:
  (1) implement `WicSaveImage` for the `PVSF_USERDEFINED_OUTPUT` +
  `PVF_RAW`/`PV_COLOR_TC32` case, or (2) bypass `PVSaveImage` in
  `CreateThumbnail` (`thumbs.cpp:1026-1042`) entirely.
- Independently: `LoadThumbnail` should `return code == PVC_OK` instead of
  unconditional `TRUE` (`thumbs.cpp:1023`) so failures fall through to the
  next loader and become diagnosable.
- `TProgressProc` polarity fix from commit `61c0dab` is correct
  (`thumbs.cpp:521-527`, return TRUE cancels) but currently unreachable —
  becomes live again once the producer works.

## (f) Other load-bearing details

- **No user-facing on/off switch** for which files get thumbnails; the
  only setting is thumbnail *size* (`src/dialogs5.cpp:2854-2858`
  `IDC_THUMBNAILSIZE`, clamped `THUMBNAIL_SIZE_MIN..MAX`; persisted
  `mainwnd2.cpp:1732`, `3286-3288`). No hardcoded extension list in core —
  the eligible set is the union of plugin `ThumbnailMasks`.
- **Fast live diagnostic**: Plugins Manager shows the mask list
  (`src/dialogs5.cpp:305-319`, `IDC_PLUGINTHUMBNAILS` /
  `IDS_PLUGINTHUMBNONE`). Long mask list for PictView ⇒ registration fine,
  break inside plugin (the confirmed state); "none" ⇒ registration issue.
- **Reader-side skip conditions** (all currently benign): archives
  preferred over thumbnails (`fileswn3.cpp:716-718`); disk-only, never
  directories (`fileswn4.cpp:1346`, `fileswn3.cpp:717`);
  `DoNotLoadAnyPlugins` disables all providers (`plugins2.cpp:1064`);
  `ThumbnailMasksDisabled`/`StopThumbnailLoading`/`UseThumbnails=FALSE`
  set during plugin unload (`fileswn1.cpp:1567-1583`), reset in
  `plugins1.cpp:2856`, `plugins1.cpp:3162`; paths > `MAX_PATH` skipped
  with `TRACE_I` (`fileswn1.cpp:920-924`).
- **`HandleIncompleteImages()` cannot mask the failure**
  (`src/thumbnl.cpp:593-616`): white-fill needs ≥ ~3 delivered rows,
  false at `NextLine == 0` — consistent with "plain icons" symptom
  (nothing drawn, not a white tile).
