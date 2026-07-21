# Feature Specification: Switchable Visual Themes (Default + Dark)

**Feature Branch**: `028-visual-themes`
**Created**: 2026-07-21
**Status**: Draft
**Input**: User description: "Proveď detailní analýzu celého projektu a navrhni
jak by bylo možné implementovat možnost přepínání vizuálních stylů (themes)
v celém programu, tedy aby bylo možné např. přepnout celý program do tmavého
stylu, se vším všudy - grafika, pozadí, barvy, ikony, prostě tak aby to sedělo.
Výchozí vizuální styl bude takový jaký je program nyní. Možnost přepínání stylů
bude dostupné v menu. Prozatím v rámci tohoto rozšíření navrhni jeden nový
styl - tmavý. Tedy po dokončení implementace budou k dispozici dva styly -
výchozí (současný, tak jak program vypadá nyní) a nový tmavý styl."

## Clarifications

### Session 2026-07-21

- Q: While the Dark theme is active, how should it interact with the existing
  panel color-scheme customization (Colors page)? → A: Theme is a separate,
  independent setting. Dark uses a built-in dark panel palette that is not
  user-editable in this feature; the user's color-scheme choice (including
  Custom colors) applies to the Default theme only and is restored untouched
  when switching back.
- Q: What form should the theme switch take in the user interface? → A: A
  "Theme" submenu in the Options menu with radio-checked items "Default" and
  "Dark" (active one checked). No Configuration-dialog page in this feature.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Switch the whole main window to the Dark theme from the menu (Priority: P1)

A user who prefers dark interfaces (low-light work, eye strain, personal
preference) opens the program's menu, picks the Dark theme, and the entire
main window — both file panels, panel captions and directory lines, the
information line, toolbars, the main menu bar and its drop-down menus, the
command line, the bottom function-key bar, and the window title bar —
immediately changes to a coherent dark appearance: dark backgrounds, light
readable text, and icons/graphics adjusted so nothing looks out of place.
Picking the Default theme returns the program to exactly its current look.

**Why this priority**: This is the core of the feature — a visible,
all-encompassing theme switch of the primary working surface. Without it
there is no feature; with it alone the feature is already valuable.

**Independent Test**: Can be fully tested by launching the program, selecting
the Dark theme from the menu, and visually inspecting every region of the
main window for dark styling; then selecting Default and confirming the
appearance matches the pre-feature program.

**Acceptance Scenarios**:

1. **Given** the program is running with the Default theme, **When** the user
   selects the Dark theme from the menu, **Then** all main-window surfaces
   (panels, panel captions/directory lines, info line, toolbars, menu bar,
   drop-down menus, command line, function-key bar, title bar) switch to the
   dark appearance immediately, without restarting the program.
2. **Given** the Dark theme is active, **When** the user opens any drop-down
   menu or hovers toolbar buttons, **Then** menus and hover/pressed states
   render in dark-appropriate colors with readable text and visible icons.
3. **Given** the Dark theme is active, **When** the user selects the Default
   theme, **Then** the program returns to an appearance identical to the
   program before this feature existed.
4. **Given** either theme is active, **When** the user opens the theme menu,
   **Then** the currently active theme is clearly indicated (checked/selected
   state).

---

### User Story 2 - The chosen theme survives program restart (Priority: P2)

A user selects the Dark theme, closes the program, and starts it again the
next day. The program opens already in the Dark theme — no light flash of
the old appearance, no need to re-select anything.

**Why this priority**: A theme that resets on every start is effectively
unusable as a preference; persistence makes the choice durable.

**Independent Test**: Select Dark theme, exit the program, relaunch, and
verify the main window appears dark from the first frame shown; repeat in
the opposite direction for Default.

**Acceptance Scenarios**:

1. **Given** the user selected the Dark theme, **When** the program is closed
   and started again, **Then** it starts directly in the Dark theme.
2. **Given** a fresh installation or a configuration with no theme choice
   recorded, **When** the program starts, **Then** the Default theme is used
   and the program looks exactly as it does today.

---

### User Story 3 - Dialogs and secondary windows follow the theme (Priority: P3)

With the Dark theme active, the user opens dialogs and secondary windows —
the configuration dialog, Find window, copy/move progress dialogs, message
boxes, the internal file viewer — and they all appear in the dark style:
dark backgrounds, readable text, controls that fit the theme.

**Why this priority**: Dialogs are opened constantly during real work; a
dark main window with blinding white dialogs breaks the "everything fits
together" goal, but the main-window switch (P1) is still useful without it.

