# Feature Specification: Archive Browsing Works — Plugin Archive Associations Self-Heal

**Feature Branch**: `016-archive-browse-assoc`
**Created**: 2026-07-18
**Status**: Draft
**Input**: User: Enter (or click) on a ZIP archive — e.g.
`C:\Users\pavel\AppData\Local\Temp\salamander-test\unicode-test.zip` — opens the
archive in Windows Explorer instead of browsing it inside Salamander. Fix ZIP
browsing (and other archive types). Persisted even after a full configuration
reset, so the cause is in code.

## Problem Statement

Enter on an archive decides "browse in panel" via the archive-association
table (`PackIsArchive`). Forensics of the user's registry (current config, the
pre-reset backup, and the Altap 4.0 source config) proved the chain:

1. On upgrade/import of any pre-105 configuration, the **feature-010
   "packers reset" gate** discards the stored packer tables (which contained a
   working `zip;pk3;jar` plugin association) and rebuilds them from
   `AddDefault(0)` — which creates the ZIP entry in its **legacy form**
   (OldType, plugin reference `-1`).
2. Immediately afterwards `CPlugins::CheckData()` deletes that legacy entry
   (the historical "remove old internal ZIP/TAR/PAK" cull plus the
   invalid-index validation). Result: an association table with only the six
   external-archiver defaults — no `zip`, `7z`, `tar`, `iso`, `cab`, …
3. Nothing ever re-adds plugin associations: they are normally created only
   during a plugin's **first install** (`Connect` with "newly gained function"
   flags). The plugins are already registered (imported/installed), so every
   later `Connect` runs as a no-op upgrade. The damage is persisted and
   self-sustaining — even a fresh config import reproduces it.

Evidence: the user's config backup (whole history) and the current config both
contain **zero plugin-added entries** (no "ZIP (Plugin)" custom packers, no
plugin archive associations), while 18 plugins are correctly registered with
their function flags and extensions.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Enter on an archive browses it in the panel (Priority: P1)

Enter (or double-click) on `unicode-test.zip` (or any `.zip`, `.7z`, `.tar`,
`.iso`, `.cab`, …) opens the archive for browsing inside the Salamander panel,
not in Windows Explorer.

**Why this priority**: Core file-manager functionality; currently broken for
every archive type handled by a plugin.

**Independent Test**: After the fix, start Salamander with the user's CURRENT
(broken) configuration; press Enter on `unicode-test.zip` → the panel enters
the archive.

**Acceptance Scenarios**:

1. **Given** the existing broken config, **When** Salamander starts, **Then**
   the archive associations for all registered archiver plugins are restored
   automatically (self-heal) — no manual configuration needed.
2. **Given** the healed config, **When** the user presses Enter on a `.zip`,
   **Then** the panel browses the archive (Explorer does not open).
3. **Given** other plugin archive types (7z, tar/tgz, iso, cab), **When**
   Enter is pressed, **Then** they browse in the panel too.

---

### User Story 2 - The repair is permanent and does not regress (Priority: P1)

The association survives restarts, config saves, and future upgrades; a fresh
configuration (or a future import) ends up with working plugin associations.

**Acceptance Scenarios**:

1. **Given** the healed config, **When** Salamander is restarted repeatedly,
   **Then** the associations remain intact (no cull/re-heal oscillation).
2. **Given** a fresh config or a pre-105 import, **When** the first start
   completes, **Then** plugin archive associations exist.

---

### Edge Cases

- A user-customized association (e.g. `zip` remapped to an external archiver)
  must NOT be overridden — self-heal only adds extensions that no existing
  entry claims, and only for plugins that currently have no panel-view entry.
- A plugin registered but with panel-view capability off must not gain entries.
- Plugin order/index changes (install/remove) must keep references valid
  (existing renumbering machinery is unchanged).
- Known accepted trade-off: if a user deliberately deleted a plugin's whole
  association, self-heal restores it on next start (documented; the reliability
  of default behavior wins).

## Requirements *(mandatory)*

- **FR-001**: Enter on an archive whose extension is handled by a registered
  archiver plugin MUST browse the archive in the panel.
- **FR-002**: On startup, after plugins and associations are loaded and
  validated, every registered plugin with panel-view capability and declared
  extensions that has NO association entry MUST get one created for those of
  its extensions not claimed by any other entry (edit/pack capability honored).
- **FR-003**: The self-heal MUST be idempotent (no duplicates on repeated
  starts) and MUST NOT modify entries referencing other handlers.
- **FR-004**: The existing user's broken configuration MUST be repaired
  automatically by simply starting the fixed build (no config reset needed).
- **FR-005**: Behavior of non-plugin (external-archiver) associations and the
  rest of the packer tables MUST remain unchanged.

## Success Criteria *(mandatory)*

- **SC-001**: With the user's current config, one start of the fixed build
  restores associations for 100% of registered panel-view plugins (zip, 7zip,
  tar, uniso, uncab at minimum); Enter on `unicode-test.zip` browses in panel.
- **SC-002**: Registry after two consecutive starts is stable (no oscillation).
- **SC-003**: Debug and Release x64 build clean.
- **SC-004**: No change for external-archiver associations (rar/arj/… rows
  untouched).

## Assumptions

- Builds on the forensic root-cause analysis (research.md): the 010 reset +
  legacy-default cull + install-only association creation is the confirmed
  chain; the fix is a standing self-heal at the point the tables are
  reconciled (`CheckData`), matching that function's existing role.
- Autonomous execution per the established pattern; verification = builds +
  a controlled run of the fixed binary against the current broken config with
  registry inspection before/after (the environment can launch and close the
  app), then the user's interactive Enter test.
