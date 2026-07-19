# Feature Specification: mdview Viewer UX Fixes

**Feature Branch**: `022-mdview-viewer-ux-fixes`
**Created**: 2026-07-19
**Status**: Draft
**Input**: User report (Czech) after GUI-testing the feature-021 WebView2 build:
six concrete defects in the F3 Markdown viewer.

**Relationship**: Bug-fix increment on feature 021 (mdview HTML rendering
surface). No new dependencies; no architectural change. Fixes defects in the
WebView2 host (`webview.cpp`) and the viewer window (`viewer.cpp`).

---

## Overview

Feature 021 replaced mdview's rendering with a locked-down WebView2 surface.
Live GUI testing surfaced six usability defects, all in keyboard/mouse
interaction and two viewer commands:

1. After F3, keyboard scrolling (arrows / PgUp / PgDn) does nothing until the
   user clicks into the view to give it focus.
2. Zoom reset **Ctrl+0** works only for the top-row `0`, not the numeric-keypad
   `0`.
3. There is no indication of the current zoom level anywhere.
4. Zoom does not respond to **Ctrl + mouse wheel**.
5. Search (**Ctrl+F**) opens the dialog, but "Find Next" / Enter does nothing —
   no match is located or highlighted.
6. **File → Open as Text (Ctrl+U)** does nothing.

All six are corrected while preserving the feature-021 security invariants and
the single HTML rendering backend.

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Keyboard scrolling works immediately after F3 (Priority: P1)

A user presses F3 on a long Markdown file and immediately uses the arrow keys,
PgUp/PgDn, Home/End to scroll the rendered document — without first clicking
into the window.

**Why this priority**: Scrolling is the most basic viewer interaction; needing
a mouse click first is a constant friction on every open.

**Independent Test**: Open a document taller than the window with F3; without
clicking, press PgDn/Down-arrow and verify the view scrolls.

**Acceptance Scenarios**:

1. **Given** a freshly F3-opened document, **When** the user presses PgDn or
   Down-arrow without clicking, **Then** the rendered content scrolls.
2. **Given** focus moved elsewhere (e.g. the menu) and back to the viewer,
   **When** the window regains focus, **Then** keyboard scrolling works again
   without a click.

---

### User Story 2 - Zoom is discoverable and fully controllable (Priority: P1)

A user changes the zoom with Ctrl+Plus / Ctrl+Minus, **Ctrl+mouse-wheel**, and
resets with **Ctrl+0** (both the top-row and the numeric-keypad `0`), and can
see the current zoom percentage in the window title.

**Why this priority**: Zoom is a core viewer control; two of its inputs are
broken and the current level is invisible.

**Independent Test**: Zoom in/out with the wheel and keys; verify the title
shows the live percentage and both `0` keys reset to 100%.

**Acceptance Scenarios**:

1. **Given** a rendered document, **When** the user holds Ctrl and scrolls the
   mouse wheel, **Then** the content zooms in/out.
2. **Given** any zoom level, **When** the user presses Ctrl+0 using the numeric
   keypad `0`, **Then** the zoom resets to 100%.
3. **Given** any zoom change (wheel, keys, or menu), **When** it takes effect,
   **Then** the window title shows the current zoom percentage (e.g. "… (125%)").
4. **Given** a chosen zoom level, **When** the user opens another document or
   restarts, **Then** the zoom level persists (unchanged from v1 behavior).

---

### User Story 3 - Search finds and highlights matches (Priority: P1)

A user presses Ctrl+F, types a term, and clicks "Find Next" (or presses Enter);
the viewer scrolls to and highlights the first match. Pressing F3 / Shift+F3
moves to the next / previous match, wrapping around.

**Why this priority**: Search is a headline v1 feature (Decision Q4) and is
currently completely non-functional.

**Independent Test**: Open a document with several occurrences of a word;
Ctrl+F, type it, Enter → first occurrence is highlighted and scrolled into
view; F3 advances through the rest and wraps.

**Acceptance Scenarios**:

1. **Given** a document containing the search term, **When** the user confirms
   the Find dialog, **Then** the first match is highlighted and scrolled into
   view.
2. **Given** a located match, **When** the user presses F3 (Find Next), **Then**
   the next match is highlighted; at the end it wraps to the first.
3. **Given** a term with no matches, **When** the user confirms, **Then** a
   "not found" message is shown.
4. **Given** a new search term entered via Ctrl+F, **When** confirmed, **Then**
   the previous highlights are replaced by the new term's matches.

---

### User Story 4 - "Open as Text" works (Priority: P2)

A user viewing a rendered document invokes File → Open as Text (Ctrl+U) and is
shown the raw Markdown source of the file.

**Why this priority**: A visible menu command that does nothing is a defect;
seeing the raw source is a useful escape hatch, but it is secondary to the
core interaction fixes.

**Independent Test**: Open a `.md`, invoke Ctrl+U, verify the raw Markdown text
is displayed (not the rendered view); invoke again to return to the rendered
view.