**Independent Test**: With Dark theme active, open each of the commonly used
windows (configuration, Find, viewer, a file-operation progress dialog, an
error/message box) and verify dark styling and readability in each.

**Acceptance Scenarios**:

1. **Given** the Dark theme is active, **When** the user opens the
   configuration dialog, Find window, or a message box, **Then** the window
   renders with dark backgrounds and readable light text, including lists,
   buttons, input fields, checkboxes, and tabs/trees.
2. **Given** the Dark theme is active, **When** the user views a text file in
   the internal viewer, **Then** the viewer content area uses a dark
   color scheme by default.
3. **Given** the Dark theme is active and a themed window is open, **When**
   the user switches back to the Default theme, **Then** that window renders
   in the default style no later than the next time it is opened (immediately
   where already-open windows can be refreshed).

---

### User Story 4 - Graphics and icons look right in the Dark theme (Priority: P4)

With the Dark theme active, all imagery "sits" in the dark design: toolbar
icons, menu glyphs, function-key bar symbols, progress bars, selection and
focus markers. Nothing appears as a light rectangle around an icon, no icon
becomes invisible against the dark background, and file/folder icons remain
recognizable.

**Why this priority**: This is the polish that makes the theme feel
intentional ("se vším všudy") rather than a recolor; it builds on P1–P3.

**Independent Test**: With Dark theme active, visually sweep all toolbars,
menus, the function-key bar, and panel item states (normal, selected,
focused, highlighted) checking icon legibility and absence of light artifacts.

**Acceptance Scenarios**:

1. **Given** the Dark theme is active, **When** toolbars and menus are
   displayed, **Then** their icons are rendered against dark backgrounds
   without light halos/rectangles and remain clearly recognizable.
2. **Given** the Dark theme is active, **When** files are listed in the
   panels, **Then** standard file/folder icons remain legible on the dark
   item backgrounds in all item states (normal, selected, focused, hidden).

---

### Edge Cases

- **Windows High Contrast mode**: when the operating system's high-contrast
  accessibility mode is active, accessibility takes precedence — the program
  follows the system's high-contrast colors as it does today, regardless of
  the selected theme.
- **User-customized panel colors**: a user may have customized the existing
  panel color scheme (e.g. "Custom" colors). Activating the Dark theme
  presents its built-in dark palette; switching back to Default restores the
  user's previous color-scheme selection and custom colors untouched. Any
  color-scheme changes made while the Dark theme is active affect the
  Default theme's appearance, not the dark palette.
