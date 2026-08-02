# Feature Specification: Hot Path Display Names and Custom Icons

**Feature Branch**: `047-hot-path-names-icons`
**Created**: 2026-08-02
**Status**: Draft
**Input**: User description: "Cílem této funkční úpravy je rozšíření možností Hot Paths. Nyní je možné přidat Hot Path, která se zobrazí v listě Hot Path Bar a zároveň v seznamu jednotek (ALT+F1, ALT+F2) jako adresářová cesta. V nastavení Hot Path přidáme možnost pojmenování - tedy v případě, že bude toto pole vyplněno, bude se název zobrazovat v Hot Path Bar, resp. v dialogu výběru jednotky namísto adresářové cesty, která může být někdy moc dlouhá. Zároveň přidáme možnost změnit ikonu záložky Hot Path - výchozí bude ikona, která je použita nyní, ale uživatel si bude moci ikonu změnit. Připrav sadu několika předdefinovaných ikon včetně té současné, ale v několika barevných variantách."

## Context

A Hot Path is one of 30 bookmark slots pointing to a directory. Hot paths appear on
the Hot Path Bar (a toolbar of clickable buttons), in the Change Drive menu
(Alt+F1 / Alt+F2), in the Go menu submenus, in the hot paths drop-down on the top
toolbar, and in the Windows taskbar jump list. Today every one of these surfaces
labels an entry with what is effectively the directory path: when the user assigns
a hot path (Ctrl+Shift+digit, or "Assign Hot Path" from the directory line), the
entry's label is automatically filled with the raw path, and the only way to change
it afterwards is an undiscoverable in-place rename inside the settings list. Long
paths therefore produce unusably wide buttons and menu entries. Every entry also
shares one fixed bookmark icon with no way to tell entries apart visually.

This feature makes naming a hot path an explicit, optional part of its settings —
a filled-in name replaces the path on every surface, an empty name keeps today's
path display — and adds a per-entry icon chosen from a predefined set of color
variants of the current bookmark icon.

## Clarifications

### Session 2026-08-02

- Q: What should the predefined hot path icon gallery contain? → A: Color
  variants only — one motif (the current bookmark icon) in ~8–10 color variants
  including today's default coloring; no other glyph shapes in this feature.
- Q: How should an upgraded configuration treat an existing hot path whose
  stored name is exactly identical to its stored path (the historical
  auto-fill)? → A: Treat as unnamed — the Name field loads empty, the entry
  displays its path, and the label follows the path if it is later edited.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Show a short custom name instead of a long path (Priority: P1)

A user keeps deep project folders as hot paths (e.g. `D:\Clients\Acme\2026\Website
Redesign\assets\source`). In the Hot Paths settings page they fill in an optional
**Name** field — clearly visible next to the path input — with a short label such
as "Acme assets". From that moment the Hot Path Bar button, the Change Drive menu
entry (Alt+F1/F2), and every other place that lists hot paths shows "Acme assets"
instead of the long path. If the name is left empty, the entry keeps showing its
directory path exactly as today.

**Why this priority**: This is the core of the request — long paths make the Hot
Path Bar and the Change Drive menu hard to read and waste horizontal space. A
short label restores their usefulness.

**Independent Test**: Create a hot path with a long directory path, verify it
displays as the path; enter a name in settings and verify the bar and Alt+F1 menu
now show the name; clear the name and verify the display reverts to the path.

**Acceptance Scenarios**:

