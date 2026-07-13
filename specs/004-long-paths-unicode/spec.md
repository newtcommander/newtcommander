# Feature Specification: Long Path and Unicode File Name Support

**Feature Branch**: `004-long-paths-unicode`
**Created**: 2026-07-13
**Status**: Draft
**Input**: User description: "Detailne analyzuj zdrojovy kod a priprav implementaci upravy, ktera opravi dve dlouhodbe chyby celeho programu. Jednak se jedna o podporu dlouhych souboru, resp. dlouhych nazvu souboru a absolutnich cest a jednak se jedna o spravne pracovani s Unicode. V principu jde o to, ze Unicode umoznuje zapsat symboly s diakritikou pomoci vice zpusobu - program spatne pracuje se soubory, resp. s nazvy souboru, ktere maji Unicode znaky s diakritikou vytvorene pomoci sekvence dvou znaku - napr. hacek a pak c, coz vytvari č, ale Salamander s tím neumí pracovat."

## Problem Statement

Open Salamander suffers from two long-standing, related defects that
prevent users from managing files that every modern Windows system can
otherwise create and store:

1. **Long paths** — The application cannot browse, display, or operate
   on files and folders whose absolute path exceeds the legacy Windows
   limit of about 260 characters, even though the operating system
   itself supports paths up to roughly 32,000 characters. Users hit
   this daily with deep project trees, synchronized cloud folders, and
   backups produced by other tools.

2. **Unicode file names** — The application internally represents file
   names in a legacy single-byte character encoding. Any file name
   containing characters outside the current system code page (for
   example Greek, Cyrillic on a Western system, Asian scripts, emoji)
   or containing *decomposed* accented characters — where a character
   such as "č" is stored as the base letter "c" followed by a separate
   combining accent mark — is displayed incorrectly and cannot be
   reliably opened, copied, renamed, or deleted. Decomposed names are
   routinely produced by files copied from macOS, some archive tools,
   and cloud-synchronization services.

Both defects share the same root cause: the application's file-name
handling predates modern Windows conventions. Fixing them requires the
application to treat file names and paths exactly as the operating
system stores them — at full length and in full Unicode fidelity.

## Clarifications

### Session 2026-07-13

- Q: How far should long-path and Unicode support extend for plugins
  in this feature? → A: The whole program including plugins — the
  plugin interface is extended and all 35 bundled plugins are migrated
  within this feature; only third-party plugins built against the
  legacy interface fall back to the protective detect-and-refuse
  behavior.
- Q: What performance target should the spec set for large
  directories (e.g. 100,000 items) after the file-name handling
  migration? → A: No perceptible change — listing, sorting, and
  scrolling must not be measurably worse than before the change
  (±10% tolerance).
- Q: Does this feature also cover rendering of file *content* (text
  encoding in the internal viewer), or only file names and paths? →
  A: Names and paths only — the viewer must open any long-path or
  Unicode-named file, but content-encoding improvements are a
  separate follow-up feature.
- Q: When a copy would place a file whose name is canonically
  equivalent (but differently composed) next to an existing file in
  the destination, how should the program behave? → A: Follow the
  operating system (both files coexist, no overwrite prompt), but
  show a one-time notice when an operation creates a visually
  indistinguishable pair in one directory.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Manage Files in Deep Directory Trees (Priority: P1)

A user opens a directory whose absolute path exceeds the legacy
260-character limit (for example, a node_modules tree, a deep backup
hierarchy, or a cloud-synchronized folder). The panel lists the
contents correctly, and the user can perform every ordinary file
management task — view, edit, copy, move, rename, delete, create
subdirectories, and change attributes — exactly as in any shallow
directory.

**Why this priority**: This is a hard functional block: today the user
cannot reach these files at all with Salamander and must fall back to
other tools. It affects the core purpose of a file manager.

**Independent Test**: Create a directory structure whose total path
length exceeds 300 characters, place files in the deepest folder, and
verify that browsing plus each basic file operation completes
successfully from the panel.

**Acceptance Scenarios**:

1. **Given** a folder whose absolute path is longer than 260
   characters, **When** the user navigates into it, **Then** the panel
   shows the complete and correct directory listing.
2. **Given** a file at a path longer than 260 characters, **When** the
   user copies or moves it to another long or short path, **Then** the
   operation completes and the content is intact.
3. **Given** a file at a path longer than 260 characters, **When** the
   user renames, deletes, or edits its attributes, **Then** the
   operation completes without error.
4. **Given** a panel positioned in a deep directory, **When** the user
   creates a new subdirectory or file that pushes the total path
   further beyond the legacy limit, **Then** creation succeeds.
