# Feature Specification: Complete Revision of File Name and Path Display Encoding

**Feature Branch**: `010-fix-filename-encoding`
**Created**: 2026-07-17
**Status**: Draft
**Input**: User description loaded from `features/kodovani-znaku.md`: "Proveď detailní hloubkovou analýzu celého programu a identifikuj VŠECHNA místa, kde se zobrazují názvy souborů, resp. cest. V jednom z předchozích úkolů jsme předělávali kódování Unicode a podporu pro dlouhé názvy souborů. Stále se ale v programu nacházejí místa, kde je zobrazení znaků cest, resp. souborů chybné. Jedním z takových míst je např. Directory Line — viz zobrazení: G:\Můj disk\AI, zobrazí se jako: G:\MĹŻj disk. Dej si pozor ať se podíváš na všechna místa výskytu a použití názvů souborů a cest včetně pluginů. Zaměř se i na ostatní výskyty kódování — např. když dám ALT+F5, tak se zobrazí popup okno Pack a packer select, ve kterém jsou položky se špatným kódováním znaků. Prověř vše."

## Problem Statement

Feature 004 gave Open Salamander full Unicode fidelity for file names
and long paths, and feature 005 fixed the dialog text fields that were
left behind (the F2 Rename defect and its audited siblings). Despite
both efforts, users still encounter surfaces where file names, paths,
or other application text are displayed garbled (mojibake). Two
defects are confirmed today:

1. **Directory Line** (the path display above each panel): a folder
   whose real path is **`G:\Můj disk\AI`** is displayed as
   **`G:\MĹŻj disk`**. Each accented character is replaced by a
   two-character garbled sequence, and the displayed path is visibly
   not the real path — in the reported example the trailing `\AI`
   component is missing as well, suggesting the display's
   length/shortening logic is also confused by the garbled text. The
   garbling pattern shows the correctly stored name being
   misinterpreted on its way to the screen, the same defect class
   fixed for dialogs in feature 005.

2. **Pack dialog (Alt+F5)**: the packer selection list contains
   entries with garbled characters. These entries are not file names
   but application-provided display strings, which shows the defect
   class extends beyond file-name surfaces to other text the
   application stores and displays.

Because two defective surfaces surfaced *after* feature 005's audit,
this feature mandates a deeper, exhaustive, documented audit of
**every place in the application where a file name, a path, or an
application-provided display string is rendered** — main-window chrome
(Directory Line, window title, status/information line), all dialogs
and prompts, menus, tooltips, history lists, the Find window, viewers,
and the bundled plugins — followed by fixes to every surface found
defective, so no garbled text remains anywhere a user can see it.

## Clarifications

### Session 2026-07-17

- Q: How should the application treat display strings (e.g. packer
  entries) stored by an older version under the legacy text encoding?
  → A: Reset affected entries to the current default values during
  configuration load; do not attempt conversion. Users re-create any
  custom entries.
- Q: Which plugins must the display audit cover? → A: Only the plugins
  enabled in `plugins.cfg` (currently 18) — the shipped set. Disabled
  plugins are audited only if and when they are re-enabled.
- Q: How is each surface in the audit inventory verified? → A: By a
  manual walkthrough with the standard sample-name set, with the
  verdict recorded per surface in the inventory (same protocol as
  feature 005).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Main-window chrome renders Unicode paths correctly (Priority: P1)

A user browses into a directory whose path contains accented or
non-Latin characters (for example `G:\Můj disk\AI`). The Directory
Line above the panel, the main window title, and the
status/information line all show exactly the real path and file names,
identical to how the panel list renders them.

**Why this priority**: This is the reported, reproducible defect. The
Directory Line is permanently visible and is the user's primary
orientation point; a garbled path there undermines trust in the whole
application and makes it impossible to verify the current location.

**Independent Test**: Create (or mount) a directory with Czech
diacritics in its path, navigate into it, and compare the Directory
Line, window title, and status line against the panel rendering.

**Acceptance Scenarios**:

1. **Given** a panel showing the directory `G:\Můj disk\AI` (or a
   local equivalent containing diacritics), **When** the user
   navigates into it, **Then** the Directory Line shows exactly
   `G:\Můj disk\AI` with no garbled characters and no missing path
   components.
2. **Given** the same directory, **When** the main window title and
   the status/information line reference the current path or the
   focused file, **Then** the text is rendered correctly.
3. **Given** a window too narrow to show the full path, **When** the
   Directory Line shortens the path with an ellipsis, **Then** the
   visible portions remain correct — no character is garbled, split
   in half, or dropped beyond the intended ellipsis.
