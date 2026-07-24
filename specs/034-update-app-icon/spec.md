# Feature Specification: Update Application Icon to Revised Artwork

**Feature Branch**: `034-update-app-icon`
**Created**: 2026-07-24
**Status**: Draft
**Input**: User description: "V minule uprave jsme pouze vymenovali ikonu, potrebuji to cele zopakovat, ikonu jsem jeste upravit. Je opet v adresari ./temp/icon, je to ./temp/icon/newt-commander-icon.svg a PNG jsou v ./temp/icon/png. Vymen v aplikaci tuto ikonu."

## Overview

Feature 033 replaced the Newt Commander application icon with the "folder on
a dark navy rounded tile" artwork on every surface where the application icon
is shown. The designer has since **revised the artwork**: the new master
(delivered again as an SVG plus pre-rendered PNGs at 16–1024 px in
`temp/icon/`) **drops the dark tile background entirely** — the icon is now
the orange folder with peeking documents alone, enlarged to fill the canvas,
on a fully transparent background.

This feature repeats the complete 033 icon swap with the revised artwork:
every surface that today shows the tile-based icon must show the new
tile-less icon, and the repository's brand-asset source of truth must be
updated so future regenerations reproduce the revised design.

Surfaces where the application icon is used today (all adopted the 033
artwork and must all be updated):

1. **Executable / main window icon** — Windows Explorer, taskbar, window
   caption, Alt+Tab switcher, shortcuts, pinned items.
2. **Colored main-window icon variants** — the red / green / blue icon
   choices offered in Configuration → Main Window (used to visually
   distinguish running instances).
3. **Splash screen** — the icon drawn above the brand accent strip.
4. **About dialog** — the same icon next to the wordmark.
5. **Companion programs** — the crash reporter and the installer /
   uninstaller, which received the 033 icon and must follow the revision.

The wordmark, tagline, gradient accent strip, and all other feature 032
brand elements stay unchanged — only the icon artwork is replaced.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Revised icon in the operating system shell (Priority: P1)

A user who installs or launches Newt Commander sees the revised tile-less
icon everywhere Windows presents the application: the program file in
Explorer, the taskbar button, the window caption, the Alt+Tab switcher,
desktop shortcuts, and the Start menu.

**Why this priority**: The OS shell is where the application icon is seen
most often and by every user; it is the primary identity of the product.
Until it changes, the revision effectively does not exist.

**Independent Test**: Build the application, locate the executable in
Explorer and run it. Verify the revised icon appears in Explorer (small,
medium, large, extra-large views), on the taskbar, in the window caption,
and in Alt+Tab — with no rendering artifacts (jagged edges, wrong
transparency, leftover tile background, black background) at any size.

**Acceptance Scenarios**:

1. **Given** a freshly built application, **When** the user views the
   executable in Explorer at any icon view size, **Then** the revised icon
   is shown crisply, with everything outside the folder silhouette fully
   transparent, at every size.
2. **Given** the application is running, **When** the user looks at the
   taskbar, the window caption, and the Alt+Tab switcher, **Then** all three
   show the revised icon.
3. **Given** the default icon setting, **When** the user opens the main
   window, **Then** the window icon matches the executable icon.

---

### User Story 2 - Revised icon on in-app branding surfaces (Priority: P2)

A user opening the About dialog or watching the startup splash screen sees
the revised icon rendered as the brand mark, visually consistent with the
prepared master artwork and harmonious with the existing wordmark, tagline,
and gradient accent.

**Why this priority**: About and splash are the product's deliberate
brand-presentation moments; the superseded tile icon there would directly
contradict the revised identity. Second only to the OS shell because these
surfaces are seen less frequently.

**Independent Test**: Launch the application with the splash screen enabled
and open Help → About. Compare the displayed icon against the prepared
master artwork in both light and dark application themes.

**Acceptance Scenarios**:

1. **Given** the splash screen is enabled, **When** the application starts,
   **Then** the splash shows the revised icon, visually faithful to the
   master artwork (shapes, colors, proportions).
