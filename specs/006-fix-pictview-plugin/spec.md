# Feature Specification: Repair the PictView Plugin So It Loads and Views Images

**Feature Branch**: `006-fix-pictview-plugin`
**Created**: 2026-07-15
**Status**: Draft
**Input**: User description: "cílem úpravy je hloubková analýza a oprava pluginu PictView, který při spuštění hlásil chybu že se jej nepovedlo načíst. Cílem je jeho oprava tak, aby fungoval"

## Clarifications

### Session 2026-07-15

- Q: What scope should "make it work" cover, given the proprietary
  engine is permanently unavailable (GPL)? → A: **Full viewer** — the
  plugin loads without error AND actually displays the common raster
  formats (JPEG/PNG/BMP/GIF/TIFF) via a built-in Windows imaging
  capability, with zoom/scroll/next-previous and graceful degradation
  for anything unsupported (User Stories 1–3). Advanced engine-heavy
  capabilities (save/convert to other formats, image editing, TWAIN
  scanning, screen capture, printing) are **out of scope** for this
  feature and are cleanly disabled/hidden; they may be a follow-up.

## Problem Statement

Every time Open Salamander starts, it shows an error box:

> This plugin (…\plugins\pictview\pictview.spl) has not been loaded.
> Plugin is not valid Open Salamander Plugin or some plugin internal
> error has occurred.

As a result the built-in **PictView image viewer is completely
unavailable** — users cannot view images with it, and they are
interrupted by an error dialog on every launch.

Root cause (verified in source): PictView's viewing pipeline was built
on a **proprietary native engine, `PVW32Cnv.dll`** (on 64-bit Windows
hosted out-of-process by a 32-bit helper, `SalPVEnv.exe`). That engine
and its build inputs were **deliberately removed from the repository
because their source is unavailable and shipping them violates the
GPLv2 license** (`src/plugins/pictview/lib/readme.txt`). The plugin
treats the engine as **mandatory**: its startup routine tries to load
the engine, fails, and makes the whole plugin refuse to register — so
Salamander reports it as "not loaded." The repository's own note
records the intended direction: *"Replace the PictView engine with WIC
or another library."*

This feature makes PictView **work again without any proprietary
component**: it must load cleanly on every start (no error box) and
actually display common image formats using a royalty-free imaging
capability already available on Windows, degrading gracefully for
anything it cannot handle.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Salamander starts without the PictView error (Priority: P1)

A user launches Open Salamander. No "plugin not loaded" error appears,
and PictView is listed as an active, enabled plugin in the Plugin
Manager.

**Why this priority**: This is the literal reported defect and it
interrupts every single launch. Removing the error and getting the
plugin to register is the minimum viable outcome and a prerequisite for
everything else.

**Independent Test**: Start Salamander with no proprietary engine
present; confirm no error dialog and that PictView shows as loaded in
the Plugin Manager (Plugins menu).

**Acceptance Scenarios**:

1. **Given** a build with no `PVW32Cnv.dll` / `SalPVEnv.exe` present,
   **When** Salamander starts, **Then** no plugin-load error dialog is
   shown.
2. **Given** Salamander has started, **When** the user opens the Plugin
   Manager, **Then** PictView appears as loaded/enabled and is
   associated as an image viewer.

---

### User Story 2 - View common image formats (Priority: P1)

A user focuses an image file (JPEG, PNG, BMP, GIF, TIFF) in a panel and
opens it in the PictView viewer. The image is displayed correctly, and
the standard viewer interactions work: zoom (fit / 1:1 / in / out),
scroll/pan, and moving to the next/previous image in the folder.

**Why this priority**: "Make it work" means the viewer actually shows
images. A plugin that loads but can display nothing is not a working
viewer. Common web/photo raster formats cover the overwhelming majority
of real use.

**Independent Test**: Open one file of each listed format in the
viewer; verify the picture renders, matches the file (compare with
Windows Photos side by side), and that zoom/scroll/next-previous
behave.

**Acceptance Scenarios**:

1. **Given** a valid JPEG/PNG/BMP/GIF/TIFF file, **When** the user
   opens it in the PictView viewer, **Then** the image renders
   correctly at correct dimensions and colors.
2. **Given** an image open in the viewer, **When** the user zooms and
   scrolls, **Then** the view updates correctly with no crash.
3. **Given** a folder of images, **When** the user pages to the
   next/previous image, **Then** the adjacent image loads.
4. **Given** an animated GIF, **When** it is opened, **Then** at least
   its first frame renders (animation playback is a nice-to-have, not
   required for P1).

---

### User Story 3 - Graceful handling of unsupported files and features (Priority: P2)

A user opens a file the built-in engine cannot decode (an exotic or
corrupt format), or invokes an advanced action that depended on the
removed engine. Instead of crashing, silently failing, or reprinting
the load error, PictView shows a clear, localized message that the file
or feature is not supported by the current image engine, and the rest
of the application keeps working.

**Why this priority**: With the proprietary engine gone, some formats
and advanced operations (e.g., certain conversions/edits) cannot be
supported. Predictable, honest degradation preserves trust and
stability; it is important but secondary to loading and basic viewing.

