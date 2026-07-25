# Feature Specification: Dark Theme for Plugin Windows and Dialogs

**Feature Branch**: `036-plugin-dark-theme`
**Created**: 2026-07-25
**Status**: Draft
**Input**: User description: "Cilem upravy bude aplikace tmaveho stylu viz Options->Themes->Dark i na polozky, okna a pop-up okna pluginu tak, aby byl vzhled konzistentni. Nyni kdyz napr. pouzivam SFTP plugin v tmavem rezimu, tak se zobrazi svetly. Cela aplikace tedy i pluginy museji prebirat nastaveni barevneho tematu."

## Clarifications

### Session 2026-07-25

- Q: What is the delivery scope of feature 036? → A: Everything in this one feature: the theme mechanism in the plugin interface plus dark styling of all 18 plugins in the default distribution, delivered incrementally via the prioritized user stories (SFTP first).
- Q: What happens to plugin windows already open at the moment of a theme switch? → A: Reopen is sufficient — newly opened windows always match the new theme; already-open windows keep their previous consistent appearance and adopt the theme when closed and reopened. Immediate live repaint is not required.
- Q: How does the Dark theme treat the content area of viewer plugins? → A: Text/document content (markdown, database tables, text listings) renders dark — light text on a dark background, like modern editors. Images and binary data are never recolored.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Plugin dialogs follow the Dark theme (Priority: P1)

A user works with the Dark theme active (Options → Theme → Dark, introduced
in feature 028). They open a plugin-provided window — the SFTP plugin's
connection dialog, a password prompt, a transfer progress window, an error
box — and it appears dark: dark backgrounds, light readable text, controls
styled consistently with the rest of the application. Today these plugin
surfaces appear in the light style, which visually breaks the dark
environment (a bright white dialog over a dark main window).

**Why this priority**: This is the reported pain — the SFTP plugin is the
exemplar the user named. A dark application with blinding light plugin
dialogs defeats the purpose of the Dark theme for anyone using plugins
daily.

**Independent Test**: Activate the Dark theme, open the SFTP plugin's
connection dialog, connect to a server, trigger a password prompt, a file
transfer with progress, and an error message; every one of these surfaces
renders dark with readable text.

**Acceptance Scenarios**:

1. **Given** the Dark theme is active, **When** the user opens the SFTP
   plugin's connection/site dialog, **Then** the dialog renders with the
   dark palette (background, text, list/tree items, buttons, edit fields)
   consistent with the application's own dark dialogs.
2. **Given** the Dark theme is active, **When** an SFTP password prompt,
   progress window, or error pop-up appears, **Then** it renders dark with
   readable text — no light-on-light or dark-on-dark combinations.
3. **Given** the Default theme is active, **When** the user opens the same
   plugin surfaces, **Then** they look exactly as they do today (no visual
   change to the light appearance).

---

### User Story 2 - Every shipped plugin is theme-consistent (Priority: P2)

The user moves through the whole product with the Dark theme active:
archive plugin dialogs (pack/unpack options, password prompts), viewer
windows (picture viewer, markdown viewer, database viewer), network plugin
dialogs (FTP), utility dialogs (compare, checksum, undelete…), and each
plugin's configuration dialog. Every plugin-provided window, dialog and
pop-up follows the active theme, so the product looks like one coherent
application, not a mix of dark core and light plugins.

**Why this priority**: The user's goal is explicit — "the whole application,
including plugins, must adopt the color theme." Fixing only SFTP would
leave the same inconsistency everywhere else.

**Independent Test**: With the Dark theme active, open at least one
representative window/dialog from each plugin shipped in the default build
and visually audit it against a per-plugin checklist; every audited surface
is dark and readable.

**Acceptance Scenarios**:

1. **Given** the Dark theme is active, **When** the user opens any window,
   dialog, or pop-up of any plugin shipped in the default distribution,
   **Then** it renders in the dark appearance consistent with the core
   application.
2. **Given** the Dark theme is active, **When** a plugin viewer window
   displays text or document content (markdown, database tables, text
   listings), **Then** both the window chrome and the content area render
   dark — light text on a dark background; **When** it displays an image
   or binary data, **Then** that content is shown faithfully without
   recoloring while the surrounding chrome stays dark.
