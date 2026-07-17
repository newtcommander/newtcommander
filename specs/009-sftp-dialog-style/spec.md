# Feature Specification: Consistent SFTP Plugin Dialog Appearance

**Feature Branch**: `009-sftp-dialog-style`
**Created**: 2026-07-17
**Status**: Draft
**Input**: User description: "Pri sestaveni cisteho buildu jsou okna v pluginu sftp stale zobrazena nekonzistentne. Na zaklade predchoziho kontexu a popsaneho chovani zajisti konzistentni vzhled pro plugin sftp a tento vzhled pak udrzuj i pro dalsi rozsireni v budoucnu."

## Clarifications

### Session 2026-07-17

- Q: Which concrete visual difference must be eliminated for SFTP windows to count as consistent on a clean build? → A: The focused text-input field decoration (the modern accent underline / frame shown when a field has keyboard focus) that still differs from the rest of the application. This is the confirmed primary difference; the field's selected-state appearance must match a focused field elsewhere in the application.
- Q: How should the "keep it consistent for future extensions" requirement be enforced — documented convention, a lightweight automated check, or full visual-regression testing? → A: No automated check of any kind. The house-style convention is stored durably in the project's governing documentation (the project constitution) and upheld through code review only.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - SFTP dialogs look like the rest of the application (Priority: P1)

A person using Open Salamander opens the SFTP plugin's windows (for example the connect
dialog or the password prompt). Every SFTP window looks like it belongs to the same
program as the rest of Open Salamander: the same text style, the same input-field
appearance, and the same way a field looks when it is selected. Nothing about the SFTP
windows stands out as visually "foreign" compared with the other file-manager windows
(such as the FTP connect dialog or the core rename/find dialogs).

**Why this priority**: This is the whole point of the feature and the concrete complaint —
on a fresh build the SFTP windows still look different from the rest of the application.
Delivering this one story alone already resolves the reported problem and gives the user a
coherent, trustworthy product.

**Independent Test**: On a freshly built application, open each SFTP window and place it
next to an equivalent core/FTP window. Confirm the text style, the input-field frame, and
the selected-field highlight are the same in both.

**Acceptance Scenarios**:

1. **Given** a clean build of the application, **When** the user opens the SFTP connect
   window, **Then** its text and input fields are rendered in the same visual style as the
   application's other windows.
2. **Given** an SFTP input field, **When** the user moves keyboard focus into it, **Then**
   the field's selected-state highlight looks the same as a selected input field elsewhere
   in the application (a frame around the whole field, not a divergent accent marking).
3. **Given** any SFTP window that contains text (connect, password/passphrase prompt,
   host-key verification, permissions, symbolic link, rename, configuration page, logs),
   **When** it is shown, **Then** it uses the same typeface and sizing as the rest of the
   application.

---

### User Story 2 - Consistency is deterministic within a session (Priority: P2)

The consistent appearance does not depend on what the user did earlier in the same session
or on the order in which windows are opened. Opening the SFTP plugin does not change how
any other window in the application looks, and other parts of the application do not change
how the SFTP windows look.

**Why this priority**: The reported inconsistency is the kind of defect that can appear or
disappear depending on session state and load order, which makes it confusing and hard to
trust. Guaranteeing order-independence turns "sometimes looks right" into "always looks
right".

**Independent Test**: In a single session, open SFTP windows and other application windows
in several different orders (SFTP first, SFTP last, interleaved). Confirm all windows keep
the same consistent appearance in every order, with no window changing style after another
is opened.

**Acceptance Scenarios**:

1. **Given** a session where the SFTP plugin has already been used, **When** the user then
   opens a non-SFTP window (e.g. FTP connect or a core dialog), **Then** that window looks
   exactly as it does when the SFTP plugin has not been used.
2. **Given** a session where other plugins/dialogs were used first, **When** the user then
   opens an SFTP window, **Then** the SFTP window still matches the application's house
   style.

---

### User Story 3 - New SFTP windows stay consistent by default (Priority: P3)

When the SFTP plugin is extended in the future with new windows or new input controls,
those additions inherit the same house style automatically, so a maintainer does not have
to hand-tune each new window and consistency does not silently drift over time.

**Why this priority**: The user explicitly asked to *keep* the appearance consistent for
future extensions. Without a durable convention, the same inconsistency will reappear the
next time someone adds a dialog. This protects the P1 outcome going forward but is not
required to fix today's visible problem.