2. **Given** the About dialog is opened, **When** the user views it in light
   theme and in dark theme, **Then** the revised icon is displayed correctly
   in both, with no visual artifacts and no remnant of the old tile
   background.
3. **Given** the About dialog and the executable icon side by side, **When**
   compared, **Then** they clearly read as the same artwork.

---

### User Story 3 - Colored window-icon variants keep working (Priority: P3)

A user who runs several instances of Newt Commander and distinguishes them
via Configuration → Main Window icon color (default / red / green / blue)
keeps that capability: all four choices remain selectable and now render as
color variants of the revised icon design.

**Why this priority**: An existing, user-visible configuration feature must
not silently break or regress to superseded artwork; it is P3 because only a
subset of users changes the window icon color.

**Independent Test**: Open Configuration → Main Window, cycle through all
four icon choices, and confirm each applies a clearly distinguishable
variant of the revised design to the window and taskbar.

**Acceptance Scenarios**:

1. **Given** the configuration page for the main window, **When** the user
   selects each of the four icon options, **Then** each option previews and
   applies a variant of the revised icon design.
2. **Given** the red, green, and blue variants, **When** shown at taskbar
   size, **Then** they are clearly distinguishable from each other and from
   the default at a glance.

---

### User Story 4 - Companion programs adopt the revised icon (Priority: P4)

A user who encounters the crash reporter or runs the installer /
uninstaller sees the revised Newt Commander icon rather than the superseded
tile-based artwork these programs received in feature 033.

**Why this priority**: These surfaces appear rarely (crash, install,
uninstall), but leaving them on the previous artwork would make the product
show two different icons and the swap incomplete.

**Independent Test**: Inspect the crash reporter and installer /
uninstaller executables in Explorer and, where feasible, run them; verify
each displays the revised icon.

**Acceptance Scenarios**:

1. **Given** the built crash reporter, **When** its executable or window is
   viewed, **Then** it shows the revised icon.
2. **Given** the built installer and uninstaller, **When** their executables
   or windows are viewed, **Then** they show the revised icon.

---

### Edge Cases

- **Transparent background**: unlike the 033 tile, the icon's silhouette is
  now the folder shape itself. Everything outside it must be fully
  transparent so the icon looks correct on light Explorer backgrounds, dark
  taskbars, and both application themes — no square tile remnant, halo, or
  matte edge may remain at any size.
- **In-app surfaces without the tile**: on About and splash the icon
  previously carried its own dark tile background; the revised icon sits
  directly on the dialog / splash background. It must remain legible and
  well-composed against both light and dark theme backgrounds.
- **Small sizes (16–32 px)**: the artwork is detailed (folder, documents,
  text rows); at the smallest sizes it must stay recognizable — the prepared
  PNG set includes dedicated 16/24/32 px renders that are used as delivered.
  If a delivered small size proves illegible in practice, that is raised as
  feedback on the asset, not silently redrawn.
- **In-app rendering fidelity**: the About/splash surfaces render the icon
  through the application's built-in vector renderer, which does not support
  every effect used in the master SVG (e.g., soft drop shadow). The displayed
  mark must remain visually faithful — a simplified variant without
  unsupported effects is acceptable as long as shapes, colors, and
  proportions match the master.
- **Windows icon cache**: after an in-place upgrade, Explorer may keep
  showing the previous icon from its icon cache. Verification is done on a
  fresh build location (or after a cache refresh); stale caches on end-user
  machines are outside the application's control and not a defect.
- **Regeneration reproducibility**: the brand-asset pipeline currently
  reproduces the *033* design; if it were left unchanged, a future
  regeneration would silently overwrite the revised icon with the superseded
  artwork. The pipeline inputs and its documentation must be brought in line
  with the revised icon so regeneration reproduces what ships.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The application executable MUST carry the revised icon in all
  standard Windows icon sizes currently shipped (16, 24, 32, 48, 64, 128,
  256 px), with correct alpha transparency at every size.
- **FR-002**: The main window, taskbar button, and Alt+Tab entry MUST show
  the revised icon when the default icon setting is active.
