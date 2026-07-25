# Feature Specification: Manual Brand Asset Replacement

**Feature Branch**: `035-manual-brand-assets`
**Created**: 2026-07-25
**Status**: Draft
**Input**: User description: "Uprav zdrojova data pro kompilaci projektu tak, abych mohl jednoduse rucne vymenit ikonu aplikace. Abych mohl vymenit - ikonu aplikace, ktera se zobrazuje male v okne vlevo nahore, ikonu exe souboru a pak obrazky, ktere se zobrazuji v pop upech about app a v splash screenu. Proste abych nemusel kazdou zmenu grafiky resit s AI. Dale drobnost, v splash screenu se zobrazuje copyright text, je moc dlouhy, zobrazuje se pouze v jedne radce a nevejde se na ni. Copyright ma dve casti - puvodni Open Salamader Authors a New Commander Authors, dej to pod sebe na dve radky."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Replace the application icon by swapping image files (Priority: P1)

The project maintainer has new icon artwork (e.g. exported from a design tool as standard raster images). They drop the new images into one designated, documented location in the repository, run one documented regeneration step, rebuild the application, and the new icon appears everywhere the application icon is shown: the small icon in the top-left corner of the main window, the taskbar, and the `.exe` file icon in Windows Explorer. No source-code, resource-script, or project-file edits are needed, and no AI assistance is required.

**Why this priority**: This is the core request — the maintainer wants graphics changes to be a self-service file swap instead of an engineering task. The application icon is the most visible and most frequently iterated brand asset.

**Independent Test**: Replace the designated icon source images with visibly different artwork, follow only the documented steps, rebuild, and confirm the new artwork appears in the window title bar, taskbar, and Explorer — without touching any code.

**Acceptance Scenarios**:

1. **Given** a set of replacement icon images in the required sizes, **When** the maintainer places them in the designated location and runs the single documented regeneration step, **Then** all shipped icon files are regenerated from the new artwork with no manual edits to any other file.
2. **Given** the regenerated icons, **When** the application is rebuilt and started, **Then** the new icon is shown in the main window's top-left corner, in the taskbar, and as the `.exe` file icon in Explorer.
3. **Given** the icon swap, **When** the crash reporter, installer, or uninstaller is displayed, **Then** their icons also reflect the new artwork (they are part of the same regenerated set).

---

### User Story 2 - Replace the About and splash-screen artwork by swapping one image (Priority: P2)

The maintainer wants to change the picture drawn in the About dialog and on the startup splash screen. They replace one designated artwork file with their new image, rebuild, and both places show the new picture. They do not need to know anything about the application's internal rendering constraints (which today force hand-authoring a restricted vector format).

**Why this priority**: Second half of the "no AI needed" goal. Today this asset is the hardest to change by hand because the file must obey undocumented renderer limitations; a plain image swap removes that barrier.

**Independent Test**: Replace the designated artwork file with a visibly different image, rebuild, open the About dialog and restart the application, and confirm both show the new picture undistorted.

**Acceptance Scenarios**:

1. **Given** a replacement artwork image in a documented common format, **When** the maintainer swaps the designated file and rebuilds, **Then** the About dialog shows the new artwork.
2. **Given** the same swap, **When** the application starts, **Then** the splash screen shows the new artwork.
3. **Given** replacement artwork with a different aspect ratio than the original, **When** it is displayed in the About dialog and splash screen, **Then** it is scaled to fit its reserved area without distortion.

---

### User Story 3 - Splash-screen copyright on two lines (Priority: P3)

When the application starts, the splash screen shows the copyright notice. Today the full text ("Copyright © 1997-2026 Open Salamander Authors, © 2026 Newt Commander Authors") is drawn on a single line and does not fit the splash width, so it is cut off. The maintainer wants the two parts shown beneath each other: the Open Salamander Authors part on the first line and the Newt Commander Authors part on the second line, both fully visible.

**Why this priority**: A small, self-contained display fix; valuable but independent of the asset-replacement work.

**Independent Test**: Start the application and observe the splash screen: the copyright occupies two lines, neither line is truncated, and other splash elements (version, status text) remain readable and correctly placed.

**Acceptance Scenarios**:

1. **Given** the application starts, **When** the splash screen is shown, **Then** the copyright is displayed on two lines — "Copyright © 1997-2026 Open Salamander Authors" above and "© 2026 Newt Commander Authors" below — with no truncation of either line.
2. **Given** the two-line copyright, **When** the rest of the splash content is drawn (version text, loading status), **Then** no element overlaps another and the layout remains visually balanced.
3. **Given** the change, **When** the `.exe` file properties or other places showing the version copyright are inspected, **Then** the stored copyright string itself is unchanged (only the splash presentation is split).

---

### User Story 4 - Self-service asset guide (Priority: P2)

The maintainer (or any future contributor) opens a single document in the repository and finds everything needed to change the application's graphics: which files are replaceable, where each one appears in the product, what format and sizes are required, and the exact step-by-step replacement procedure. Following that document alone is sufficient — no AI, no code archaeology.

**Why this priority**: The documentation is what actually delivers "I don't need AI for graphics changes"; without it the mechanics of US1/US2 remain expert knowledge.

**Independent Test**: Give the document to a person unfamiliar with the codebase and have them perform an icon swap end-to-end using only the document.

**Acceptance Scenarios**:

