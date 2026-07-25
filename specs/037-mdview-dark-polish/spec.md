# Feature Specification: Markdown Viewer Dark-Mode Polish

**Feature Branch**: `037-mdview-dark-polish`
**Created**: 2026-07-25
**Status**: Draft
**Input**: User description: "Nyni uprava pluginu Markdown View. Plugin funguje spravne, ale pri zobrazeni .md souboru pomoci F3 na chvili problikne bile pozadi okna nez se nacte obsah pro zobrazeni, ktery se jiz zobrazi se spravnou barvou pozadi dle zvoleneho barevneho schematu pro zobrazeni Markdown. To neni prijemne, uprav fungovani pluginu tak, aby se hned pro zobrazeni okna okno zobrazilo s barvou, ktera odpovida barve pozadi schematu pro zobrazeni markdown souboru. Dalsi uprava je pak zobrazeni systemoveho menu v Markown souboru - to je zobrazene jako svetle i kdyz je zvoleny tmavy rezim aplikace. Menu musi byt take tmave - v hlavni aplikaci to jde, tak to musi jit i tady."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - No White Flash When Opening a Markdown File (Priority: P1)

A user browsing files in the panel presses F3 on a `.md` file. The Markdown viewer window appears. Today, the window first flashes white for a moment and only then fills with the background color of the selected Markdown color scheme. After this change, the window shows the scheme's background color from the very first moment it becomes visible — the user never sees a white (or any other wrong-colored) surface, regardless of how long the content takes to render.

**Why this priority**: The flash happens on every single viewer open and is most jarring with a dark scheme, where a bright white rectangle appears in an otherwise dark environment. It is the core annoyance the user reported and delivers immediate perceived-quality value on its own.

**Independent Test**: Can be fully tested by selecting a dark Markdown color scheme, opening several `.md` files with F3, and visually confirming (including via screen recording reviewed frame by frame) that no white background is ever visible.

**Acceptance Scenarios**:

1. **Given** a dark Markdown color scheme is selected, **When** the user opens a `.md` file with F3, **Then** the viewer window is filled with the scheme's background color from the first visible frame, with no white flash before content appears.
2. **Given** a light Markdown color scheme is selected, **When** the user opens a `.md` file with F3, **Then** the window shows that scheme's light background immediately and content renders on top of it without any color jump.
3. **Given** a large Markdown file that takes noticeably longer to render, **When** the user opens it, **Then** the window shows the scheme's background color for the entire loading period until the content appears.
4. **Given** the user changes the Markdown color scheme, **When** they next open a `.md` file, **Then** the newly opened viewer shows the new scheme's background color immediately.

---

### User Story 2 - Dark Menus in the Markdown Viewer (Priority: P2)

A user runs the application in dark mode and opens a Markdown file in the viewer. Today, menus displayed by the viewer window (the menu bar's drop-down menus and the window's system menu) appear light, clashing with the dark window. After this change, these menus render dark, matching the way menus already behave in the main application window.

**Why this priority**: Visually inconsistent menus are a polish defect noticeable every time the user interacts with the viewer's menus in dark mode, but they do not flash unprompted the way the white background does — hence P2.

**Independent Test**: Can be fully tested by switching the application to dark mode, opening a `.md` file in the viewer, and opening each menu of the viewer window to confirm dark rendering consistent with the main window's menus.

**Acceptance Scenarios**:

1. **Given** the application is in dark mode, **When** the user opens any drop-down menu from the Markdown viewer's menu bar, **Then** the menu renders with a dark appearance consistent with menus in the main application window.
2. **Given** the application is in dark mode, **When** the user opens the viewer window's caption (system) menu (e.g., via Alt+Space or clicking the window icon), **Then** it looks and behaves identically to the main application window's caption menu (parity — refined during planning, see `research.md` R5).
3. **Given** the application is in light mode, **When** the user opens any menu in the Markdown viewer, **Then** the menu renders light exactly as it does today (no regression).
4. **Given** the user switches the application theme, **When** a Markdown viewer window is opened after the switch, **Then** its menus follow the newly selected theme.

---

### Edge Cases

- What happens when the selected Markdown color scheme differs from the application theme (e.g., light Markdown scheme while the app is in dark mode)? The startup background must match the **Markdown scheme** (what the rendered content will use), while the menus must follow the **application theme** — the two are independent choices.
- What happens when the application theme is switched while a viewer window is already open? The viewer follows the application's established theme-switch convention (already-open windows may adopt the new theme on reopen; windows opened after the switch always use the new theme).
- What happens when several `.md` files are opened in rapid succession, or an already-open viewer loads another file? Every newly shown window and every subsequent load shows the correct scheme background with no flash.
- What happens when the file fails to load or render? The window keeps the scheme's background color; any error presentation appears on top of it without a white flash.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The Markdown viewer window MUST display the background color of the currently selected Markdown color scheme from the first moment the window becomes visible; no white or otherwise mismatched background may be visible at any point before the rendered content appears.
- **FR-002**: The no-flash guarantee MUST hold for every way a viewer window comes to show a Markdown file: opening via F3, opening additional viewer windows, and loading another file into an existing viewer window.
- **FR-003**: When the user changes the Markdown color scheme, viewer windows opened afterwards MUST immediately use the new scheme's background color for their startup fill.
- **FR-004**: When the application is in dark mode, the Markdown viewer's menu bar and all its drop-down menus MUST render with a dark appearance consistent with the menus of the main application window; the caption (system) menu MUST match the main application window's caption menu (parity — it is not restyled beyond what the main window itself shows).
- **FR-005**: When the application is in light mode, the viewer's menus MUST render light, preserving today's appearance (no regression).
- **FR-006**: Theme changes MUST apply to the viewer's menus following the same convention used across the application for theme switching (windows opened after the switch adopt the new theme).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In 10 consecutive openings of a `.md` file with a dark Markdown scheme selected, a frame-by-frame review of a screen recording shows zero frames with a white or mismatched viewer background.
- **SC-002**: With the application in dark mode, 100% of the Markdown viewer's menu-bar drop-downs render dark, visually consistent with the main window's menus, and the caption (system) menu is indistinguishable from the main window's caption menu.
- **SC-003**: After changing the Markdown color scheme, the first viewer window opened afterwards shows the new scheme's background immediately — zero openings display the previous scheme's color or white.
- **SC-004**: In light mode, viewer appearance and menu rendering are unchanged from the current release (zero visual regressions in a side-by-side comparison).

## Assumptions

- "System menu" in the user's description is understood as the system-drawn menu of the Markdown viewer window: the menu bar and its drop-down menus (the menus that visibly clash in dark mode). These must follow dark mode as the main application window's menus already do. The caption (system) menu opened via Alt+Space is held to **parity** with the main window's caption menu — planning research (R5) found the main application does not restyle the native caption menu either, so forcing it dark is out of scope.
- The startup background color is defined by the currently selected **Markdown color scheme** (the same color the fully rendered document will use), which may legitimately differ from the application-wide theme; the menus, by contrast, follow the **application theme**.
- Theme-switch-while-open behavior follows the convention established for the application and its plugins in feature 036: windows opened after a theme switch adopt the new theme; already-open windows adopt it on reopen. No live re-theming of an open viewer is required beyond that convention.
- Scope is limited to the Markdown View plugin's viewer window; no other plugin or the main application is changed by this feature.
- The plugin's existing rendering behavior (content correctness, scheme colors themselves) is correct and out of scope; only the startup fill and menu appearance change.