- **FR-003**: The splash screen MUST display the revised icon as its brand
  mark, visually faithful to the master artwork.
- **FR-004**: The About dialog MUST display the revised icon as its brand
  mark in both light and dark application themes, visually faithful to the
  master artwork.
- **FR-005**: The red, green, and blue main-window icon options MUST remain
  functional and MUST render as clearly distinguishable color variants of
  the revised icon design (no superseded artwork remains selectable).
- **FR-006**: The crash reporter MUST carry the revised icon instead of the
  033 tile-based icon.
- **FR-007**: The installer and uninstaller MUST carry the revised icon
  instead of the 033 tile-based icon.
- **FR-008**: The revised icon source artwork (master SVG and the
  pre-rendered PNG set) MUST be adopted into the repository's brand-asset
  source of truth, replacing the 033 master artwork there (the delivery
  location under `temp/` is not version-controlled).
- **FR-009**: The brand-asset regeneration procedure MUST reproduce the
  shipped icon files from the adopted source artwork, so that regenerating
  assets can never resurrect the superseded design; its documentation MUST
  be updated accordingly.
- **FR-010**: All other brand elements from features 032/033 (wordmark,
  tagline, gradient accent strip, product name, version) MUST remain
  unchanged.
- **FR-011**: No user- or OS-visible surface of the product may continue to
  display the 033 "folder on navy tile" icon (or any older icon) after this
  change.

### Key Entities

- **Master icon artwork**: the revised vector original (single source of
  design truth — orange folder with documents, transparent background) plus
  its pre-rendered PNG set at 16, 24, 32, 48, 64, 128, 256, 512, and
  1024 px, delivered in `temp/icon/`.
- **Shipped icon assets**: the packaged multi-size icon resources embedded
  in the application, crash reporter, installer, and uninstaller, and the
  in-app logo asset used by About/splash — all derived from the master
  artwork.
- **Icon variants**: the default plus red / green / blue main-window
  variants offered in configuration, derived from the same design.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On a fresh build, 100% of the enumerated OS-shell surfaces
  (Explorer file icon at four view sizes, taskbar, window caption, Alt+Tab)
  display the revised icon with no rendering artifacts and no tile remnant.
- **SC-002**: The About dialog and splash screen marks pass a side-by-side
  visual comparison against the master artwork (same shapes, colors,
  proportions) in both light and dark themes.
- **SC-003**: An audit of all product surfaces that display an application
  icon finds zero occurrences of the 033 tile-based icon or any older icon.
- **SC-004**: All four main-window icon choices in configuration are
  selectable and produce four variants that a user can tell apart at
  taskbar size on the first attempt.
- **SC-005**: Running the documented asset-regeneration procedure on a clean
  checkout reproduces the shipped icon assets (no differences that are
  visible to the user).
- **SC-006**: At 16 px the icon remains recognizable as the Newt Commander
  orange folder when viewed alongside other application icons, on both light
  and dark backgrounds.

## Assumptions

- The prepared artwork in `temp/icon/` is final and approved; no design
  changes to it are part of this feature. Its PNG renders (16–1024 px) are
  authoritative for raster output, including the small-size renders.
- The scope is identical to feature 033 ("cele zopakovat" — repeat the whole
  thing): all surfaces covered there are covered here, including the
  companion programs (crash reporter, installer/uninstaller) and the
  colored window-icon variants.
- The red/green/blue window-icon feature is kept (not removed); the state
  colors are applied to the revised design the same way as in 033 (exact
  recoloring approach is a planning-level detail, with the only requirement
  being at-a-glance distinguishability).
- The wordmark, tagline, gradient accent strip, and lockup artwork from
  feature 032 are out of scope and remain unchanged; only icon artwork is
  replaced.
- Per-file-type icons inside the application (folders, archives, plugins,
  viewers, etc.) are separate artwork, not the application icon, and are
  out of scope.
- Documentation screenshots and website imagery are out of scope for this
  feature.
- Sizes above 256 px (512, 1024) are available in the master set but are not
  currently shipped in the packaged icon; shipping them is not required.
