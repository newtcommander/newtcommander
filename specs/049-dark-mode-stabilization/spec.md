# Feature Specification: Dark Mode Stabilization

**Feature Branch**: `049-dark-mode-stabilization`
**Created**: 2026-08-02
**Status**: Draft
**Input**: User description: "Oprava tmavého režimu. Proveď detailní analýzu implementace tmavého režimu a všechny návazné úpravy, které jsme řešili. Stále není tmavý režim zcela bez chyb, v některých pop-up oknech se stále vyskytují černá textová pole na tmavém pozadí, bílé oddělovací čáry, které nepůsobí dobře, při přepínání pohledů okna ALT+3/4/5 se horizontální a vertikální scroll bar obarví zpět na světlý i když je zapnutý tmavý režim, atd. Nejprve alokuj několik nezávislých agentů, každý zaměřený na jiný potenciální aspekt, kde by mohl tmavý režim dělat problémy. Následně konsoliduj jejich zjištění a naplánuj implementaci řady oprav s cílem stabilizace chování tmavého režimu. Jedním z problémů pro opravu je tmavě modrý odkaz v about app dialogu při zobrazení na tmavém pozadí. Další potenciální problémy musí odhalit počáteční velmi velmi hluboká analýza."

**Analysis basis**: A six-track deep audit of the dark-mode implementation was performed before
this specification was written. The consolidated defect inventory (32 defects, clusters A–G,
with severity and evidence) lives in [`analysis/dark-mode-audit.md`](analysis/dark-mode-audit.md);
raw per-track findings are in `analysis/raw-01…06-*.md`. Functional requirements below reference
defect IDs from that inventory for traceability.

## Clarifications

### Session 2026-08-02

- Q: How should dark-mode input fields be re-colored to fix the reported "black holes" (FR-006 / defect C1)? → A: Lighter than face — raise the input-surface color slightly above the dialog face (Windows 11 convention); list backgrounds follow or get their own palette slot; contrast floors re-verified by the test suite.
- Q: Should native Win32 menus (viewer menu bar, 13 core context-menu sites, plugin menus) be included in 049, or deferred? → A: Defer all to a dedicated follow-up feature; 049 stays focused on color/theming correctness.
- Q: How broad should the themed message-box conversion (FR-016) be? → A: Safe sites only — convert the shared-library validation boxes and normal-operation sites; early-startup and failure-path errors (registry problems, allocation failures) keep the plain system message box for robustness.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Panel chrome stays dark while working (Priority: P1)

A user with the Dark theme enabled navigates their files all day: switching panel view modes
(Brief, Detailed, Icons, Thumbnails, Tiles — via Alt+number keys, the menu, the toolbar, or
Alt+mouse-wheel), toggling the column header line, entering archives or plugin file systems that
switch the view automatically, and renaming files in place (F2). At no point does any part of the
panel — scroll bars, bottom bar, header, or the inline rename box — flip back to light colors.

**Why this priority**: This is the most frequently hit reported defect; view switching and inline
rename are everyday actions, and a light scroll bar or white rename box against a dark panel is
the most visible "dark mode is broken" signal. (Defects A1, A2, A6)

**Independent Test**: Enable Dark theme, press Alt+3, Alt+4, Alt+5 (and use the View menu and
Alt+wheel) in both panels, toggle the header line, enter a ZIP archive and an FTP/plugin path,
and rename a file with F2. Every panel element must remain dark throughout.

**Acceptance Scenarios**:

1. **Given** Dark theme is active and a panel shows the Brief view, **When** the user switches to
   Detailed, Icons, Thumbnails or Tiles view by any entry point (Alt+3/4/5…, menu, toolbar,
   Alt+wheel), **Then** the horizontal and vertical scroll bars and the panel bottom bar render
   dark immediately after the switch, with no light flash requiring another action to fix.
2. **Given** Dark theme is active, **When** the user toggles the column header line or enters an
   archive/plugin file system that changes the view mode automatically, **Then** all recreated
   panel elements render dark.
3. **Given** Dark theme is active, **When** the user starts an in-place rename (F2 or slow double
   click), **Then** the rename box uses dark-field styling consistent with other dark input fields.

---

