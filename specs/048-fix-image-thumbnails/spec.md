# Feature Specification: Restore Image Thumbnails in Thumbnail View

**Feature Branch**: `048-fix-image-thumbnails`
**Created**: 2026-08-02
**Status**: Draft
**Input**: User description: "Při zobrazení panelu v režimu miniatury (ALT+5) se u obrázků zobrazuje pouze ikona souboru. Toto chování je potřeba opravit tak, aby se zobrazoval přímo obrázek, tak jako v původním projektu. Detailně analyzuj zdrojový kód, jak se zobrazovaly náhledy obrázků dříve, navrhni úpravu a implementuj tak, aby se opět při zobrazení minuatur zobrazovaly náhledy obrázků."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Image previews appear in thumbnail view (Priority: P1)

A user browsing a folder that contains pictures switches the panel to
thumbnail view (Alt+5). Instead of the current behavior — every picture
showing only its generic file-type icon — each picture displays a
scaled-down preview of its actual content, exactly as the original
Open Salamander did.

**Why this priority**: This is the defect itself. Thumbnail view exists
solely to show image content; without previews the mode has no value.

**Independent Test**: Open a folder containing common image files
(JPEG, PNG, GIF, BMP, TIFF), press Alt+5, and confirm each image shows
its picture content rather than a file icon.

**Acceptance Scenarios**:

1. **Given** a panel showing a folder with common-format image files,
   **When** the user switches to thumbnail view (Alt+5),
   **Then** each image file displays a preview of its actual content.
2. **Given** the panel is already in thumbnail view,
   **When** the user navigates into another folder containing images,
   **Then** previews appear for the images in that folder as well.
3. **Given** a fresh installation with default settings (no prior
   configuration in the registry),
   **When** the user switches to thumbnail view in a folder with images,
   **Then** previews appear without any manual configuration step.

---

### User Story 2 - Graceful fallback for non-previewable files (Priority: P2)

While in thumbnail view, files that are not images — or images that
cannot be decoded (corrupt, truncated, unsupported format) — continue to
show their standard file icon, with no error messages interrupting
browsing.

**Why this priority**: The fix must not trade one defect for another;
mixed folders are the norm and failures must stay silent, as upstream
behaved.

**Independent Test**: Open a folder mixing images, documents, and a
deliberately corrupted image file in thumbnail view; verify documents and
the corrupt file show icons and no error dialog appears.

**Acceptance Scenarios**:

1. **Given** a folder with images and non-image files in thumbnail view,
   **When** thumbnails load,
   **Then** non-image files show their standard icons.
2. **Given** a corrupt or truncated image file in the folder,
   **When** thumbnail loading reaches it,
   **Then** that file falls back to its standard icon and no error dialog
   is shown.

---

### User Story 3 - Panel stays responsive while thumbnails load (Priority: P3)

In a folder with many or very large images, thumbnails fill in
progressively in the background while the user continues to browse,
scroll, select, and open files without delay — matching the original
project's behavior (icons appear first and are replaced by previews as
they are produced).

**Why this priority**: Restoring previews must not regress the panel's
responsiveness, which is a core quality of the product.

**Independent Test**: Open a folder with 100+ high-resolution photos in
thumbnail view and verify scrolling and keyboard navigation remain
immediate while previews continue to appear.

**Acceptance Scenarios**:

1. **Given** a folder with a large number of high-resolution images,
   **When** the user switches to thumbnail view,
   **Then** the panel is immediately usable and previews appear
   progressively.
2. **Given** thumbnails are still loading,
   **When** the user leaves the folder or switches view mode,
   **Then** the application continues to respond normally and no stale
   previews appear in the new location.

---

### Edge Cases

- Image file whose extension does not match its content (e.g., a text
  file renamed to `.jpg`) — must fall back to the icon silently.
- Extremely large image dimensions or file sizes — must not exhaust
  memory or freeze the panel; the file may fall back to an icon if it
  cannot be previewed within reasonable limits.
- Files on slow media (network drives, removable media) — previews load
  progressively; browsing is not blocked.
- Both panels in thumbnail view on the same folder at once — previews
  appear in both without conflicts.
- Thumbnail size setting changed in Configuration — previews honor the
  configured size after the change.
- Zero-byte files with image extensions — icon fallback, no error.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: In thumbnail view, the system MUST display a scaled preview
  of the file's image content for every image format the application's
  built-in picture viewer can display.
- **FR-002**: Image previews MUST work out of the box on a fresh
  installation with default settings — no manual plugin configuration,
  registry edit, or one-time setup step may be required.
- **FR-003**: Files whose preview cannot be produced (non-image content,
  corrupt data, unsupported format, read error) MUST silently fall back
  to the standard file icon; no error dialogs may interrupt browsing.
- **FR-004**: Thumbnail production MUST run in the background: the panel
  MUST remain fully interactive (scrolling, selection, navigation,
  opening files) while previews are being produced.
- **FR-005**: Preview behavior MUST match the original Open Salamander
  project: previews respect the configured thumbnail size, preserve the
  image's aspect ratio, and coexist with existing panel visuals
  (selection, focus, overlay symbols).
- **FR-006**: The existing thumbnail-related options in the Configuration
  dialog MUST remain functional and MUST affect preview production as
  they did in the original project.
- **FR-007**: The restored behavior MUST NOT depend on configuration data
  from any previously installed product (the application never reads
  Open Salamander, Altap, or Newt Commander registry keys).

### Key Entities

- **Thumbnail (preview)**: A scaled-down rendering of an image file's
  content shown in place of its file icon in thumbnail view.
- **Image file**: A file in a format the application's picture viewer
  supports; the set of previewable formats follows the viewer's
  capabilities.
- **Thumbnail provider**: The application component responsible for
  producing previews from image files; its availability on a default
  installation is a precondition for FR-002.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In a folder of at least 20 images across the common formats
  (JPEG, PNG, GIF, BMP, TIFF), 100% of the images display content
  previews in thumbnail view on a default installation.
- **SC-002**: A user can scroll and navigate a folder of 100+
  high-resolution photos immediately after switching to thumbnail view,
  with no perceptible freeze, while previews continue to fill in.
- **SC-003**: Browsing a mixed folder (images, documents, one corrupt
  image) in thumbnail view produces zero error dialogs; every
  non-previewable file shows its standard icon.
- **SC-004**: Side-by-side with the original Open Salamander on the same
  test folder, thumbnail view shows previews for the same set of files
  with equivalent visual results (size, aspect ratio, overlays).

## Assumptions

- The defect is a regression introduced in this fork; the original
  Open Salamander displays previews correctly with the same folder
  contents, so "match the original project" is a well-defined target.
- The picture viewer plugin (pictview) is the thumbnail provider and is
  enabled in the default build (`plugins.cfg`: `pictview=on`); restoring
  its provider role — however the chain is broken — is in scope.
- The scope is restoring previously working behavior, not extending it:
  no new image formats, no new configuration options, no visual redesign
  of thumbnail view.
- The set of previewable formats equals whatever the application's
  picture viewer supports today (WIC-based since feature 006); formats
  the viewer cannot open are out of scope for previews.
- Root-cause analysis is complete (see
  [research-thumbnail-chain.md](research-thumbnail-chain.md)): the
  provider registration and capability propagation are intact; the break
  is confined to preview *production* inside the picture viewer plugin —
  a known, documented follow-up left open by the feature 006 engine
  rewrite. The planning phase starts from that confirmed diagnosis.
