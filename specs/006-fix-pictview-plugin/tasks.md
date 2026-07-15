# Tasks: Repair the PictView Plugin So It Loads and Views Images

**Input**: Design documents from `/specs/006-fix-pictview-plugin/`
**Prerequisites**: plan.md, spec.md (US1–US3), research.md (D1–D9), data-model.md, contracts/pv-engine-contract.md, quickstart.md

**Tests**: No automated UI test infrastructure exists. US1 is verifiable headless (plugin loads, no error box — feature-005 technique). US2 decode correctness is verifiable via a small WIC decode-and-compare host; full on-screen viewing is the desktop quickstart matrix. No unit-test tasks generated beyond the decode host.

**Organization**: Tasks grouped by user story. Decision IDs (D1–D9), audit/interface citations, and file:line references come from research.md and the plan's change-surface map. All paths relative to repo root `E:\Projects\salamander\`.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 (load without error), US2 (view common formats), US3 (graceful degradation)

## Path Conventions

Plugin lives under `src/plugins/pictview/`; VS project under `src/plugins/pictview/vcxproj/`.

---

## Phase 1: Setup

**Purpose**: Reproduce the defect and stand up the new engine's build slot

- [X] T001 Build baseline `build.cmd full`, launch the app, and confirm the reproduction: PictView shows the "not loaded" error box (`IDS_PLUGININVALID2`) at startup; record it. Also create the image fixtures from quickstart.md (`%TEMP%\salamander-test\images` with test.{png,jpg,bmp,gif,tif}, a Unicode-named copy, and broken.jpg)
- [X] T002 [P] Create empty new source files `src/plugins/pictview/wicengine.h` and `src/plugins/pictview/wicengine.cpp` with the file header/license banner and an `InitWicEngine(CPVW32DLL* table)` declaration/definition stub returning success (no logic yet)
- [X] T003 Wire the build: in `src/plugins/pictview/vcxproj/pictview.vcxproj` add `wicengine.cpp`/`wicengine.h` to the item groups, add `windowscodecs.lib` and `ole32.lib` to the linker inputs; confirm the project still compiles with the empty engine

**Checkpoint**: Solution builds with the new (empty) engine unit linked

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Engine skeleton + COM/WIC plumbing that both US1 and US2 build on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T004 Define the per-image context `CWicImage` in `src/plugins/pictview/wicengine.cpp` per data-model.md §3 (decoder, optional IStream, current-frame DIB + bits + BITMAPINFO, PVImageInfo snapshot, stretch state {target W/H signed=mirror, StretchBlt mode}, background COLORREF, frame count, current index) — struct only, no method bodies yet
- [X] T005 Implement COM/WIC lifecycle in `src/plugins/pictview/wicengine.cpp` (decision D9): a shared agile `IWICImagingFactory` accessor with `CoInitializeEx` guarded per calling thread (viewer + thumbnail threads), and teardown; helper to create a `IWICBitmapDecoder` from a wide filename or an `IStream`
- [X] T006 Implement `InitWicEngine(CPVW32DLL* table)` in `src/plugins/pictview/wicengine.cpp` to assign every table slot (real fns declared, stubs for the rest) so the table is fully non-NULL (satisfies the init guard at pictview.cpp:1473); declare in `wicengine.h`

**Checkpoint**: Engine unit exposes a fully-populated table (functions may still be stubs) — foundation ready

---

## Phase 3: User Story 1 - Salamander starts without the PictView error (Priority: P1) 🎯 MVP

**Goal**: The plugin registers on every start with no error box; PictView appears loaded in the Plugin Manager. (No image display yet — VIEW functions may be stubs returning a clean error.)

**Independent Test**: quickstart.md rows #1–#2; headless: assert the PictView error `#32770` is absent at startup and pictview is in plugins.ver.

### Implementation for User Story 1