### User Story 2 - Every link is readable on dark surfaces (Priority: P1)

A user opens the About dialog, an error box that carries a support link, the Plugins Manager, the
keyboard-shortcuts configuration page, the regional/language pages, or a plugin About box. Every
hyperlink is clearly readable against its dark background instead of near-invisible dark blue.

**Why this priority**: Explicitly reported (About dialog link) and it affects trust surfaces —
About, error messages, configuration. One shared root cause fixes every link in the product,
including plugin-provided About boxes. (Defects D1, F6)

**Independent Test**: Open the About dialog, a message box containing a URL, Plugins Manager,
Configuration → Keyboard and → Regional, and the Language Selection dialog in Dark theme; verify
each link is readable (meets the project's 4.5:1 contrast floor) and still readable in the
Default (light) theme.

**Acceptance Scenarios**:

1. **Given** Dark theme is active, **When** the About dialog is opened, **Then** the website link
   is readable against the dialog's dark background at no less than the 4.5:1 contrast floor.
2. **Given** Dark theme is active, **When** any window containing a hyperlink is shown (message
   boxes with URLs, configuration pages, plugin manager, plugin About boxes), **Then** the link
   color meets the same contrast floor.
3. **Given** Default (light) theme is active, **When** the same windows are opened, **Then** links
   keep their familiar light-theme appearance (no visual change in light mode).

---

### User Story 3 - Dialog fields and separators look coherent (Priority: P2)

A user opens configuration pages and everyday dialogs (Change Attributes, Find → Advanced
Options, Drive Info, Size results, Language Selection, Master Password, Change Icon, Archivers
auto-configuration, Plugin Keyboard Shortcuts…). Input fields no longer read as black holes
punched into the dialog; read-only and disabled fields are consistent with their editable
siblings; group frames and radio buttons no longer draw bright light-theme chrome; date/time and
shortcut-entry fields are dark; section headers are readable.

**Why this priority**: These are the reported "black text fields" and "white separator lines"
symptoms. They span many dialogs but are individually less frequent than the panel defects.
(Defects C1, C2, C3, C4, B1, B2, B3, E1, E2, A3, A4, A5, D2, D3)

**Independent Test**: Open each affected dialog listed in the audit in Dark theme and visually
verify: uniform input-field surfaces, no bright frames or glyphs, dark date/time and hotkey
fields, readable headers, dark status bar in the Archivers auto-configuration dialog, dark
inline editors in list-based configuration pages.

**Acceptance Scenarios**:

1. **Given** Dark theme is active, **When** any dialog with text input fields is shown, **Then**
   input surfaces use a single consistent dark field color that does not read as a black hole
   against the dialog surface, and read-only/disabled fields are visually consistent with (and
   distinguishable from) editable ones — including multiline ones.
2. **Given** Dark theme is active, **When** a dialog containing group frames or radio buttons is
   shown, **Then** no bright light-theme frames or glyphs appear.
3. **Given** Dark theme is active, **When** the Change Attributes or Find Advanced Options dialog
   is shown, **Then** date and time picker fields render dark like other input fields.
4. **Given** Dark theme is active, **When** dialogs create additional controls while opening or
   while in use (inline list editors, status bars, in-place label edits), **Then** those controls
   are dark from their first appearance.
5. **Given** Dark theme is active, **When** the Change Icon dialog or any section-header caption
   is shown, **Then** list backgrounds and header text follow the dark palette (no white list, no
   black-on-dark text).

---

### User Story 4 - Indicators, markers and transient surfaces follow the theme (Priority: P2)

A user sees progress bars inside dialogs, checkbox columns inside dark lists, tooltips over the
panel splitter and viewer, the drive-information gauge, focus rectangles and drag markers while
customizing toolbars, and the transient "executing" overlay. All of them follow the dark palette
instead of flashing light-theme or invisible elements.

**Why this priority**: Individually small, together they are the remaining "atd." tail the user
pointed at — each one undermines the impression of a finished dark mode. (Defects E3, E4, E5,
C5, C6, C7, D4, D5, B4, B5)

**Independent Test**: In Dark theme: run a checksum/pictview operation with a progress bar, open
configuration lists with checkbox columns, hover the panel splitter and viewer for tooltips, open
Drive Info, drag a button in Customize Toolbar, and launch an external tool (executing overlay);
verify each surface is dark and readable.

**Acceptance Scenarios**:

1. **Given** Dark theme is active, **When** any progress bar is shown in any dialog (including
   plugin dialogs), **Then** it renders with dark-consistent colors.
2. **Given** Dark theme is active, **When** a list with checkbox items is shown, **Then** the
   checkbox glyphs match the dark style used elsewhere.
3. **Given** Dark theme is active, **When** tooltips appear (splitter drag, viewer), **Then** they
   use dark tooltip colors.
4. **Given** Dark theme is active, **When** the transient "executing" overlay or the Drive Info
   gauge is shown, **Then** text and outlines remain readable against their backgrounds.

---

### User Story 5 - Plugin windows are covered without exceptions (Priority: P3)

A user opens the PE Viewer configuration, a plugin's multi-page configuration (FTP, PictView,
File Comparator), the Markdown viewer's Find box, the Disk Map About box, or triggers a numeric
input validation message from a plugin dialog. All these surfaces follow the Dark theme like the
rest of the product.