5. **Given** a copy operation whose destination path would exceed the
   maximum the target storage supports, **When** the operation runs,
   **Then** the user receives a clear error identifying the affected
   item, and no partially corrupted result is left behind.

---

### User Story 2 - Manage Files with Any Unicode Name (Priority: P1)

A user has files whose names contain accented characters stored in
decomposed form (base letter + combining accent, e.g. "c" + caron
producing "č"), or characters outside the current system code page
(e.g. Greek, Japanese, emoji). The panel displays every name exactly
as it appears in other Windows applications, and all file operations
work on these files.

**Why this priority**: Equally severe functional block: affected files
are today displayed with wrong or substitute characters and operations
on them fail or target the wrong file. Data received from macOS users
or cloud services is a common, recurring source of such names.

**Independent Test**: Create files whose names use (a) decomposed
accents, (b) precomposed accents, (c) characters outside the system
code page; verify correct display and successful completion of each
basic file operation on each file.

**Acceptance Scenarios**:

1. **Given** a file whose name contains a decomposed accented
   character, **When** the panel lists it, **Then** the name renders
   identically to how the operating system's own file explorer renders
   it (no substitute characters, no visible accent separation).
2. **Given** a file whose name contains characters outside the current
   system code page, **When** the user opens, copies, renames, or
   deletes it, **Then** the operation succeeds and targets exactly that
   file.
3. **Given** a file with a decomposed name is copied or moved, **When**
   the operation completes, **Then** the resulting name is preserved
   exactly as stored — the application does not silently convert it to
   a different Unicode form.
4. **Given** a directory containing two distinct files whose names
   differ only in Unicode composition form (one precomposed, one
   decomposed), **When** the panel lists the directory, **Then** both
   files appear and each can be selected and operated on individually.
5. **Given** a file with a name containing characters beyond the basic
   Unicode range (e.g. emoji), **When** any file operation is
   performed, **Then** it completes correctly.

---

### User Story 3 - Find and Navigate to Files Regardless of Name Form or Depth (Priority: P2)

A user searches for a file by typing its name — via quick search in
the panel, the Find dialog, or a path entry field. The user types the
name the natural way (producing precomposed characters on a Windows
keyboard), and the application finds the file even if its stored name
uses the decomposed form, and even if it resides deeper than the
legacy path limit.

**Why this priority**: Search is the primary recovery tool when a user
cannot locate a file; without form-insensitive matching, files with
decomposed names are effectively invisible to search even after
display is fixed. Depends on the foundations built by stories 1 and 2.

**Independent Test**: Place files with decomposed names in a deep
directory tree; search for them by typing the precomposed form; verify
they are found and can be acted upon from the results.

**Acceptance Scenarios**:

1. **Given** a file stored with a decomposed name, **When** the user
   types the equivalent precomposed name in quick search, **Then** the
   selection lands on that file.
2. **Given** files deeper than the legacy path limit, **When** the user
   runs Find over the parent tree, **Then** matching files at any depth
   appear in results, and operations invoked from results succeed.
3. **Given** a search pattern containing accented characters, **When**
   names stored in either composition form match the pattern's meaning,
   **Then** all such files are returned.

---

### User Story 4 - Full Capability Across Plugins and Integration Surfaces (Priority: P2)

A user works with long-path or Unicode-named files everywhere the
program reaches: bundled archive, viewer, filesystem and utility
plugins, the system clipboard, drag & drop with other applications,
context menus, and launching external programs. Bundled plugins handle
these names and paths at full parity with the core panels — archives
containing decomposed-name entries unpack correctly, plugin viewers
open such files, network plugins transfer them. Only where a component
outside the delivered program (a third-party plugin built against the
legacy interface, or an external application) cannot represent a name,
the application clearly says so instead of failing silently or
corrupting names.

**Why this priority**: The fix must cover the whole program
(stakeholder decision, see Clarifications) — a file manager whose
panels handle a name but whose bundled archiver mangles it would still
lose user data. Builds on the core capabilities from stories 1–3.

**Independent Test**: Exercise every bundled plugin category
(archiver, viewer, filesystem/network, utility) plus clipboard and
drag & drop against long-path and decomposed-name files; verify
correct completion — and for legacy third-party plugins, an explicit,
actionable message.

**Acceptance Scenarios**:

1. **Given** files with long paths or Unicode names, **When** the user
   copies them to the clipboard and pastes into the system file
   explorer (and vice versa), **Then** the transfer completes with
   names preserved.
2. **Given** an archive containing entries with decomposed Unicode
   names, **When** the user browses it and extracts to a destination
   deeper than the legacy path limit, **Then** all entries extract
   with names preserved exactly.