- **Windows-owned surfaces**: windows drawn by the operating system or third
  parties (shell context menus, common Open/Save dialogs, shell property
  pages, other applications' windows launched from the program) follow the
  operating system's own theming, not the program's theme. This is expected
  and out of scope.
- **Plugin-owned user interface**: plugins that obtain colors from the
  program follow the active theme automatically; any additional plugin
  windows that use their own fixed colors may remain light. Restyling
  plugin-internal UI beyond the program-provided color mechanism is out of
  scope for this feature.
- **Theme switch while windows are open**: secondary windows that cannot be
  restyled in place adopt the new theme when reopened; the main window always
  updates immediately.
- **Reduced color environments** (e.g. some remote-desktop sessions): the
  program continues to function; the dark theme may degrade gracefully in
  fidelity but must remain readable.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The program MUST offer exactly two visual themes: **Default**
  (the program's current appearance, unchanged) and **Dark**.
- **FR-002**: The theme MUST be switchable from a "Theme" submenu in the
  Options area of the main menu, listing Default and Dark as mutually
  exclusive (radio-checked) items with the currently active theme checked.
- **FR-003**: Selecting a theme MUST apply it to the main window immediately,
  without restarting the program.
- **FR-004**: The selected theme MUST be persisted with the program's
  configuration and restored on the next start. When no choice has been
  recorded (fresh install, upgraded configuration), the Default theme MUST
  be used.
- **FR-005**: With the Default theme active, the program MUST look exactly as
  it does before this feature — no visual differences introduced.
- **FR-006**: The Dark theme MUST cover all main-window surfaces: file
  panels (all item states), panel captions and directory lines, the
  information line, all toolbars, the main menu bar and all drop-down menus,
  the command line, the bottom function-key bar, and the window title bar
  (where the operating system allows applications to darken it).
- **FR-007**: The Dark theme MUST cover windows and dialogs created by the
  program itself — including the configuration dialog, Find window,
  file-operation progress dialogs, message boxes, and password/confirmation
  prompts — with dark backgrounds and readable controls.
- **FR-008**: The internal file viewer MUST provide a dark color scheme that
  is active by default when the Dark theme is selected, while respecting any
  explicit viewer color customization the user makes afterwards.
- **FR-009**: Icons, glyphs, and other imagery displayed by the program
  (toolbar icons, menu marks, function-key bar symbols, progress and focus
  indicators) MUST be adjusted or re-rendered so they remain clearly legible
  and visually consistent in the Dark theme, with no light artifacts around
  them.
- **FR-010**: All text in the Dark theme MUST remain readable — standard
  text must have clearly sufficient contrast against its background in all
  item and control states.
- **FR-011**: The theme MUST be a setting separate from the existing panel
  color-scheme customization. The Dark theme supplies its own built-in
  coherent panel palette, which is not user-editable within this feature;
  the user's previously selected scheme (including custom colors) continues
  to apply to the Default theme and is preserved and restored untouched when
  returning to it.
- **FR-012**: Plugins that obtain display colors from the program MUST
  automatically receive the active theme's colors, so plugin surfaces built
  on the program's color mechanism follow the theme without plugin changes.
- **FR-013**: When the operating system's high-contrast accessibility mode is
  active, the program MUST respect the system's accessibility colors as it
  does today; the application theme must not override them.
- **FR-014**: Switching themes MUST NOT disrupt ongoing work: running file
  operations, open panels, selections, and the command-line content must be
  unaffected by the switch.

### Key Entities

- **Visual Theme**: a named, complete visual definition of the program's
  appearance — background and text colors for every surface, state colors
  (selection, focus, hover), and the imagery variants that fit them. Two
  instances exist: Default and Dark.
- **Theme Selection**: the user's persisted choice of active theme; part of
  the program configuration, restored at startup.
- **Panel Color Scheme** (existing): the current user-facing color
  customization for panel items and the viewer; related to but not replaced
  by themes — the Dark theme provides its own palette while preserving the
  user's scheme choice for the Default theme.
- **Themed Surface Inventory**: the enumerated list of program surfaces
  (main-window regions, dialogs, secondary windows, imagery) that a theme
  must cover; serves as the completeness checklist for acceptance.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A user can switch themes in at most 3 interactions starting
  from the main menu, and the main window reflects the new theme in under
  2 seconds without restarting the program.
- **SC-002**: The chosen theme is restored on 100% of program restarts, and
  a configuration with no recorded choice always starts in the Default theme.
- **SC-003**: With the Default theme active, a side-by-side comparison with
  the pre-feature program finds zero visual differences across the main
  window and all program dialogs, with the sole exception of the new "Theme"
  submenu in the Options menu.
- **SC-004**: With the Dark theme active, 100% of the main-window surfaces
  and at least 95% of program-created dialogs and secondary windows in the
  Themed Surface Inventory render dark — no white/light panels, stripes, or
  control backgrounds — as verified by a documented visual walkthrough.
- **SC-005**: In the Dark theme, standard text meets a contrast ratio of at
  least 4.5:1 against its background in all common states (normal, selected,
  focused; disabled text readable at 3:1).
- **SC-006**: 20 consecutive theme switches during normal use (panels
  populated, viewer and Find windows opened between switches) complete
  without a crash, hang, drawing corruption, or loss of panel state.
- **SC-007**: Program startup time and panel drawing performance with either
  theme active show no measurable regression compared to the pre-feature
  program (within normal run-to-run variance).

## Assumptions

- The theme switch lives in the existing **Options** menu as a "Theme"
  submenu with radio-checked Default/Dark items (per Clarifications); no
  Configuration-dialog page for themes is part of this feature.
- Manual switching only: automatic following of the Windows light/dark
  setting is **not** part of this feature (noted as a possible future
  enhancement).
- The Dark theme changes colors and imagery only — no layout, spacing, font,
  or behavioral changes relative to the Default theme.
- Surfaces owned by the operating system or other applications (shell
  context menus, common Open/Save dialogs, shell property sheets) are out of
  scope; they follow the OS.
- Plugin-internal UI is themed only to the extent plugins already consume
  program-provided colors; requiring plugin code changes is out of scope.
- The feature targets the main program; the crash reporter, installer, and
  other auxiliary executables keep their current appearance.
- Windows High Contrast accessibility mode takes precedence over the
  application theme.
- The existing configuration storage mechanism will hold the theme choice;
  older configurations without it silently default to the Default theme (no
  migration step needed).
- A supporting technical survey of the program's visual architecture
  (existing color system, drawn surfaces, icon handling, persistence) was
  captured during specification and is stored alongside this spec in
  `analysis/visual-architecture-survey.md` for use by the planning phase.