3. **Given** the Dark theme is active, **When** the user opens a plugin's
   configuration dialog (from the plugin's menu or Plugin Manager), **Then**
   it renders dark like the application's own configuration dialog.

---

### User Story 3 - Theme switching propagates to plugins without restart (Priority: P3)

The user switches between Default and Dark themes while the application is
running. Plugin windows opened after the switch always match the newly
selected theme. Plugin windows that are already open keep their previous
consistent appearance and adopt the new theme when closed and reopened
(clarified 2026-07-25: live repaint of open windows is not required) — the
application never needs a restart.

**Why this priority**: Matches the no-restart behavior the core application
already provides (feature 028); without it the feature feels broken right
after a switch, but it is a smaller concern than the steady-state
consistency of US1/US2.

**Independent Test**: With a plugin window open, switch Default → Dark and
back; verify newly opened plugin windows always match the active theme and
already-open windows adopt at latest on reopen, with no crash or visual
corruption at the moment of switching.

**Acceptance Scenarios**:

1. **Given** any theme is active, **When** the user switches the theme and
   then opens a plugin window, **Then** the newly opened window matches the
   newly selected theme in 100% of cases.
2. **Given** a plugin window is open during a theme switch, **When** the
   switch happens, **Then** the window keeps its previous consistent
   appearance until reopened — it must not crash, garble, or end up
   half-switched within one surface.

---

### Edge Cases

- What happens to a plugin window that is open at the moment of a theme
  switch? It stays fully in the old appearance until reopened (clarified
  2026-07-25) — never a mix of both themes within one window.
- What about third-party or legacy plugins that do not use the theme
  mechanism? They MUST continue to load and function exactly as today
  (light appearance in dark mode is acceptable for them); the feature must
  not crash or visually corrupt them.
- What about operating-system-drawn elements inside plugin flows (common
  Open/Save dialogs, shell context menus, system message boxes)? Out of
  scope — the same boundary feature 028 established for the core.
- What about content areas of viewer plugins? Text/document content
  (markdown, tables, listings) renders dark like the rest of the theme; a
  photograph in the picture viewer or other image/binary content is never
  artificially recolored — only its surrounding chrome is dark.
- What if a plugin surface mixes themed and unthemed elements after the
  change (e.g. a dark dialog with one light control)? This counts as a
  defect — each surface must be audited as a whole.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: With the Dark theme active, every window, dialog, and pop-up
  of every plugin shipped in the default distribution MUST render in the
  dark appearance — backgrounds, text, buttons, edit fields, lists, trees,
  headers, status areas — visually consistent with the core application's
  dark dialogs.
- **FR-002**: The SFTP plugin's complete user-facing surface (connection/
  site management, password and confirmation prompts, progress windows,
  error boxes, context dialogs) MUST follow the active theme.
- **FR-003**: The application MUST expose the active theme (and its colors)
  to plugins through its plugin interface so any plugin — including
  third-party ones — can follow the theme; the mechanism MUST be available
  to all plugins, not only the shipped set.
- **FR-004**: With the Default theme active, all plugin surfaces MUST look
  exactly as they do today — zero visual change for users who never switch
  to Dark.
- **FR-005**: A theme switch MUST NOT require an application restart:
  plugin windows opened after the switch MUST match the new theme; plugin
  windows already open keep their previous consistent appearance and MUST
  adopt the new theme when reopened; they MUST NOT crash or render a mixed
  appearance at the moment of the switch. Immediate live repaint of open
  plugin windows is not required (clarified 2026-07-25).
- **FR-006**: Plugins that do not use the theme mechanism (e.g. older
  third-party plugins) MUST continue to load and function unchanged; the
  feature MUST NOT break binary compatibility of the existing plugin
  interface.
- **FR-007**: All themed plugin surfaces MUST keep text readable in both
  themes — no dark-on-dark or light-on-light combinations anywhere.
- **FR-008**: Plugin configuration dialogs (opened from plugin menus or the
  Plugin Manager) MUST follow the active theme like the application's own
  configuration dialog.
- **FR-009**: Viewer-type plugin windows MUST theme their chrome and
  controls; text/document content areas (markdown, database tables, text
  listings) MUST also render dark — light text on a dark background —
  while images and binary data are displayed faithfully without recoloring
  (clarified 2026-07-25).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: With the Dark theme active, 100% of audited user-visible
  windows/dialogs/pop-ups across all plugins shipped in the default build
  render dark, verified against a per-plugin audit checklist.
- **SC-002**: Zero surfaces with unreadable text (insufficient contrast,
  wrong-on-wrong colors) found in the dark-mode audit of plugin UI.
- **SC-003**: After a theme switch without restart, 100% of newly opened
  plugin windows match the active theme; no crash or mixed-theme surface is
  observed during switch testing.
- **SC-004**: With the Default theme active, a before/after comparison of
  representative plugin surfaces shows zero visual differences.
- **SC-005**: Plugins built against the current plugin interface (without
  theme support) load and operate without modification or errors.

## Assumptions

- "Plugins" means the plugin set shipped in the default distribution
  (currently 18 enabled by the build policy); the theme mechanism itself is
  available to every plugin, and plugins excluded from the default build
  adopt it whenever they are built.
- The dark palette is the one established by feature 028 — this feature
  extends its reach, it does not introduce new colors or a theme editor.
- Feature 028's boundary stands: UI drawn by the operating system or other
  processes (common Open/Save dialogs, shell context menus, shell property
  sheets) is out of scope.
- Third-party plugins without theme support showing light surfaces in dark
  mode is acceptable; forcing them dark without their cooperation is not
  required.
- Already-open plugin windows adopt the new theme on reopen; immediate
  live repaint is explicitly not required (clarified 2026-07-25) —
  consistent with the no-restart rule.
- The existing plugin interface may be extended (new capability), but not
  changed incompatibly — existing compiled plugins keep loading (project
  constitution, plugin ABI preservation).