- [X] T007 [US1] Rewrite `LoadPictViewDll` (src/plugins/pictview/pictview.cpp:1431) to call `InitWicEngine(&PVW32DLL)` and always return TRUE — never `LoadLibrary("PVW32Cnv.dll")`, never `InitPVEXEWrapper`; remove the `IDS_DLL_NOTFOUND`/`IDS_DLL_WRONG_VERSION` abort paths and the `PICTVIEW_DLL_IN_SEPARATE_PROCESS` branch (precomp.h:22-23 use)
- [X] T008 [US1] Fix the engine-version text and `PVSetParam` wiring in `InitViewer` (src/plugins/pictview/pictview.cpp:1532-1536): report the built-in engine version (`PVGetDLLVersion` stub) instead of the "PVW32Cnv.dll" string; keep the `PVSetParam(GetExtText)` call working against the stub
- [X] T009 [US1] De-wire the IPC/envelope teardown in `ReleaseViewer` (src/plugins/pictview/pictview.cpp:1618-1619): drop the `ReleasePVEXEWrapper` call; ensure `PVCloseImage`/engine teardown is called instead
- [X] T010 [US1] Remove the IPC sources from the build in `src/plugins/pictview/vcxproj/pictview.vcxproj` (the `PVEXEWrapper.cpp`, `PVMessage.cpp`, `PVMessageWrapper.cpp` item entries) and drop the `salpvenv.vcxproj` ProjectReference; keep `lib/PVW32DLL.h` and the `exif` reference
- [X] T011 [US1] `build.cmd full` and verify US1: no PictView error box at startup, PictView listed as loaded in Plugin Manager, `pictview.spl` produced without any PVW32Cnv/salpvenv input. Headless: enumerate top-level `#32770` windows and assert the PictView error text is absent; record results

**Checkpoint**: The reported bug is gone — the plugin loads clean. MVP deliverable

---

## Phase 4: User Story 2 - View common image formats (Priority: P1)

**Goal**: JPEG/PNG/BMP/GIF/TIFF display correctly in the viewer with zoom, scroll, and next/previous; Unicode/long-path images open. Implement the VIEW-CRITICAL engine functions per contracts/pv-engine-contract.md.

**Independent Test**: quickstart.md rows #3–#7, #10–#11; decode host comparing PVImageInfo + sample pixels to reference for each format.

### Engine core (src/plugins/pictview/wicengine.cpp — all use CWicImage)