**Independent Test**: Open a deliberately unsupported/corrupt image and
an advanced action; verify a clear message and no crash, and that the
viewer/app remain usable afterward.

**Acceptance Scenarios**:

1. **Given** a file the engine cannot decode, **When** the user opens
   it, **Then** a clear "cannot display this image" message is shown and
   the viewer stays open and responsive.
2. **Given** an advanced action with no engine backing, **When** the
   user invokes it, **Then** it is either hidden/disabled or reports a
   clear "not available" message — never a crash or the plugin-load
   error.

---

### Edge Cases

- No proprietary engine anywhere on the system (the normal case): the
  plugin must still load and view common formats.
- A legitimately corrupt or truncated image file: report an error, do
  not crash.
- Very large images (e.g., > 100 MP): either display (possibly scaled)
  or report a clear limit; never hang indefinitely or crash the app.
- Unicode and long-path image file names (per features 004/005): the
  viewer must open them and show the name correctly in its title/status.
- Existing user configuration/registry entries from the old PictView:
  loading them must not fail or crash.
- Files whose extension is registered to PictView but whose content is
  not actually an image: clear error, no crash.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The PictView plugin MUST load and register successfully
  on every application start when no proprietary image engine is
  present, and MUST NOT display any plugin-load error.
- **FR-002**: The plugin MUST NOT require, link, load, or ship any
  proprietary or non-GPLv2-compatible component; it MUST rely only on
  capabilities already available on the supported Windows platform or
  on GPLv2-compatible code.
- **FR-003**: The viewer MUST display images in the common raster
  formats JPEG, PNG, BMP, GIF, and TIFF, rendered correctly in size and
  color.
- **FR-004**: The viewer MUST support core viewing interactions for a
  displayed image: zoom (fit-to-window, actual size, in, out),
  scroll/pan, and navigation to the next/previous image in the folder.
- **FR-005**: When a file cannot be decoded or an action is not
  supported by the current engine, the plugin MUST show a clear,
  localized message and remain stable (no crash, no hang, no repeat of
  the load error).
- **FR-006**: Features that cannot be provided without the removed
  engine MUST be cleanly disabled or hidden rather than presented as
  broken controls.
- **FR-007**: The viewer MUST open images with Unicode and long-path
  file names and display those names correctly (consistent with
  features 004 and 005).
- **FR-008**: Existing behavior for other plugins and for the core
  application MUST NOT regress; the whole solution MUST continue to
  build and run.

### Key Entities

- **PictView plugin (`pictview.spl`)**: the viewer plugin that must
  register with the application and provide image viewing.
- **Image engine (capability, not a specific product)**: the component
  that decodes image bytes into a displayable bitmap and reports image
  metadata. Previously the proprietary `PVW32Cnv.dll`; this feature
  replaces that dependency with a royalty-free capability.
- **Supported format set**: the list of image formats the viewer can
  display; its guaranteed members are JPEG, PNG, BMP, GIF, TIFF.
- **Unsupported input/action**: any file the engine cannot decode or
  any operation with no engine backing; handled by graceful
  degradation.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In 100% of application starts (with no proprietary engine
  present), no PictView load error appears and PictView is listed as
  loaded in the Plugin Manager.
- **SC-002**: 100% of a test set of valid JPEG, PNG, BMP, GIF, and TIFF
  images open in the viewer and render correctly (verified against a
  reference viewer).
- **SC-003**: For every image displayed, zoom, scroll, and
  next/previous navigation work without error in 100% of trials.
- **SC-004**: 100% of unsupported/corrupt test inputs produce a clear
  message and leave the viewer and application responsive (zero
  crashes/hangs).
- **SC-005**: Unicode-named and long-path images open and show their
  correct name in 100% of trials.
- **SC-006**: The full solution builds clean and no other plugin or
  core feature regresses (existing verification passes).

## Assumptions

- **"Works" means a functioning viewer, not a stub.** Because the
  proprietary engine is permanently unavailable (GPL), the fix replaces
  the decode/display path with a royalty-free imaging capability that
  ships with the supported Windows platform, targeting the common
  raster formats above. This matches the repository's own recorded TODO
  ("Replace the PictView engine with WIC or another library").
- **Scope depth (confirmed with the requester, 2026-07-15)**: the
  deliverable is *loading + viewing + basic navigation + graceful
  degradation* for common formats (User Stories 1–3). Advanced,
  engine-heavy capabilities of the original PictView — save/convert to
  other formats, image editing (crop/rotate/color), multi-page/large
  format decoding beyond the common set, TWAIN scanning, screen
  capture, printing pipeline — are **out of scope for this feature**
  and are disabled gracefully (FR-006) or deferred to a follow-up.
- The out-of-process 32-bit host (`SalPVEnv.exe`) is retired for the
  common-format viewing path, since it existed only to host the 32-bit
  proprietary engine; it may remain as dead/optional build output but
  is not required for the plugin to work.
- Target platform is the current development baseline (64-bit Windows
  11, branch `ai-main` after features 004/005), consistent with the
  project constitution.
- The internal viewer content-encoding limitations noted in feature 004
  (viewer is out of scope there) do not block this feature: image
  *display* is precisely what PictView provides and is in scope here.