3. **Given** a bundled plugin of any category (viewer, filesystem,
   network, utility), **When** the user invokes it on a long-path or
   Unicode-named file, **Then** the operation completes at parity with
   the equivalent core operation.
4. **Given** a third-party plugin built against the legacy interface
   that cannot represent a particular name or path length, **When**
   the user directs such an item to it, **Then** the application
   refuses that item with a clear message naming it — it never passes
   through a silently altered name.
5. **Given** a file with a Unicode name, **When** the user opens the
   system context menu on it or launches an associated application,
   **Then** the correct file is targeted.

---

### Edge Cases

- A single name component at the filesystem's per-name limit (255
  characters) inside an already-deep path.
- Two files in one directory whose names are canonically equivalent
  but stored in different composition forms — both must remain
  individually listable, selectable, and operable (see US2 scenario
  4); an operation that first creates such a pair triggers the
  one-time notice defined in FR-007.
- Combined stress: decomposed Unicode name at a path deeper than the
  legacy limit.
- Network locations (UNC paths) with long paths and Unicode names.
- Names containing characters beyond the basic Unicode plane
  (surrogate pairs, e.g. emoji) in sorting, display, and column width
  calculations.
- Sorting and case-insensitive grouping of names that differ only in
  accents or composition form — order must be deterministic and
  visually sensible.
- Storage destinations that do not support long paths — the operation
  must fail per-item with a clear message, not corrupt or truncate.
- Persisted references (directory history, favorite/hot paths, saved
  configuration, session restore) pointing to long or Unicode paths
  must survive an application restart intact.
- External tools launched from the application with an affected file
  as parameter may themselves lack support — the application must pass
  the exact name and leave the limitation to the external tool.
- Existing user configurations created before this change must load
  unchanged (no migration required for ordinary paths).

## Requirements *(mandatory)*

### Functional Requirements

**Long paths**

- **FR-001**: The application MUST list, display, and navigate
  directories whose absolute path length is up to the maximum the
  operating system supports (approximately 32,000 characters).
- **FR-002**: All built-in file operations — open, view, edit, copy,
  move, rename, delete, create file/directory, change attributes,
  calculate sizes — MUST succeed on items whose absolute path exceeds
  the legacy 260-character limit, on local and network storage alike.
- **FR-003**: Bulk operations (copying or moving whole trees) MUST
  handle mixtures of short and long paths within one operation,
  reporting per-item errors without aborting unrelated items, and
  leaving no partially written orphan on failure.
- **FR-004**: When a destination cannot accommodate a resulting path,
  the application MUST report which item failed and why, and MUST NOT
  silently truncate or alter the name to force a fit.

**Unicode file names**

- **FR-005**: The application MUST display every file name exactly as
  stored by the operating system — including decomposed accent
  sequences, characters outside the current system code page, and
  characters beyond the basic Unicode plane — with no substitute
  characters, in panels, dialogs, title bars, progress windows, and
  error messages.
- **FR-006**: All built-in file operations MUST work on names in any
  Unicode composition form and MUST preserve the stored form
  byte-for-byte when the user did not intentionally change the name.
- **FR-007**: Two distinct directory entries whose names are
  canonically equivalent but differently composed MUST both be shown
  and MUST be individually addressable by every operation. Overwrite
  detection follows the operating system: such entries are different
  files and coexist without an overwrite prompt. When an operation
  first creates such a visually indistinguishable pair in one
  directory, the application MUST show a one-time informational
  notice identifying both entries.
- **FR-008**: Name matching driven by user input — quick search in the
  panel, Find, wildcard masks, path entry — MUST treat canonically
  equivalent input and stored names as matching, regardless of which
  composition form either side uses.
- **FR-009**: Sorting MUST produce a deterministic, locale-sensible
  order for names containing any Unicode content; canonically
  equivalent names MUST collate adjacently.

**Cross-cutting**

- **FR-010**: Paths persisted by the application (history, favorites,
  configuration, session state) MUST round-trip long and Unicode paths
  without loss across restarts.
- **FR-011**: System integration surfaces — clipboard (file and text
  forms), drag & drop, shell context menus, launching associated or
  user-specified external programs — MUST convey the exact stored name
  and full path.
- **FR-012**: All bundled plugins (archivers, viewers,
  filesystem/network, utilities — 35 in total) MUST support long paths
  and Unicode names for the operations they provide (browsing,
  packing/unpacking, viewing, transferring) at parity with the
  equivalent core operations.
- **FR-013**: The plugin interface MUST be extended to convey full
  Unicode names and long paths between the core and plugins; the
  extension MUST be documented before modification and MUST provide a
  migration path for third-party plugins built against the legacy
  interface (per project constitution).