**Why this priority**: Plugin coverage was declared complete in an earlier feature, but the audit
found concrete remaining holes; they are less frequently visited than core surfaces. (Defects
F1, F2, F3, F4, F5)

**Independent Test**: In Dark theme open: PE Viewer plugin configuration; FTP / PictView /
File Comparator configuration dialogs (frame area around the pages); Markdown viewer Ctrl+F;
Disk Map About; and a numeric-validation error from any plugin dialog — verify each is dark.

**Acceptance Scenarios**:

1. **Given** Dark theme is active, **When** the PE Viewer configuration dialog opens, **Then** it
   is fully dark.
2. **Given** Dark theme is active, **When** a plugin multi-page configuration dialog opens,
   **Then** the frame around the pages (tab strip, buttons, background) is dark, not a light ring
   around dark content.
3. **Given** Dark theme is active, **When** the Markdown viewer's Find box or Disk Map's About box
   opens, **Then** it follows the dark theme.
4. **Given** Dark theme is active, **When** a validation message box fires from a plugin dialog,
   **Then** it uses the product's themed message box appearance.

---

### User Story 6 - Dark mode survives system events (Priority: P3)

A user runs the app in Dark theme while Windows fires environment changes: enabling/disabling
High Contrast, a visual-style refresh, or a system color change. The app keeps its promised
behavior — High Contrast always wins over Dark, and no dark surface silently reverts to light
because of the event's message ordering.

**Why this priority**: Latent robustness defects — hard to hit, but each one produces a
confusing, hard-to-reproduce "dark mode randomly broke" report. (Defects G1, G2, G5, C7, G3, G4)

**Independent Test**: With Dark theme active, toggle Windows High Contrast on and off; change a
system color; trigger a visual-style refresh; verify High Contrast takes precedence immediately,
and after returning, all previously dark surfaces (including open windows and the Configuration →
Views lists) are dark again.

**Acceptance Scenarios**:

1. **Given** Dark theme is active, **When** Windows High Contrast is enabled — regardless of which
   system notification arrives first — **Then** the app immediately honors High Contrast.
2. **Given** Dark theme is active and windows are open, **When** a system visual-style or color
   change occurs, **Then** no open surface permanently reverts to light styling.

---

### Edge Cases

- Theme switched while dialogs/windows are open: main window and Find re-theme live (existing
  behavior); plugin windows adopt on reopen (existing contract, unchanged). No regression allowed.
- High Contrast active: all new dark handling must remain inert (High Contrast wins — existing
  invariant).
- Default (light) theme: every fix must be a no-op — light mode must remain pixel-identical.
- Displays limited to 256 colors: reduced-color fallback branches must not reintroduce unreadable
  pairs (Drive Info gauge legend).
- Third-party plugins built against the current plugin interface must keep loading and working
  unchanged; plugins that never adopted theming stay light by design.
- View switched repeatedly and rapidly (held Alt+wheel): re-applied styling must not flicker,
  leak resources, or degrade switching latency perceptibly.

## Requirements *(mandatory)*