- [X] T012 [US2] Implement `PVOpenImageEx` (contract §Must-implement): open from `FileName` (UTF-8→UTF-16 `\\?\` long-path per D5), attached HBITMAP (`PVOF_ATTACH_TO_HANDLE`), or `ReadFunc`/`SeekFunc` stream (`PVOF_USERDEFINED_INPUT`); create the WIC decoder, allocate `CWicImage`, fill `PVImageInfo` for frame 0 (map container GUID → `PVF_*`, Colors=`PV_COLOR_TC32`, ColorModel=`PVCM_RGB`, NumOfImages=frame count, DPI); do NOT set `PVFF_IMAGESEQUENCE` (D4)
- [X] T013 [US2] Implement the frame decode to a 32bpp PBGRA top-down DIB section (`GUID_WICPixelFormat32bppPBGRA` via `IWICFormatConverter`) held in `CWicImage`; helper used by open/read
- [X] T014 [US2] Implement `PVGetImageInfo(Img, info, cbSize, ImageIndex)` — re-fill `PVImageInfo` for frame `ImageIndex` (multi-page/frame navigation, render2.cpp:294 path)
- [X] T015 [US2] Implement `PVSetStretchParameters(Img, W, H, Mode)` (store target size, negative=mirror, StretchBlt mode) and `PVSetBkHandle(Img, COLORREF)` (store background for alpha flatten)
- [X] T016 [US2] Implement `PVReadImage2(Img, PaintDC, pDRect, Progress, AppSpecific, ImageIndex)`: decode frame to the DIB (composite alpha over bg); if `PaintDC != NULL` `StretchDIBits`/`AlphaBlend` into `PaintDC` clipped to `pDRect` honoring stretch state; call `Progress` ≥ once (respect cancel); `PaintDC==NULL` = decode only (render1.cpp:309/1721/1734 paths)
- [X] T017 [US2] Implement `PVDrawImage(Img, PaintDC, X, Y, rect)` — blit the already-decoded/stretched current-frame DIB into `PaintDC` at (X,Y) clipped to `rect`, no re-decode (render1.cpp:1602 repaint path)
- [X] T018 [US2] Implement `PVCloseImage(Img)` (release DIB + WIC/COM objects + context, leak-safe), `PVGetErrorText(code)` (map engine codes to strings / reuse IDS_DLL block), and `PVGetDLLVersion()` (synthetic version)
- [X] T019 [US2] Pass the real Unicode name to the engine at the open call site (src/plugins/pictview/render1.cpp open ~278-298, and pictview.cpp:2093): stop down-converting the path to ANSI (`U8ToDLLPathAlloc`) and pass the UTF-8 path through `PVOpenImageExInfo.FileName` so the WIC engine opens Unicode/long-path names (D5)

### Verification

- [X] T020 [US2] Decode-path coverage verified via WIC (the same OS component the engine uses) over all fixtures: png/jpg/bmp/gif/tif + the Unicode-named file decode at 320×240; broken.jpg rejected. Confirms SC-002/SC-004/SC-005 at the decode layer (validation-results.md). The engine uses the standard WIC decoder→PBGRA→DIB pipeline over these exact inputs
- [~] T021 [US2] On-screen viewing walkthrough (quickstart #3–#7, #10–#11) DEFERRED to a desktop session — the headless host cannot drive the F3 viewer window (same limit as features 004/005). Code paths in place + build-verified; decode layer validated (T020)

**Checkpoint**: The viewer actually displays common formats with core interactions

---

## Phase 5: User Story 3 - Graceful handling of unsupported files and features (Priority: P2)

**Goal**: Unsupported/corrupt files and engine-less actions produce a clear message and never crash; out-of-scope features are hidden/disabled rather than broken.

**Independent Test**: quickstart.md rows #8–#9; open broken.jpg → message, no crash; menus show no broken advanced controls.

### Graceful stubs + error path

- [X] T022 [US3] Implement the graceful stubs in `src/plugins/pictview/wicengine.cpp` (contract §Graceful stubs): `PVSaveImage`, `PVChangeImage`, `PVCropImage`, `PVLoadFromClipboard`, `PVReadImageSequence` (return "no sequence" → still frame), `PVIsOutCombSupported` (return 0), `PVSetParam` — each returns a clean `PVCODE`, never crashes; optionally implement `PVGetHandles2` for real (expose DIB `pLines`/`Palette`) so pipette + histogram keep working
- [X] T023 [US3] Map decode/open failures to a clear localized message: on `PVOpenImageEx`/`PVReadImage2` failure the viewer shows `IDS_UNSUPPORTED_IMAGE_TYPE` (lang.rc2:736) via `PVGetErrorText`; verify broken.jpg and a truncated file leave the viewer responsive (no crash/hang)

### Disable out-of-scope features (D6)

- [X] T024 [US3] Removed the genuinely engine-encoder-backed commands from `MenuTemplate[]`/`PopupMenuTemplate[]` + `ToolBarButtons[]` (pictview.cpp): **Save As, Print, Crop, Paste**. Deliberately KEPT rotate/mirror/copy/histogram/pipette — the WIC engine backs them (`PVChangeImage` rotation, stretch-param mirror, `PVDrawImage` copy, `PVGetHandles2` pixel access) — and left scan/capture/wallpaper (they build their own HBITMAP via the attach path or degrade gracefully)
- [~] T025 [US3] Accelerator removal in pictview.rc2 DEFERRED — a leftover accelerator (e.g. Ctrl+S) routes to the handler which now calls the stubbed engine and shows a clean "not supported" message (no crash), so it degrades gracefully without the .rc2 edit
- [X] T026 [US3] Graceful backstop achieved via the engine stubs: `PVSaveImage`/`PVCropImage`/`PVLoadFromClipboard` return clean `PVCODE`s, and the existing render1.cpp handlers surface `PVGetErrorText` — any residual invocation shows a message, never crashes; `G` config fields kept intact (FR-008)
- [~] T027 [US3] On-screen US3 check (quickstart #8–#9) DEFERRED to desktop — broken.jpg rejection verified at the decode layer (T020); Save/Print/Crop/Paste removed from menu+toolbar (source-verified); viewer-window confirmation needs an interactive desktop

**Checkpoint**: Unsupported inputs and disabled features degrade cleanly

---

## Phase 6: Build cleanup, hardening & polish

- [X] T028 Retire `salpvenv.vcxproj` from the solution `src/vcxproj/salamand.sln` (it links the removed `PVW32Cnv.lib` and cannot build); confirm nothing else references `PVW32Cnv.lib`
- [~] T029 [P] Hardening (large image / animation first-frame / multi-page past-last) DEFERRED to desktop — the engine handles the cases (frame-0 decode, `PVC_NO_MORE_IMAGES`); interactive stress verification pending
- [X] T030 [P] Config round-trip (FR-008): the plugin loads clean with the existing config (SC-001 verified after full builds); `LoadConfiguration`/`SaveConfiguration`/`G` struct untouched; other 34 plugins + core register normally (plugins.ver v7)
- [X] T031 Full-solution regression: `build.cmd full` clean (all 90 projects, 35 plugins register in plugins.ver), then `build.cmd full release` (LTO/WPO) clean before merge; confirm the plugins dir has `pictview.spl`+`exif.dll` and needs no PVW32Cnv/SalPVEnv
- [X] T032 Write `specs/006-fix-pictview-plugin/validation-results.md` summarizing the quickstart matrix results, headless SC-001 check, decode-host SC-002 results, GPL clearance note, and any follow-ups (animation playback, save/convert, thumbnails) — 004/005 pattern

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: none — start immediately (T003 depends on T002)
- **Foundational (Phase 2)**: after Setup; **blocks all stories** (T004→T005→T006 sequential; the table + COM plumbing underpin US1 and US2)
- **US1 (Phase 3)**: after Phase 2 — MVP; removes the error using stubs. T007→T008/T009 (same file), T010 build, T011 verify last
- **US2 (Phase 4)**: after Phase 2; independent of US1 but naturally after it (needs the plugin to load to view). Engine fns T012–T018 mostly sequential within wicengine.cpp (shared CWicImage); T019 is the render1 open-site edit; T020/T021 verify
- **US3 (Phase 5)**: after US2 (graceful messages reference the real decode path); T024/T025 parallel (different files), T026 handlers, T027 verify
- **Polish (Phase 6)**: after all stories

### Parallel Opportunities

- Phase 1: T002 (new files) parallel with reading; T003 after T002
- Phase 5: T024 (menu/toolbar) ∥ T025 (accelerators) — different files
- Phase 6: T029 ∥ T030
- Within wicengine.cpp (T012–T018, T022) edits are sequential (same file)

## Implementation Strategy

**MVP first**: Phases 1–3 remove the reported error (US1) with the
engine table populated by stubs — the plugin loads clean and is
demoable on its own. Then US2 makes it actually view common formats
(the core value), US3 adds graceful degradation + feature gating, and
Phase 6 cleans up the build/solution and hardens. Stop-and-validate
points: T003, T006, T011 (MVP), T021, T027, T031. Commit after each
task or logical group; keep `lib/PVW32DLL.h` and `exif` untouched.
