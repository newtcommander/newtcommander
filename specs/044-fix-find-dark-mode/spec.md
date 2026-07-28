# Feature Specification: Fix Find Window Dark-Mode Rendering

**Feature Branch**: `044-fix-find-dark-mode`
**Created**: 2026-07-28
**Status**: Draft
**Input**: User description: "Oprav zobrazeni okna pro vyhledavani souboru
(tj. CTRL+F) v tmavem rezimu. Analyzuj screenshot tohoto okna v tmavem rezimu
v ./temp/dark_find_window.png. Jsou v nem bile linky, ktere oddeluji
horizontalne jednotlive casti, nektera textova pole maji cerny text na cernem
pozadi a status bar na spodni strane okna je take cely svetly. Nacti v
historii projektu v specs jak jsme toto jiz resili pro hlavni okno aplikace a
pouc se z toho."

## Observed Defects *(from screenshot `temp/dark_find_window.png`)*

With the Dark theme active, the Find window (Ctrl+F) opens mostly dark, but
three classes of light-mode remnants break the appearance:

1. **Light separator lines** — the horizontal lines that visually divide the
   window's sections (e.g., the line running beside the "Search file content"
   checkbox) render bright white instead of a subdued dark-theme color.
2. **Unreadable or light-framed text fields** — some text fields show dark
   text on a dark background (illegible), and the advanced-options summary
   box (showing "No Advanced Options") is drawn with a bright white frame
   that stands out against the dark window.
3. **Light status bar** — the status strip along the bottom of the window
   (showing hints such as "Specify find options and click Find Now.") is
   entirely light with dark text, the most prominent bright artifact in the
   window.

The dark theme itself, including live theming of the Find window, was
introduced in feature 028 (`specs/028-visual-themes/`); this feature fixes
the residual defects that work missed, following the conventions later
reaffirmed in features 036 and 037.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - The Find window opens fully dark (Priority: P1)

A user working in the Dark theme presses Ctrl+F to search for files. The
Find window appears as a coherent dark surface: the horizontal lines
separating its sections are subdued and dark-appropriate, the
advanced-options summary box no longer glows with a white frame, and the
status bar along the bottom is dark with light, readable text. Nothing in
the window flashes or stands out as a leftover light-mode element.

**Why this priority**: These are the immediately visible defects present the
moment the window opens — every dark-theme user sees them on every search.
Fixing them delivers the core value of the feature on its own.

**Independent Test**: Switch to the Dark theme, press Ctrl+F, and visually
inspect the freshly opened window: section separator lines, the
advanced-options summary box, and the bottom status bar must all render
dark; compare against `temp/dark_find_window.png` to confirm each captured
defect is gone.

**Acceptance Scenarios**:

1. **Given** the Dark theme is active, **When** the user opens the Find
   window with Ctrl+F, **Then** all horizontal section-separator lines
   render in a subdued dark-theme color — no white or light lines anywhere
   in the window.
2. **Given** the Dark theme is active, **When** the Find window is shown,
   **Then** the status bar at the bottom has a dark background with light,
   clearly readable text.
3. **Given** the Dark theme is active, **When** the Find window is shown
   with no advanced options set, **Then** the advanced-options summary box
   renders with a dark-appropriate frame and background, and its text
   ("No Advanced Options") is readable.

---

### User Story 2 - Every text field is readable in every state (Priority: P2)

A user in the Dark theme fills in the Find window: types a name pattern,
picks a location, enables "Search file content" to reveal the content-search
fields, and sets advanced options so the summary box shows them. In every
one of these states, every text field — editable or read-only, enabled or
disabled — shows readable light text on a dark background. No field ever
shows dark text on a dark background.

**Why this priority**: Illegible fields make parts of the window unusable,
not merely ugly — but some of them appear only after the user expands or
fills sections, so they are encountered slightly less often than the
always-visible defects of Story 1.

**Independent Test**: In the Dark theme, walk the window through its states
(initial, "Search file content" enabled, advanced options set, fields
disabled by their controlling checkboxes) and confirm every text field's
content is readable in each state.

**Acceptance Scenarios**:

1. **Given** the Dark theme is active, **When** the user views any text
   field in the Find window in any of its states (empty, filled, focused,
   disabled), **Then** the field's text is light on a dark background and
   clearly readable — never dark-on-dark.
2. **Given** the Dark theme is active, **When** the user enables "Search
   file content" so additional fields and options become visible, **Then**
   the newly revealed fields, labels, and any separators around them render
   dark with readable text.
3. **Given** the Dark theme is active, **When** the user defines advanced
   options so the summary box displays them, **Then** the summary text is
   readable against the box's dark background.

---

### User Story 3 - Search progress, theme switching, and light mode stay correct (Priority: P3)

The fixes hold up beyond the window's idle state: while a search is running,
the status bar shows progress text dark and readable; when the user switches
themes while the Find window is open, the window — including the fixed
elements — follows the switch as the application's theming convention
dictates; and in the Default (light) theme the Find window looks exactly as
it does today, pixel for pixel.