### Functional Requirements

**Panel stability (US1)**

- **FR-001**: The system MUST keep all panel chrome (scroll bars, bottom bar, header) dark after
  every view-mode change, through every entry point that changes the view (keyboard shortcuts,
  menus, toolbar, mouse wheel shortcut, configuration apply, automatic view changes by
  archives/plugin file systems, header-line toggle). (A1, A6)
- **FR-002**: The in-place rename editor in the panel MUST render with dark-field styling
  consistent with other dark input fields. (A2)
- **FR-003**: The theming machinery MUST provide a single reusable way to (re)apply dark styling
  to any window subtree, so that windows created or recreated after a window's initial theming
  pass can be covered by one call, and this mechanism MUST be used by all fixes in this feature
  instead of per-site one-off styling. (A6)

**Readable foregrounds (US2)**

- **FR-004**: All hyperlinks across the product (core and plugin-hosted) MUST meet the project's
  4.5:1 contrast floor against their actual dark background, including the About dialog's branded
  background, while keeping their familiar appearance in the light theme. (D1, F6)
- **FR-005**: The automated contrast checks MUST be extended to cover the link color against every
  background it is drawn on (including the About dialog's branded background). (D1)

**Dialog surfaces (US3)**

- **FR-006**: Input-field surfaces (editable, read-only, and disabled — single-line and
  multiline) MUST use consistent dark styling with the input surface slightly **lighter** than
  the dialog face (Windows 11 convention), so fields no longer appear as black holes and sibling
  fields in one dialog do not differ arbitrarily; disabled/read-only states MUST remain visually
  distinguishable and readable, and all resulting pairs MUST keep the existing contrast floors.
  (C1, C2, C3)
- **FR-007**: Group frames and radio buttons MUST NOT render bright light-theme chrome in the
  Dark theme; their labels MUST stay readable. (B1, B2)
- **FR-008**: Date/time picker and shortcut-entry fields MUST render dark like other input
  fields. (E1, E2)
- **FR-009**: Controls created after a dialog's initial theming pass (inline list editors,
  in-place label edits, late-created status bars) MUST be dark from their first appearance in
  every affected dialog. (A3, A4, A5)
- **FR-010**: Section-header captions and the Change Icon dialog's list MUST follow the dark
  palette (readable text, dark backgrounds, themed selection). (D2, C4)
- **FR-011**: The color-configuration page MUST NOT present white-on-white swatches when no item
  is selected. (D3)

**Indicators and transient surfaces (US4)**

- **FR-012**: Progress bars, list checkbox glyphs, and tooltips MUST follow the dark palette in
  all dialogs and windows where they appear, including plugin dialogs. (E3, E4, E5)
- **FR-013**: Transient overlay windows and the shared window classes they rely on MUST render
  dark backgrounds with readable text in Dark theme. (C5, C6)
- **FR-014**: The Drive Info gauge, focus rectangles, drag markers, and remaining hardcoded
  drawing colors identified in the audit MUST be derived from the theme palette so they stay
  visible on dark surfaces (including the reduced-color fallback). (D4, D5, B3, B4, B5)

**Plugin coverage (US5)**

- **FR-015**: Every enabled plugin's windows and dialogs MUST follow the Dark theme, closing the
  audited holes: PE Viewer configuration, plugin multi-page configuration frames, the Markdown
  viewer Find box, and the Disk Map About box. Any new capability plugins need for this MUST be
  added compatibly so existing third-party plugins keep working unchanged. (F1, F2, F3, F4)
- **FR-016**: Message boxes raised during normal operation (including input-validation messages
  reachable from plugin dialogs) MUST use the product's themed message box instead of the plain
  system one wherever the audit identified bypasses. Early-startup and failure-path error boxes
  (registry problems, allocation failures) deliberately keep the plain system message box for
  robustness (clarified 2026-08-02). (F5)

**Robustness (US6)**

- **FR-017**: High Contrast activation MUST be honored regardless of which system notification
  delivers it; the High-Contrast-wins invariant MUST hold under all notification orderings. (G1)
- **FR-018**: A system visual-style refresh or system color change MUST NOT permanently strip
  dark styling from any open window; affected surfaces MUST be re-covered automatically. (G2,
  G5, C7)