4. **Given** a directory whose name mixes scripts (Latin + Cyrillic +
   CJK + emoji), **When** it is the current path, **Then** all chrome
   surfaces render it correctly.

---

### User Story 2 - Packer lists and other stored display strings render correctly (Priority: P2)

A user presses Alt+F5 to pack files. The Pack dialog's packer
selection list shows every packer name correctly spelled and readable.
The same holds for every other place that lists packers or displays
application-stored text (the Unpack dialog, the Pack/Unpack pages of
the configuration, custom packer definitions), and the text stays
correct across configuration save, application restart, and reload.

**Why this priority**: This is the second reported defect. It proves
the problem is not limited to file names — text the application itself
stores and displays is affected too. A garbled packer list makes the
pack feature effectively unusable because the user cannot tell the
entries apart.

**Independent Test**: Open Alt+F5 on a fresh default configuration and
read every entry of the packer list; restart the application and
verify the list is unchanged and correct.

**Acceptance Scenarios**:

1. **Given** a default configuration, **When** the user opens the Pack
   dialog (Alt+F5), **Then** every entry in the packer selection list
   is correctly spelled with no garbled character sequences.
2. **Given** the Unpack dialog and the Pack/Unpack configuration
   pages, **When** they list packers or user-defined entries, **Then**
   all entries render correctly and match the Pack dialog.
3. **Given** a configuration that has been saved, **When** the
   application is restarted and the Pack dialog reopened, **Then** the
   entries are unchanged and still correct (full round trip).
4. **Given** a user-defined packer entry whose name contains accented
   characters, **When** it is created, saved, and redisplayed,
   **Then** the name is preserved exactly.

---

### User Story 3 - Exhaustive audit closes every remaining surface, including plugins (Priority: P3)

A user working with Unicode-named files sees correct text in *every*
remaining corner of the application: Find window results and status,
viewer window titles, archive browsing (paths inside archives),
plugin file systems (e.g. FTP/SFTP remote paths), plugin dialogs,
menus, tooltips, drive menus with volume labels, path history lists,
and confirmation/error prompts. The audit that guarantees this is
documented: every surface identified, classified, and — where
defective — fixed.

**Why this priority**: "Prověř vše" — the explicit mandate of this
feature. The two reported defects escaped the previous audit, so the
value of this story is systematic completeness rather than any single
fix. It is last only because the P1/P2 stories fix the known,
user-visible pain first.

**Independent Test**: Walk the documented audit inventory with the
sample-name set (composed, decomposed, multi-script, emoji) and verify
each surface renders the names correctly.

**Acceptance Scenarios**:

1. **Given** the completed audit inventory, **When** each listed
   surface is exercised with the sample-name set, **Then** no surface
   shows garbled, substituted, or missing characters.
2. **Given** an archive containing entries with Unicode names,
   **When** the user browses it and views the current path, **Then**
   the archive path and entry names render correctly in all surfaces.
3. **Given** a plugin file system displaying a remote path with
   Unicode characters, **When** the path appears in the Directory
   Line, dialogs, or plugin UI, **Then** it renders correctly.
4. **Given** a drive or volume whose label contains diacritics,
   **When** drive menus or drive information surfaces show the label,
   **Then** it renders correctly.

---

### Edge Cases

- Directory Line shortening: when the path must be abbreviated, the
  shortening must operate on characters as the user perceives them —
  it must never cut a character in half (producing garbage) or drop
  trailing path components, as observed in the reported example.
- The same text shown in multiple forms — e.g. the Directory Line, its
  tooltip, and the path history drop-down — must all be correct and
  identical.
- Copying the displayed path (e.g. Copy Full Name / clipboard actions)
  must place the *true* path on the clipboard, never a garbled
  rendering.
- Configurations carried over from an older version of the application
  (stored under the previous text encoding conventions): affected
  entries are reset to the current defaults on load (per the
  2026-07-17 clarification), so the user never sees garbled entries —
  at the cost of re-creating any custom definitions.
- Names combining composed and decomposed accent forms, multiple
  scripts, and emoji must render correctly in every audited surface,
  consistent with the feature 005 sample set.
- Very long Unicode paths (feature 004 territory) must behave the same
  as short ASCII paths in every audited surface, including shortened
  display variants.
- Plugin-provided text (packer names, file-system paths, dialog
  content) must render correctly regardless of which bundled plugin
  supplies it.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Every application surface that displays a file name,
  directory name, or path MUST render it identically to how the file
  panels render it, for any name or path the operating system can
  store — including main-window chrome (Directory Line, window title,
  status/information line), dialogs, prompts, menus, tooltips,
  history lists, the Find window, viewer titles, and drive menus.
