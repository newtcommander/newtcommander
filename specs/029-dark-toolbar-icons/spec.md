# Feature Specification: Theme-Adaptive Toolbar Icons (Dark Icon Set)

**Feature Branch**: `029-dark-toolbar-icons`
**Created**: 2026-07-21
**Status**: Draft
**Input**: User description: "Proveď analýzu jak bude možné v rámci barevného
schématu (tmavé jsme přidávali v minulém rozšíření) upravit i sadu ikon
v toolbaru. Ikony totiž stále zůstávají stejné a některé moc dobře nefungují
s tmavým pozadím. Všiml jsem si, že ve výsledném buildu je adresář toolbars
ve kterém jsou svg ikony, ale nevím jak a kde jsou použity. Analyzuj možnosti
nahrazení výchozích ikon sadou ikon, která bude lépe odpovídat tmavému pozadí."

> Companion document: [analysis-toolbar-icons.md](analysis-toolbar-icons.md)
> answers the "how do the icons work today and what are the options" part of
> the request (Czech, technical). This specification captures the resulting
> user-facing feature.

## Clarifications

### Session 2026-07-21

- Q: How far should this feature go in creating the dark icon set —
  automatic adaptation only, automatic plus a per-icon override mechanism,
  or a complete hand-authored dark set? → A: Automatic dark adaptation of
  all icons, plus a per-icon override mechanism: a hand-tuned dark variant
  of any individual icon can be supplied and takes precedence over the
  automatic adaptation; hand-tuned variants can be added incrementally,
  even after this feature ships.
- Q: What should the dark icon variants look like in terms of color —
  keep the icons colorful, render them as monochrome light glyphs, or
  mute/desaturate them? → A: Keep the icons colorful: each icon preserves
  its characteristic hues (colored accents stay colored); only dark and
  neutral strokes/fills are lightened so they read clearly against the
  dark background. No monochrome restyling.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Toolbar icons are legible and coherent in the Dark theme (Priority: P1)

A user who switched the program to the Dark theme (added in the previous
feature) looks at the main window: the top toolbar, the panel directory
lines, the middle toolbar between panels, the bottom bar, and the toolbars
of auxiliary windows (such as Find). Today many button pictures still use
their light-theme colors, so dark outlines and dark fills sink into the dark
background and some buttons are hard to identify. After this feature, every
command button picture is rendered in a dark-adapted variant: clearly
visible against the dark background, consistent in style with the rest of
the dark interface, and still instantly recognizable as the same command.

**Why this priority**: This is the whole point of the feature — the Dark
theme currently looks unfinished because the icon set ignores it. Fixing
icon legibility on dark surfaces delivers the visible value on its own.

**Independent Test**: Switch the program to the Dark theme and visually
audit every button in every toolbar of the main window and the Find window:
each picture must be clearly distinguishable from the background and
recognizable. No code knowledge needed — a screenshot walkthrough suffices.

**Acceptance Scenarios**:

1. **Given** the Dark theme is active, **When** the user inspects every
   button in the main-window toolbars (top toolbar, directory lines, middle
   toolbar, bottom bar), **Then** every button picture is clearly
   distinguishable from the dark background — no picture blends into the
   background or becomes an unreadable dark blob.
2. **Given** the Dark theme is active, **When** the user opens drop-down
   menus that show command icons, **Then** the same dark-adapted pictures
   appear there and are legible against the dark menu background.
3. **Given** the Dark theme is active, **When** the user hovers over or
   presses a toolbar button, **Then** the picture remains legible in the
   highlighted and pressed visuals.
4. **Given** the Dark theme is active, **When** a toolbar button is
   disabled, **Then** its picture is visibly muted compared to enabled
   buttons yet still discernible against the dark background.

---

### User Story 2 - The Default theme keeps today's icons unchanged (Priority: P2)

A user who stays on the Default (light) theme sees exactly the icons they
know today — same artwork, same colors. The dark icon adaptation activates
only with the Dark theme.

**Why this priority**: The Default theme is the compatibility promise of the
theming feature: it must remain identical to the pre-theming program. Any
visible change to light-theme icons would be a regression.

**Independent Test**: Run the program in the Default theme next to the
previous release and compare toolbars — no visible difference.

**Acceptance Scenarios**:

1. **Given** the Default theme is active, **When** the user inspects all
   toolbars and menus, **Then** the icons look identical to the current
   release.

---

### User Story 3 - Icons follow theme switches immediately (Priority: P3)

A user switches between Default and Dark themes from the Options menu. The
toolbar icons change together with the rest of the interface — immediately,
in both directions, with no restart and no leftover mixed state (dark icons
on light background or vice versa).

**Why this priority**: Live switching already works for backgrounds and
chrome; icons lagging behind or requiring a restart would break the
established behavior. Still, it is a polish concern on top of Story 1.

**Independent Test**: Toggle Default ↔ Dark repeatedly (at least 10 times)
and confirm the icons always match the active theme with no visual
artifacts.

**Acceptance Scenarios**:

1. **Given** the program is running in either theme, **When** the user
   switches to the other theme, **Then** all toolbar and menu icons update
   to the matching variant immediately, without restarting the program.