- **FR-019**: Theme resources accessed from non-main threads MUST be safe against the identified
  race, and the viewer-colors refresh MUST follow the documented contract. (G3, G4)

**Regression guarantees (all stories)**

- **FR-020**: In the Default (light) theme and in High Contrast mode, every change in this
  feature MUST be behavior-neutral (no visual or functional difference). Existing dark surfaces
  that already work MUST NOT regress.

### Out of Scope

- **Native Win32 menus** (internal viewer menu bar and its dropdowns, native context menus in
  core and plugins, plugin frame-window menu bars): darkening them requires either OS-undocumented
  mechanisms — rejected repeatedly by this project — or converting each site to the product's
  owner-drawn menu infrastructure, which is a separate feature-sized effort. Deferred to a
  dedicated follow-up feature (confirmed in clarification, 2026-08-02); the audit lists every
  affected site.
- **OS-owned surfaces**: common Open/Save/Font/Color dialogs, shell context menus, folder
  pickers — established boundary since the theme engine was introduced.
- **Auxiliary executables** (installer, crash reporter, self-extractor stubs) — excluded since
  the original theme feature.
- **Splash screen** — brand-dark by design; not a defect.
- **Live re-theming of already-open plugin windows** on theme switch — the reopen-adopts contract
  stays.
- **Automatic following of the Windows light/dark setting** — remains a possible future
  enhancement, unchanged by this feature.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Across all view-mode entry points (at least: Alt+3/4/5 style shortcuts, menu,
  toolbar, mouse-wheel shortcut, header toggle, archive/plugin-triggered switches), 100 % of
  switches leave the panel fully dark — verified by a scripted walkthrough of every entry point
  in both panels.
- **SC-002**: Every hyperlink surface in the product meets the 4.5:1 contrast floor in Dark
  theme, and the automated color-contrast test suite covers the link color against each
  background it is drawn on (suite passes with the new assertions).
- **SC-003**: A full Dark-theme walkthrough of the audited dialog inventory (110 dialog
  templates) finds zero instances of: bright group frames or radio glyphs, black-hole input
  fields, light date/time or shortcut fields, light late-created controls, or unreadable header
  captions — i.e. 100 % of the defects inventoried in clusters A–E are closed.
- **SC-004**: All windows and dialogs of every enabled plugin (18 shipped) render dark in a
  live audit, with zero remaining surfaces from the audit's plugin-gap list (cluster F).
- **SC-005**: Toggling Windows High Contrast on/off and triggering a system color/visual-style
  change leaves zero permanently light surfaces afterward, in a manual robustness pass.
- **SC-006**: In the Default (light) theme, a before/after visual comparison of the walkthrough
  surfaces shows zero differences (light mode untouched).
- **SC-007**: The automated test suite (including the new contrast and palette assertions) passes
  with zero failures; no existing check is weakened or removed.

## Assumptions

- The consolidated audit (`analysis/dark-mode-audit.md`) is the agreed defect inventory for this
  feature; fixes target those 32 defects, and newly discovered items of the same classes during
  implementation are folded in rather than deferred, provided they respect the same scope
  boundaries.
- Resolving the "black hole" input fields adjusts the built-in dark palette's input-surface
  value(s) to sit slightly lighter than the dialog face (clarified 2026-08-02); this is an
  accepted, deliberate visual change of the built-in Dark palette (which is not user-editable),
  subject to the existing contrast floors and test suite. Whether list surfaces share the new
  value or receive their own palette slot is a planning-phase decision.
- Closing the plugin configuration-frame gap may require compatibly extending the plugin
  interface (append-only, with a version bump), consistent with how the plugin theme interface
  was introduced.
- The established invariants remain binding: documented OS mechanisms only; Default theme is a
  strict no-op passthrough; High Contrast wins; theming changes carry no functional side
  effects; the built-in dark palette stays the single source of truth for colors.
- Validation combines the existing automated suite with a manual Dark-theme GUI walkthrough, as
  in previous dark-mode features; the two manual checks left open by feature 044 (duplicate-search
  progress bar, High Contrast toggle) are folded into this feature's validation pass.