- **FR-002**: The Directory Line MUST display the current path exactly
  as stored; its shortening/ellipsis behavior MUST never garble a
  character, split one apart, or silently drop path components.
- **FR-003**: The Pack dialog (Alt+F5) packer selection list, the
  Unpack dialog, the Pack/Unpack configuration pages, and every other
  surface that lists packer or archiver names MUST display those names
  correctly.
- **FR-004**: Application-stored display strings (packer definitions
  and similar configuration-derived text) MUST survive the full
  save → restart → reload round trip unchanged and MUST render
  correctly after each step. Entries found in a legacy-encoded form
  (stored by an older application version) MUST be replaced with the
  current default values during configuration load — no conversion is
  attempted, and no garbled entry may remain visible after upgrade.
- **FR-005**: The team MUST perform and document a systematic audit of
  all surfaces in the core application and the bundled enabled plugins
  where file names, paths, or application-stored display strings are
  rendered; classify each surface as correct or defective; and fix
  every defective surface within this feature. Each surface's verdict
  MUST be established by a manual walkthrough with the sample-name set
  and recorded in the audit inventory.
- **FR-006**: No display surface may silently normalize, substitute,
  or drop characters; the rendered text must always correspond to the
  true stored name or string.
- **FR-007**: Actions derived from displayed text (copying a path to
  the clipboard, navigating via a history entry, applying a packer
  selection) MUST operate on the true stored value, not on a garbled
  rendering.
- **FR-008**: Existing behavior for names and strings consisting only
  of unaccented Latin characters MUST remain unchanged — zero
  regressions in browsing, packing, and file-operation flows.

### Key Entities

- **File/Directory Path**: A Unicode string exactly as stored by the
  file system (or reported by an archive or plugin file system); the
  authoritative value every display surface must reproduce.
- **Application-stored display string**: Text the application persists
  and later displays — packer/archiver names, user-defined packer
  entries, and similar configuration-derived strings. Distinct from
  file names but subject to the same rendering-fidelity rules.
- **Display surface**: Any UI element that renders a path, name, or
  stored string as text — permanent chrome (Directory Line, title,
  status line), dialogs, menus, tooltips, lists, and plugin UI. Each
  surface is either *correct* or *defective* with respect to encoding.
- **Audit inventory**: The documented list of all display surfaces
  produced by FR-005, with a verdict (correct/defective) and a
  resolution for each defective one.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: With a test path containing Czech diacritics (equivalent
  to `G:\Můj disk\AI`), the Directory Line, window title, and status
  line render the exact path in 100% of attempts, including
  narrow-window shortened variants.
- **SC-002**: 100% of the entries in the Alt+F5 packer list (and the
  Unpack and configuration equivalents) are readable and correctly
  spelled, on a fresh configuration and after a save/restart/reload
  round trip.
- **SC-003**: 100% of the surfaces in the audit inventory render the
  full sample-name set (composed, decomposed, multi-script, emoji)
  without any garbled, substituted, or missing characters.
- **SC-004**: The audit inventory demonstrably covers the core
  application and every bundled enabled plugin, and records for each
  surface a verdict from the documented manual walkthrough and, where
  defective, the resolution.
- **SC-005**: All browsing, packing, and file-operation flows
  exercised with plain ASCII names behave exactly as before the change
  (zero regressions in the existing verification flows).

## Assumptions

- This feature builds directly on feature 004 (Unicode + long-path
  storage and operations) and feature 005 (Unicode in dialog text
  fields); the guarantees delivered there remain in force, and the
  remaining defects are presumed to be residual display-time
  conversion points that those features did not reach. If the audit
  reveals a deeper fidelity defect, fixing it is in scope, because
  FR-006/FR-007 are end-to-end requirements.
- The panels render names correctly today (per feature 004), so the
  panel rendering serves as the reference for FR-001.
- The reported `G:\Můj disk` path is a cloud-drive mount; the defect
  is reproducible with any local directory containing diacritics, so
  verification does not require the cloud drive.
- Scope covers the core application plus the plugins enabled by the
  repository's build policy (`plugins.cfg`, currently 18 enabled);
  plugins disabled by policy are out of scope until re-enabled.
- The sample-name set from feature 005 (Czech diacritics in composed
  and decomposed forms, Cyrillic/Greek/CJK scripts, emoji) is reused
  as the verification data set.
- Rendering of file *content* (viewer text-encoding selection) remains
  out of scope, as decided in feature 004; in scope are file names,
  paths, and application-stored display strings.
- The audit and fixes target the current development baseline (branch
  `010-fix-filename-encoding` created from `main` after feature 009).