- **FR-014**: Where a legacy third-party plugin or an external
  consumer cannot represent a given name or path, the application MUST
  detect this before data is modified, skip or refuse the affected
  items with an actionable per-item message, and complete the
  remaining items.
- **FR-015**: Behavior for names and paths that worked before this
  change MUST remain unchanged (no regressions in operations, display,
  sorting, or configuration compatibility), including for third-party
  plugins built against the legacy interface.

### Key Entities

- **File name**: The exact sequence of characters the operating system
  stores for a directory entry; may contain any Unicode content in any
  composition form; the application treats it as opaque and preserves
  it.
- **Absolute path**: The full location of an item, up to the operating
  system's maximum length (~32,000 characters); may combine local
  drive and network (UNC) syntax.
- **Directory listing**: The panel's view of a folder; must be a
  faithful, complete projection of what the operating system reports.
- **File operation**: A user-initiated action (copy, move, rename,
  delete, view, ...) over one or more items; carries per-item success
  or error state.
- **Integration boundary**: Any point where names/paths leave the
  delivered program — legacy third-party plugins, clipboard, drag &
  drop, external programs — each with a declared capability level for
  long/Unicode names. Bundled plugins sit *inside* the full-capability
  scope, not at this boundary.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can complete 100% of the basic file-operation
  matrix (browse, view, copy, move, rename, delete, create, attribute
  change) on items at path depths up to the operating system maximum —
  every operation the system's own file explorer can perform on the
  same item succeeds in Salamander as well.
- **SC-002**: Users can complete 100% of the same operation matrix on
  files whose names use decomposed accents, characters outside the
  system code page, or characters beyond the basic Unicode plane.
- **SC-003**: File names displayed anywhere in the application are
  rendered identically to the operating system's own file explorer for
  a reference test set covering decomposed, precomposed, non-Latin,
  and emoji names — zero visible deviations.
- **SC-004**: Copying a reference directory tree containing mixed
  composition forms preserves every name bit-exactly — zero renamed,
  substituted, or merged entries.
- **SC-005**: Searching (quick search and Find) locates the target
  file in 100% of test cases where the typed query and the stored name
  differ only in Unicode composition form.
- **SC-006**: A regression checklist of existing everyday operations on
  ordinary short ASCII paths passes with zero behavioral differences
  from the previous release.
- **SC-007**: When an operation involves a component that cannot handle
  a given name, 100% of such cases end with an explicit per-item
  message and zero silently corrupted or lost files.
- **SC-008**: For every bundled plugin, its primary operations complete
  correctly on the long-path and mixed-composition-form reference sets
  (100% of the per-plugin operation matrix, e.g. an archive containing
  decomposed-name entries extracts to a deep destination with all
  names preserved bit-exactly).
- **SC-009**: Listing, sorting, and scrolling a directory of 100,000
  items is not measurably slower than in the previous release —
  elapsed time within ±10% under identical conditions.

## Assumptions

- The application targets Windows 11 and newer (per project
  constitution), where operating-system support for long paths is
  available; enabling it is an application-side responsibility this
  feature covers.
- The per-component name limit (255 characters per single file or
  folder name) is an operating-system constraint and is not extended
  by this feature; only the *total path length* limit is lifted.
- The application never normalizes stored names on its own initiative:
  fixing the defect means *preserving and correctly handling* both
  composition forms, not converting files to one canonical form.
  Form-insensitive behavior applies to matching and collation only.
- Scope covers the whole program (stakeholder decision, see
  Clarifications): the core application and all 35 bundled plugins,
  including the plugin interface extension (FR-012/FR-013). The
  constitution's incremental modernization principle governs *how* the
  work is sequenced and delivered (reviewable, independently testable
  increments — core foundations first, then plugin migration), not the
  final scope. Third-party plugins built against the legacy interface
  remain functional at their current capability via FR-014.
- Distinct-but-equivalent name pairs (US2 scenario 4) are rare in
  practice but legal on Windows volumes; correctness for them is
  required. Beyond the one-time notice defined in FR-007, no further
  disambiguation ergonomics (e.g. permanent visual markers) are
  required in this feature.
- No new mandatory user-facing configuration is introduced; the fixed
  behavior is the default. Any compatibility switch, if one proves
  necessary, must be opt-in and preserve the fixed behavior as default.
- Existing user configuration files load unchanged; no migration step
  is required for users with ordinary paths.
- Rendering of file *content* (e.g. text-encoding detection and
  display inside the internal viewer) is out of scope: this feature
  guarantees such files can be opened from any path with any name,
  while content-encoding improvements remain a separate follow-up
  feature (stakeholder decision, see Clarifications).