**Independent Test**: Add a new trivial SFTP window that follows the documented house-style
convention and build. Confirm it matches the rest of the application without any
window-specific styling work, and that a window which violates the convention is caught by
the project's review/consistency gate.

**Acceptance Scenarios**:

1. **Given** the documented house-style convention, **When** a new SFTP window is created
   following it, **Then** the window matches the application's appearance with no
   per-window styling adjustments.
2. **Given** a proposed change that would make an SFTP window deviate from the house style,
   **When** it is reviewed against the project's rules, **Then** the deviation is flagged
   before it is accepted.

### Edge Cases

- **High-DPI / display scaling**: On monitors with display scaling, SFTP windows scale the
  same way as the rest of the application, with no SFTP-specific blurriness or sizing
  difference beyond what the whole application already exhibits.
- **Windows theme variations** (standard, high-contrast): SFTP windows follow the same
  theming behavior as the application's other windows; they do not adopt a different theme
  treatment than the core.
- **Load order and repeated opening**: Opening and closing the same window multiple times,
  and opening SFTP windows before/after other plugins, never produces a different
  appearance than the first time.
- **Clean build vs incremental build**: The appearance is the same whether the application
  was produced by a full clean build or an incremental one; it does not depend on leftover
  build artifacts.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Every SFTP plugin window MUST present the same overall visual style
  (typeface, text sizing, control framing) as the application's other windows.
- **FR-002**: Text input fields in SFTP windows MUST show the same selected/focused-state
  decoration as input fields elsewhere in the application, so a focused SFTP field is
  visually indistinguishable in framing from a focused field in a core or FTP window. This
  focused-field decoration is the confirmed primary difference still visible on a clean
  build and MUST be eliminated (no divergent accent underline or modern frame on focus).
- **FR-003**: The consistent appearance MUST be present in a freshly produced clean build
  with no manual post-build steps required to achieve it.
- **FR-004**: SFTP window appearance MUST be deterministic within a session — it MUST NOT
  depend on the order in which the SFTP plugin or other plugins are used, and using the
  SFTP plugin MUST NOT alter the appearance of any other window in the application.
- **FR-005**: The consistent styling MUST apply uniformly to all SFTP windows, including at
  minimum: connect, password/passphrase prompt, host-key verification, permissions change,
  symbolic-link creation, rename, the configuration page, and the logs window.
- **FR-006**: New or modified SFTP windows and controls MUST inherit the house style by
  default, without requiring per-window visual overrides, so future extensions remain
  consistent.
- **FR-007**: The project MUST record the application's house-style convention for windows
  and controls durably in its governing documentation (the project constitution), so it is
  upheld during code review and maintained over time. No automated consistency check or
  visual-regression tooling is required or in scope.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On a clean build, 100% of SFTP windows render with the same typeface and
  control framing as the application's other windows, verified by direct side-by-side
  comparison.
- **SC-002**: A focused text field in any SFTP window is judged visually identical in its
  frame/focus treatment to a focused text field in a core or FTP window (no divergent
  focus decoration).
- **SC-003**: Across at least three different window-open orderings in one session, there
  are 0 observable appearance differences in SFTP windows and 0 appearance changes in
  non-SFTP windows caused by using SFTP.
- **SC-004**: A newly added SFTP window that follows the documented house style requires 0
  window-specific styling adjustments to match the rest of the application.
- **SC-005**: The house-style convention is documented in the project constitution, and a
  window change that violates it is caught during code review against that documented
  convention (no automated check involved).
- **SC-006**: After the change, there are 0 outstanding reports or review findings that
  "SFTP windows look different from the rest of the application" on a clean build.

## Assumptions

- The reference "house style" is the appearance already shared by the core application and
  the other bundled plugins (for example the FTP plugin) as currently shipped — i.e. the
  classic, uniformly framed look — not a new or modernized visual design.
- The target environment is Windows 11 (and Windows 10) in line with the project's platform
  commitment; the feature does not introduce a separate look for different OS versions.
- Scope is limited to the visual consistency of the SFTP plugin's windows and controls. It
  does not include a broader visual redesign of the application, introducing dark mode, or
  modernizing display-scaling behavior; those remain as they are for the whole application.
- The comparison baseline is a clean build produced by the project's standard build process,
  with no manual tweaks applied afterward.
- Maintaining future consistency relies solely on the documented convention in the project
  constitution (the UI-consistency principle) upheld through code review. No automated
  consistency check or visual-regression testing is in scope.
- "Consistent" is assessed by human side-by-side visual comparison of the relevant windows;
  no exact pixel-difference threshold is defined for this feature.