2. **Given** the user toggles themes repeatedly in one session, **When**
   they inspect the toolbars after each toggle, **Then** the icons are
   always consistent with the active theme and show no corruption.

---

### Edge Cases

- A command whose dedicated dark-adapted artwork is missing or unreadable
  must still get a legible dark fallback — no button may remain unreadable
  in the Dark theme because an asset is absent.
- Some buttons today draw from older fallback artwork rather than the
  modern per-command artwork; both kinds must end up legible in the Dark
  theme (Story 1 audit covers every button regardless of its artwork
  source).
- Pictures that depict real system objects (drive icons in the drive bars,
  file/folder icons, shortcut targets) must keep their natural, recognizable
  appearance in both themes — they are not artificially recolored.
- Icons supplied by plugins (plugin bar, plugin menu entries) are outside
  this feature's scope; they must continue to display exactly as they do
  today in both themes.
- On high-DPI displays the dark-adapted icons must stay as crisp as the
  current icons — the adaptation must not introduce blur or scaling
  artifacts.
- Auxiliary windows with toolbars that are open at the moment of a theme
  switch (e.g., Find) must show the matching icon variant after the switch.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: While the Dark theme is active, every command button picture
  in the program's own toolbars (top toolbar, panel directory lines, middle
  toolbar, bottom bar, Find window and other auxiliary-window toolbars)
  MUST render in a dark-adapted variant that is clearly legible against the
  dark background.
- **FR-002**: The dark-adapted variant of each icon MUST preserve the
  icon's identity — same motif and silhouette as the light-theme icon, and
  the icon's characteristic color accents remain colored (no monochrome
  restyling); only dark and neutral strokes/fills are lightened for
  legibility — so users recognize commands without relearning.
- **FR-003**: Everywhere a command icon from the shared set is shown
  (toolbars, drop-down menus, auxiliary windows), the variant matching the
  active theme MUST be used consistently — never a mix of light and dark
  variants at the same time.
- **FR-004**: While the Default theme is active, all icons MUST look
  identical to the current release — no visible change for light-theme
  users.
- **FR-005**: Switching themes MUST update all displayed icons to the
  matching variant immediately, in both directions, without restarting the
  program, and repeated switching within one session MUST NOT degrade the
  icons or leak resources.
- **FR-006**: Disabled buttons in the Dark theme MUST remain visibly
  distinct from enabled buttons while staying discernible against the dark
  background.
- **FR-007**: Every command button MUST receive the dark treatment in the
  Dark theme, including commands that lack dedicated modern artwork and
  render from older fallback artwork — missing assets MUST NOT leave a
  button illegible.
- **FR-008**: Pictures depicting real system objects (drives, files,
  folders, shell items) and icons supplied by plugins MUST NOT be
  artificially recolored; they keep their natural appearance in both
  themes and continue to work unchanged.
- **FR-009**: The icon variant MUST follow the active theme automatically;
  no new user-facing configuration is introduced by this feature.
- **FR-010**: It MUST be possible to supply a hand-tuned dark variant for
  any individual command icon; when such a variant exists, the Dark theme
  uses it in preference to the automatic adaptation. When no hand-tuned
  variant exists for an icon, the automatic adaptation applies, so overall
  coverage never depends on hand-tuned artwork being present.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In the Dark theme, 100% of command buttons across all
  main-window and Find-window toolbars pass a visual audit: each picture is
  identifiable at a glance and does not blend into the background.
- **SC-002**: Each dark-adapted glyph achieves a contrast of at least 3:1
  against the dark toolbar background (measured on the glyph's dominant
  strokes), matching the legibility bar common for UI iconography.
- **SC-003**: In the Default theme, a side-by-side comparison with the
  previous release shows zero visible icon differences.
- **SC-004**: A theme switch completes with all icons updated in under one
  second, and 10 consecutive Default ↔ Dark toggles produce no visual
  artifacts and no growth in resource usage attributable to icon handling.
- **SC-005**: In the Dark theme, users can correctly distinguish disabled
  from enabled buttons for every toolbar command.

## Assumptions

- The dark icon set is derived from the existing icon motifs (adapted
  colors/tones for dark backgrounds), not a wholly new icon design
  language; a visual redesign of the icon family is out of scope. The
  baseline is produced automatically; hand-tuned dark variants are an
  optional per-icon refinement (see FR-010) and no minimum number of them
  is required for this feature to be complete.
- Exactly two themes exist (Default and Dark, from the previous feature);
  the icon variant is keyed solely off the active theme.
- Completing per-command modern artwork for every button (some commands
  today have only older fallback artwork) is not required by this feature;
  the requirement is legibility of every button in the Dark theme, whatever
  its artwork source.
- Plugin-supplied icons and system-object icons are explicitly out of
  scope; adapting plugin icons to themes would be a separate feature.
- The performance cost of preparing the dark icon variants is imperceptible
  at program start and on theme switch (icon preparation already happens at
  those moments today).
- The distribution/installation footprint may include additional icon
  artwork if the chosen approach ships dedicated dark assets; this is
  acceptable as long as Default-theme behavior is unchanged.