**Why this priority**: These are correctness guarantees around the fix —
essential for not trading one defect for another, but only observable in
transitional or non-default situations.

**Independent Test**: Run a search in the Dark theme and watch the status
bar; toggle Default ↔ Dark with the window open; then compare the Default
theme Find window side by side with the current release.

**Acceptance Scenarios**:

1. **Given** the Dark theme is active and a search is running, **When** the
   status bar shows progress or result summaries, **Then** the text remains
   light on a dark background throughout the search and after it completes.
2. **Given** the Find window is open, **When** the user switches between
   Default and Dark themes, **Then** the separator lines, text fields, and
   status bar all adopt the new theme the same way the rest of the Find
   window does (the Find window re-themes live, per the convention
   established in feature 028).
3. **Given** the Default theme is active, **When** the user opens and uses
   the Find window, **Then** its appearance is unchanged from the current
   release — zero visual differences.

---

### Edge Cases

- **"Search file content" expanded**: enabling the checkbox reveals further
  fields, option checkboxes, and separators; all revealed elements must
  render correctly in the Dark theme, including any that are disabled until
  their controlling option is checked.
- **Disabled fields**: fields disabled by their controlling checkboxes must
  remain distinguishable as disabled yet still readable in the Dark theme
  (dimmed light text, not black-on-black).
- **Status bar content changes**: the status bar cycles through hint text,
  live search progress, and result summaries; every state must render dark
  with readable text — including long paths that get clipped.
- **Multiple Find windows**: the application allows several Find windows at
  once; every one of them must render correctly, including windows opened
  before and after a theme switch.
- **Windows High Contrast mode**: as established in feature 028, the
  system's high-contrast accessibility colors take precedence over the
  application theme; this feature must not interfere with that behavior.
- **Results present**: the results list and its column headers already
  render dark correctly; the fixes must not regress them.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: In the Dark theme, all horizontal section-separator lines in
  the Find window MUST render in a subdued color consistent with the dark
  palette — no white or light-gray lines.
- **FR-002**: In the Dark theme, every text field in the Find window —
  editable, read-only (including the advanced-options summary box), enabled,
  or disabled — MUST display readable light text on a dark background in
  every state of the window (initial, content-search expanded, advanced
  options set, during and after a search).
- **FR-003**: In the Dark theme, field frames and borders (including the
  advanced-options summary box frame) MUST use dark-appropriate colors — no
  bright white outlines.
- **FR-004**: In the Dark theme, the status bar at the bottom of the Find
  window MUST render with a dark background and readable light text in all
  its states: idle hint, live search progress, and result summaries.
- **FR-005**: With the Default theme active, the Find window MUST look
  exactly as it does in the current release — this feature introduces zero
  visual changes to the light appearance.
- **FR-006**: When the theme is switched while a Find window is open, the
  corrected elements (separators, fields, status bar) MUST follow the switch
  in the same way the rest of the Find window does, per the live re-theming
  convention established in feature 028.
- **FR-007**: When the operating system's High Contrast mode is active, the
  Find window MUST continue to follow the system's accessibility colors;
  the fixes must not override them.
- **FR-008**: The corrections MUST apply consistently to every Find window
  instance when several are open at once.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A documented visual walkthrough of the Find window in the Dark
  theme — covering initial state, content-search expanded, advanced options
  set, a running search, and completed results — finds zero white or light
  artifacts: no light separator lines, no light field frames, no light
  status bar. Every defect visible in `temp/dark_find_window.png` is gone.
- **SC-002**: In the Dark theme, text in every Find-window field and in the
  status bar meets a contrast ratio of at least 4.5:1 against its background
  (disabled text at least 3:1), matching the standard set for the dark theme
  in feature 028.
- **SC-003**: In the Default theme, a side-by-side comparison of the Find
  window with the current release shows zero visual differences.
- **SC-004**: 10 consecutive Default ↔ Dark theme switches with a Find
  window open (including one with a search running) complete without a
  crash, drawing corruption, or any element left in the wrong theme.

## Assumptions

- The dark theme infrastructure from feature 028 (`specs/028-visual-themes/`)
  is in place and already themes most of the Find window, including live
  re-theming on switch; this feature is a targeted defect fix within that
  framework, not new theming machinery.
- The defect inventory is taken from the provided screenshot
  (`temp/dark_find_window.png`) plus the user's report; the phrase "some
  text fields have black text on a black background" is interpreted as
  covering **all** window states, so the fix is specified for every field in
  every state rather than an enumerated subset.
- Scope is limited to the Find window (Ctrl+F) and the surfaces it directly
  contains (separators, labels, text fields, advanced-options summary box,
  status bar). Other dialogs, the main window, and plugins are out of scope.
- Elements of the Find window that already render correctly in the Dark
  theme (menu bar, combo boxes, buttons, results list, results toolbar) are
  out of scope except for the no-regression guarantee.
- Conventions established in features 028/036/037 carry over unchanged:
  Default-theme appearance is untouchable, Windows High Contrast mode takes
  precedence, and theme switches follow the application-wide convention.
- No layout, spacing, font, or behavioral changes — colors and rendering
  only.