1. **Given** the repository, **When** the maintainer opens the asset guide, **Then** it enumerates every replaceable brand asset with its file path, where it appears in the product, and the required format(s)/size(s).
2. **Given** the guide, **When** the maintainer follows only its steps to swap an asset, **Then** the swap succeeds without consulting any other source of information.

---

### Edge Cases

- What happens when a required icon size is missing from the replacement image set? The regeneration step must stop with a clear message naming the missing size — it must not silently produce an incomplete or broken icon set.
- What happens when a replacement image has the wrong format or dimensions? The regeneration step must reject it with a message identifying the offending file and the expected properties.
- What happens when the replacement About/splash artwork has an extreme aspect ratio (very wide or very tall)? It is scaled to fit its reserved area, preserving aspect ratio, without overlapping neighbouring texts.
- What happens to the alternative-color main-window icon variants (red/green/blue, selectable in configuration) after a swap? They must remain selectable and be derived from the new artwork; if the new artwork contains no colors eligible for automatic recoloring, the variants fall back to the base artwork rather than breaking.
- What happens if a future copyright string is longer? Each of the two splash lines must independently fit the splash width at the default splash size (the split point follows the two authorship parts).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: All shipped application-identity icons (main window/title-bar icon, `.exe` file icon, crash-reporter icon, installer icon, uninstaller icon) MUST be regenerable from one designated, replaceable set of source images via a single documented step, with no edits to source code, resource scripts, or project files.
- **FR-002**: After regeneration and rebuild, the replaced artwork MUST appear in the main window's top-left corner, in the taskbar, and as the `.exe` file icon in Windows Explorer.
- **FR-003**: The alternative-color main-window icon variants offered in the configuration dialog MUST remain functional after an icon swap, derived automatically from the new artwork; when automatic derivation is not applicable they MUST fall back to the base artwork.
- **FR-004**: The artwork displayed in the About dialog MUST be replaceable by swapping a single designated image file in a documented common image format; the maintainer MUST NOT need any knowledge of internal rendering constraints.
- **FR-005**: The artwork displayed on the splash screen MUST be replaceable the same way; About and splash share the one designated artwork file.
- **FR-006**: Replacement About/splash artwork MUST be displayed scaled to its reserved area with aspect ratio preserved (no distortion), regardless of the replacement image's proportions.
- **FR-007**: The regeneration step MUST validate its inputs (file presence, required sizes, supported formats) and fail with a message that names the problem and the offending file instead of producing broken or partial assets.
- **FR-008**: A single document in the repository MUST enumerate every replaceable brand asset — file path, where it appears in the product, required format(s) and size(s) — and give the exact replacement steps, sufficient on its own for a person unfamiliar with the codebase.
- **FR-009**: The splash screen MUST display the copyright notice on two lines: the "Copyright © 1997-2026 Open Salamander Authors" part on the first line and the "© 2026 Newt Commander Authors" part on the second, each fully visible at the default splash size.
- **FR-010**: The two-line splash presentation MUST NOT change the copyright string stored in the application's version information (file properties and any other consumers keep the full single string).
- **FR-011**: The splash layout MUST accommodate the second copyright line without overlapping or displacing the version text, status text, or artwork.

### Key Entities

- **Icon source set**: The replaceable raster images (one per required size) that are the single source for every shipped application-identity icon.
- **About/splash artwork**: The single replaceable image drawn in the About dialog and on the splash screen.
- **Derived icon variants**: The alternative-color main-window icons produced automatically from the icon source set.
- **Asset guide**: The one repository document describing every replaceable asset and the replacement procedure.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A person unfamiliar with the codebase can replace the application icon everywhere it appears by following only the asset guide, in under 15 minutes, touching image files only (zero source-code edits).
- **SC-002**: A person unfamiliar with the codebase can replace the About/splash artwork by swapping exactly one image file and rebuilding, in under 10 minutes, with zero knowledge of rendering internals.
- **SC-003**: After an icon swap and rebuild, the new artwork is visible in 100% of the application-identity locations (window top-left corner, taskbar, `.exe` in Explorer, crash reporter, installer, uninstaller).
- **SC-004**: The splash-screen copyright is fully readable — both lines visible with no truncation — at the default splash size on a standard display.
- **SC-005**: An invalid replacement input (missing size, wrong format) is reported with an actionable message in 100% of cases; no build ever ships a broken icon set.
- **SC-006**: All graphics-change procedures are completed using repository documentation alone — zero AI-assistance steps required.

## Assumptions

- The person performing a swap is a project maintainer able to run a rebuild and one documented command; they can export raster images at the required sizes from their design tool.
- Replacement About/splash artwork may be supplied as a common raster image; the current need to hand-author artwork within undocumented vector-renderer limitations is removed by this feature.
- The About dialog and the splash screen intentionally share one artwork file (they show the same logo today); per-surface artwork is out of scope.
- The alternative-color icon variants continue to be derived automatically (as today); supplying hand-made per-color variants is out of scope.
- The copyright split point is the boundary between the two authorship parts ("…Open Salamander Authors," / "© 2026 Newt Commander Authors"); the stored version-info string itself is not modified.
- Out of scope: redesigning any artwork, toolbar/panel icons, plugin icons, file-type icons, and installer visuals beyond the already-shipped installer/uninstaller icons.
- The GDI-drawn "Newt Commander" wordmark and the accent gradient line are not part of this feature's replaceable-asset set; they remain as-is unless a future feature addresses them.