**Acceptance Scenarios**:

1. **Given** a rendered document, **When** the user invokes Open as Text
   (Ctrl+U or the menu), **Then** the raw Markdown source is displayed as plain
   monospaced text.
2. **Given** the source view, **When** the user invokes the command again,
   **Then** the rendered view returns.
3. **Given** the source view, **When** the user searches or zooms, **Then**
   those still work on the source text.

---

### Edge Cases

- Focus: if the WebView2 surface is not yet ready when the window first gains
  focus, focus must transfer to the content as soon as it is ready.
- Zoom: engine-driven wheel zoom and app-driven key/menu zoom must not
  double-apply or fight each other; the persisted value must reflect the last
  effective zoom.
- Search: changing the term must reload the marked content (not scroll stale
  marks); moving between matches of the same term should not reload the whole
  document unnecessarily.
- Open as Text: toggling source view must preserve the current scheme and the
  ability to return to the rendered view; if the rendering engine is entirely
  unavailable, the file must still be viewable as text.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: After a document is opened (F3) and rendered, keyboard input
  (arrows, PgUp/PgDn, Home/End, Space) MUST scroll the content without a prior
  mouse click; focus MUST be placed on the rendering surface once it is ready.
- **FR-002**: When the viewer window regains focus, the rendering surface MUST
  receive keyboard focus so scrolling continues to work.
- **FR-003**: Zoom MUST respond to Ctrl + mouse wheel.
- **FR-004**: Zoom reset (Ctrl+0) MUST work for both the top-row `0` and the
  numeric-keypad `0`.
- **FR-005**: Zoom in/out MUST continue to work via Ctrl+Plus/Minus and the
  View menu; the effective zoom level MUST be a single consistent value.
- **FR-006**: The current zoom percentage MUST be shown in the window title and
  update whenever the zoom changes.
- **FR-007**: The zoom level MUST persist across documents and sessions
  (unchanged from v1).
- **FR-008**: Confirming the Find dialog MUST locate, highlight, and scroll to
  the first match of the term in the rendered document.
- **FR-009**: Find Next / Find Previous (F3 / Shift+F3) MUST move to the next /
  previous match with wrap-around.
- **FR-010**: Entering a new term MUST replace the previous term's highlights
  with the new matches.
- **FR-011**: A search with no matches MUST show a "not found" message.
- **FR-012**: "Open as Text" (Ctrl+U and the File menu) MUST display the raw
  Markdown source of the current file as plain monospaced text, and MUST be
  toggleable back to the rendered view.
- **FR-013**: If the rendering engine cannot initialize at open time, the file
  MUST still be viewable as text (handled where the plugin runs on the
  application's main thread).
- **FR-014**: All feature-021 security invariants MUST continue to hold; none
  of these fixes may enable scripts, network access, or navigation beyond the
  document. Source view is inert escaped text.

### Non-Functional Requirements

- **NFR-001**: No regression to existing viewer behavior (schemes, links,
  encoding, size gate, long paths, fallback).
- **NFR-002**: Fixes are contained in the mdview plugin; no changes to shared
  infrastructure, the build system, or other plugins.

---

## Success Criteria *(mandatory)*

- **SC-001**: On a freshly F3-opened long document, PgDn/arrow scrolling works
  with zero prior clicks in 100% of opens.
- **SC-002**: Ctrl+wheel zoom, Ctrl+Plus/Minus, both Ctrl+0 keys, and the menu
  all change zoom; the title always shows the current percentage.
- **SC-003**: For a document with N≥3 occurrences of a term, confirming Find
  highlights match 1 and F3 cycles through all N and wraps — verified visually.
- **SC-004**: Open as Text shows the raw source and toggles back; search and
  zoom work in source view.
- **SC-005**: The build is clean (Debug x64) and the htmlgen unit tests
  (including a source-view case) pass.
- **SC-006**: Zero new security regressions (adversarial corpus still inert).

---

## Assumptions

- **"Open as Text" is realized as an in-window raw-source view** (a toggle in
  the same WebView2 window) for the user-initiated command, rather than handing
  off to Salamander's separate internal text viewer. Rationale: the mdview
  viewer runs in its own thread, and `ViewFileInPluginViewer` is documented as
  main-thread-only, so a cross-thread hand-off is unreliable; an in-window
  source view is robust, keeps zoom/search working, and matches the user intent
  of "see the file as text". The internal text viewer is still used for the
  engine-unavailable fallback, invoked on the main thread from `ViewFile`.
- The zoom range and exact wheel step follow the engine's native zoom behavior;
  the app keeps the persisted percentage in sync.
- No clarification round is needed — the defects and desired behavior are
  concrete and unambiguous.

## Dependencies

- Feature 021 (mdview WebView2 rendering surface) — this fixes defects in it.
- WebView2 controller APIs already available in the vendored SDK: `MoveFocus`,
  `add_ZoomFactorChanged`, `get/put_ZoomFactor`, `put_IsZoomControlEnabled`.