1. **Given** a hot path whose name field is empty, **When** the user opens the Hot
   Path Bar or the Change Drive menu, **Then** the entry is labeled with its
   directory path (today's behavior, unchanged).
2. **Given** a hot path with a long path, **When** the user enters "Acme assets"
   into its Name field in settings and confirms, **Then** the Hot Path Bar button,
   the Change Drive menu entry, the Go menu hot path entries and the hot paths
   drop-down all show "Acme assets" instead of the path — without restarting the
   application.
3. **Given** a hot path displaying a custom name, **When** the user hovers over
   its Hot Path Bar button, **Then** the tooltip shows the full directory path.
4. **Given** a hot path with a custom name, **When** the user clears the Name
   field and confirms, **Then** all surfaces revert to showing the directory path.
5. **Given** a settings entry with a name filled in but no path, **When** the user
   confirms the settings, **Then** the entry is rejected with an understandable
   validation message (a hot path is defined by its path).
6. **Given** named hot paths among the first ten slots, **When** the Change Drive
   menu is open, **Then** the digit accelerators (1–9, 0) are still shown and
   still activate the entries.

---

### User Story 2 - Distinguish hot paths with a chosen icon (Priority: P2)

The user has ten hot paths and wants to tell work, personal and archive folders
apart at a glance. In the Hot Paths settings they pick, per entry, an icon from a
predefined gallery: the current bookmark icon (the default) plus the same icon in
several color variants. The chosen icon appears on the Hot Path Bar button, in the
Change Drive menu, in the hot path menus and next to the entry in the settings
list.

**Why this priority**: Valuable visual differentiation, but the feature is fully
usable with names alone; icons build on top of the naming work.

**Independent Test**: Change one hot path's icon to a color variant in settings
and verify the Hot Path Bar, the Change Drive menu and the settings list all show
the variant while other entries keep the default icon.

**Acceptance Scenarios**:

1. **Given** a newly assigned hot path, **When** it appears anywhere, **Then** it
   uses the same bookmark icon the application ships with today.
2. **Given** the Hot Paths settings page, **When** the user opens the icon choice
   for an entry, **Then** a gallery of predefined icons is offered — the current
   bookmark icon plus color variants of it — with the entry's current selection
   indicated.
3. **Given** a hot path assigned the red variant, **When** the user opens the Hot
   Path Bar, the Change Drive menu, or the hot path menus, **Then** the entry
   shows the red variant while unchanged entries keep the default icon.
4. **Given** a hot path with a custom icon, **When** the user deletes the entry in
   settings, **Then** the slot returns to the default icon (and empty name/path).
5. **Given** hot paths with custom icons, **When** the display scale (DPI) or the
   application theme changes, **Then** the icons remain crisp and legible.

---

### User Story 3 - Existing setups and quick assignment stay familiar (Priority: P3)

A user upgrades from the previous build. All their hot paths survive with the
labels they are used to, and every entry shows the default icon. Quick assignment
(Ctrl+Shift+digit, Ctrl+Shift+=, "Assign Hot Path" on the directory line) still
works and produces an entry that displays as its directory path with the default
icon until the user chooses to name it or re-icon it.

**Why this priority**: Protects existing users; no re-learning and no data loss.
It is the compatibility net around the first two stories.

**Independent Test**: Load a configuration saved by the previous build and verify
labels look identical; quick-assign a path with Ctrl+Shift+5 and verify the new
entry appears as its path with the default icon.

**Acceptance Scenarios**:

1. **Given** a configuration saved by a previous build (where labels were
   auto-filled with the path), **When** the upgraded application starts, **Then**
   every hot path is present, is labeled exactly as before, and shows the default
   icon.
2. **Given** any panel path, **When** the user presses Ctrl+Shift+5, **Then** slot
   5 holds that path, displays as the path (no custom name) and uses the default
   icon; the "auto-open settings on assign" option keeps its current behavior.
3. **Given** entries with custom names and icons, **When** the user reorders them
   in settings (Move Up / Move Down), **Then** each entry's name and icon travel
   with it (its keyboard shortcut follows its new position, as today).

---

### Edge Cases

- A name consisting only of spaces is treated as empty — the entry displays its
  path.
- A very long name must not break the Hot Path Bar layout; the label may be
  truncated for display while the tooltip still shows the full path.
- A name containing `&` is displayed literally (not interpreted as a menu
  accelerator marker), matching how such text is handled elsewhere.
- Two hot paths may carry the same name; both keep working independently (an
  entry's identity is its slot and path, not its name).
- Slots beyond the first ten (which have no keyboard shortcut) support names and
  icons exactly like the first ten.
- Entries hidden from the Change Drive menu (visibility unchecked) still show
  their name and icon on the Hot Path Bar and in the other menus, as their path
  does today.
- The taskbar jump list, which lists visible hot paths, follows the same
  name-else-path labeling rule.
- All predefined icon variants must be legible in both light and dark application
  themes.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The Hot Paths settings page MUST offer, for each entry, a clearly
  visible and directly editable optional Name field alongside the path input (not
  only the current hidden in-place rename in the list).
- **FR-002**: Every surface that lists hot paths — Hot Path Bar, Change Drive menu
  (Alt+F1/F2), Go menu hot path entries, the hot paths drop-down menu, the
  directory-line "Assign Hot Path" submenu, the taskbar jump list, and the
  settings list — MUST label an entry with its name when the name is non-empty,
  and with its directory path when the name is empty.
- **FR-003**: When a custom name is displayed, the full directory path MUST remain
  discoverable: the Hot Path Bar tooltip shows the path, and the settings page
  shows both name and path.
- **FR-004**: A hot path MUST be defined by its path: an entry with a name but no
  path is invalid and MUST be rejected with an understandable message when
  confirming settings.
- **FR-005**: Leading and trailing whitespace in a name MUST be ignored; a
  whitespace-only name counts as empty.
- **FR-006**: Each hot path MUST carry an icon selection, defaulting to the
  bookmark icon used today; the user MUST be able to change it per entry from a
  predefined gallery in the Hot Paths settings.
- **FR-007**: The predefined gallery MUST consist of a single motif — the current
  bookmark icon — offered in 8 to 10 color variants including today's default
  coloring (the default selection); all variants MUST be legible in light and
  dark themes and at display scales from 100 % to 200 %. No other glyph shapes
  are part of this feature.
- **FR-008**: The selected icon MUST be shown on the Hot Path Bar button, in the
  Change Drive menu, in the hot path menus, and next to the entry in the settings
  list.
- **FR-009**: Names and icon selections MUST persist with the application
  configuration across restarts, alongside the existing per-entry path and
  visibility values.
- **FR-010**: Configurations saved by previous builds MUST load without data
  loss: an entry whose stored name is exactly identical to its stored path is
  treated as unnamed (empty Name field, path displayed, label follows later path
  edits); an entry whose stored name differs from its path keeps that name as its
  custom label; every loaded entry starts with the default icon. In both cases
  the labels visible after upgrade are identical to those before it.
- **FR-011**: Quick assignment (Ctrl+Shift+1..0, Ctrl+Shift+=, "Assign Hot Path"
  from the directory line) MUST produce an entry with no custom name (so it
  displays as its path) and the default icon.
- **FR-012**: Reordering entries in settings MUST keep each entry's name, path,
  visibility and icon together as one unit.
- **FR-013**: Deleting an entry in settings MUST reset the slot completely: empty
  name, empty path, default visibility and default icon.
- **FR-014**: After a display-scale (DPI) or theme change, hot path icons MUST be
  re-rendered so they remain crisp and legible on all surfaces.

### Key Entities

- **Hot Path entry**: One of 30 bookmark slots. Attributes: directory path
  (required for an assigned slot; may contain the existing path variables),
  optional display name, visibility flag for the Change Drive menu (existing,
  unchanged), and an icon selection referencing one item of the predefined icon
  set (default: the current bookmark icon). The first ten slots additionally map
  to the Ctrl+digit shortcuts by position, as today.
- **Predefined icon set**: A fixed gallery shipped with the application — the
  current bookmark motif in its default coloring plus several color variants of
  it. Icons are identified stably so a selection survives restarts and future
  gallery additions.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A hot path with a path of 80+ characters can be presented as a
  user-chosen short label; naming an existing hot path takes under 30 seconds and
  requires no application restart to take effect.
- **SC-002**: Changing a hot path's icon takes at most 4 interactions in the
  settings page and is reflected on all surfaces immediately after confirming.
- **SC-003**: 100 % of the surfaces that list hot paths show the identical label
  for a given entry (name when set, otherwise path) — no surface disagrees.
- **SC-004**: Upgrading with an existing configuration produces zero visible
  differences in hot path labels and zero lost entries until the user edits
  something.
- **SC-005**: Every icon in the predefined set is distinguishable from every other
  at a glance and legible in both light and dark themes at 100–200 % display
  scale.

## Out of Scope

- Picking arbitrary icon files (browsing `.ico`/`.exe`/`.dll` as the User Menu
  does) — the gallery is a fixed predefined set.
- Changing the number of slots (30), the shortcut scheme (Ctrl+digit /
  Ctrl+Shift+digit), or the visibility semantics of the Change Drive menu.
- Showing hot paths on the Drive Bar (they are not shown there today).
- Importing configuration from other products (excluded by project policy).

## Assumptions

- Additional distinct motifs can be added to the gallery in a later feature
  without changing this feature's behavior (icon selections are identified
  stably, so the gallery is extensible).
- When no name is set, the fallback label is the path text exactly as these
  surfaces render it today.
- The taskbar jump list follows the same labeling rule as the menus, since it
  already mirrors the visible hot paths.
- Naming and icons do not alter which entries appear where: the existing
  visibility flag keeps deciding Change Drive menu membership, and the Hot Path
  Bar keeps showing assigned entries.
- The settings list keeps its current columns (label, hotkey) and gains an icon
  preview; the hotkey column remains display-only.
