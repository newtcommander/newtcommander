# Feature Specification: SFTP File/Directory Context Menu — Attributes & Owner/Group

**Feature Branch**: `018-sftp-file-context-menu`
**Created**: 2026-07-18
**Status**: Draft
**Input**: User (features/file-menu-sftp.md): right-clicking a file/dir in an
SFTP connection currently does nothing. Add a file context menu with items
analogous to the FTP plugin — crucially the ability to set file attributes
(drwxrwxrwx). Additionally add changing a file's/directory's **owner and group**,
including a **recursive** option that applies the change to all nested
subdirectories and files when the target is a directory. Analyze the FTP plugin
first (≥2 agents), then implement the equivalent for SFTP. Full speckit flow.

## Problem Statement

The SFTP plugin (features 008/009/017) already lists, transfers, renames,
deletes and **changes Unix permissions** (chmod drwxrwxrwx, with a working
recursive apply — `ChmodRecursive`). But right-clicking a panel item does
nothing: `CPluginFSInterface::ContextMenu` is an empty stub, so the
already-implemented "Change Attributes" command and other file operations are
not discoverable from the panel the way the FTP plugin exposes them. And there
is no way to change a remote file's/directory's **owner or group**, which SFTP
(libssh2 setstat with UID/GID) fully supports.

Two gaps to close:
1. **No file context menu**: a right-click on files/dirs must present the
   relevant operations — analogous to the FTP plugin — with **Change
   Attributes** (chmod) prominent, so the existing chmod (incl. recursion) is
   reachable, plus the standard file operations that apply to SFTP items.
2. **No owner/group change**: add changing owner (UID) and group (GID) of a
   file or directory, with a **recursive** option for directories that applies
   to every nested entry.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Right-click gives a useful file menu incl. Change Attributes (Priority: P1)

A user right-clicks a file or directory in an SFTP panel and gets a context
menu whose items are analogous to the FTP plugin's, with **Change
Attributes** (chmod / drwxrwxrwx) available; choosing it opens the existing
permissions dialog and applies the change (recursively for a directory when
that option is chosen).

**Why this priority**: The core reported gap ("nic se neděje" on right-click)
and the explicitly "crucial" attribute-setting.

**Independent Test**: Against the local test SFTP server, right-click a file →
Change Attributes → set 0644 → applies; right-click a directory → Change
Attributes → 0755 + recursive → applies to the whole subtree.

**Acceptance Scenarios**:

1. **Given** a file/dir selected in an SFTP panel, **When** the user
   right-clicks, **Then** a context menu appears with the applicable operations
   (not an empty/no-op).
2. **Given** the context menu, **When** the user chooses Change Attributes,
   **Then** the permissions dialog opens seeded with the item's current mode and
   applies the new mode on OK.
3. **Given** a directory and the recursive option, **When** Change Attributes is
   applied, **Then** the mode is set on the directory and every nested file/dir.
4. **Given** a multi-selection, **When** Change Attributes is applied, **Then**
   it applies to all selected items (tri-state handling as today).

---

### User Story 2 - Change owner and group, recursively for directories (Priority: P1)

A user changes the owner (UID) and/or group (GID) of a selected file or
directory. For a directory, a "apply recursively to subdirectories and files"
option applies the change to the entire subtree.

**Why this priority**: Explicitly requested new capability.

**Independent Test**: Right-click a file → Change Owner/Group → set uid/gid →
applies (verify via listing owner/group column or re-stat). Right-click a
directory → set owner/group + recursive → the whole subtree changes.

**Acceptance Scenarios**:

1. **Given** a selected file, **When** the user changes owner and/or group,
   **Then** the change is applied to that file on the server.
2. **Given** a selected directory with the recursive option, **When** applied,
   **Then** owner/group are changed on the directory and every nested entry.
3. **Given** the user leaves owner or group unchanged, **When** applied,
   **Then** only the specified field(s) change; an unset field is left intact.
4. **Given** the server rejects the change (not permitted), **When** applied,
   **Then** a clear error is shown and the operation reports the failure
   (no crash; partial progress on recursion is handled gracefully).

---

### User Story 3 - No regression to existing SFTP operations (Priority: P2)

Existing SFTP behavior (browse, view, copy/move, rename, delete, the existing
chmod path via any current invocation) keeps working; the new menu and
owner/group operation do not destabilize the session or the panel.

**Acceptance Scenarios**:

1. **Given** the new context menu, **When** an item like Copy/Delete is chosen,
   **Then** it performs the existing operation (or is omitted if not applicable).
2. **Given** a dropped/lost connection, **When** a menu action needs the
   session, **Then** it reconnects or fails cleanly (existing EnsureConnected).

---

### Edge Cases

- Multi-selection (files + dirs) for both attributes and owner/group.
- Recursion into a large/deep tree; cancel mid-operation; a subtree the user
  cannot modify (permission denied) — report and continue/skip, never crash.
- Owner/group by numeric UID/GID (SFTP protocol is numeric); name resolution is
  best-effort/optional (the listing already shows names when the server sends
  them).
- A symlink target vs the link itself (apply to the named item; do not follow
  into unintended places during recursion — directories only recurse into real
  subdirectories, not through symlinks, matching the existing chmod walk).
- Server that does not permit chown (common for non-root) — clear message.
- Item with unknown current mode/owner (server didn't report) — dialog still
  usable with sensible defaults.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Right-clicking a file/dir (or selection) in an SFTP panel MUST
  present a context menu with operations analogous to the FTP plugin, including
  a prominent **Change Attributes** (chmod) item; the menu MUST reflect the
  selection (enable/omit items that do not apply).
- **FR-002**: Change Attributes MUST use the existing permissions dialog and
  operation (drwxrwxrwx, octal, special bits, multi-selection tri-state) and
  MUST apply recursively to a directory subtree when the recursive option is
  chosen (existing `ChmodRecursive`).
- **FR-003**: The plugin MUST provide changing a file's/directory's **owner
  (UID)** and **group (GID)** on the server (SFTP setstat UID/GID).
- **FR-004**: For a directory, owner/group change MUST offer a **recursive**
  option that applies to the directory and every nested file and subdirectory.
- **FR-005**: An owner or group field left "unchanged" MUST leave that attribute
  intact on the server (only the specified attribute(s) change).
- **FR-006**: Errors (permission denied, disconnect, protocol error) MUST be
  reported clearly and MUST NOT crash; a recursive operation MUST handle a
  failed/for-skipped entry gracefully and be cancellable.
- **FR-007**: The new menu and operations MUST NOT regress existing SFTP
  browse/view/copy/move/rename/delete/chmod behavior or session stability.
- **FR-008**: The implementation MUST follow the FTP plugin's proven pattern for
  the context menu and command dispatch (analyzed by the audits) rather than
  inventing a divergent mechanism, so behavior is consistent across the two FS
  plugins.

### Key Entities

- **SFTP panel item**: a file or directory (CFileData) in the SFTP FS, with a
  Unix mode, owner (UID/name), group (GID/name).
- **Attributes (chmod) change**: existing mode dialog + recursive apply.
- **Owner/Group (chown) change**: new UID/GID change with a recursive option.
- **Context menu**: the right-click popup, built analogously to FTP, dispatching
  to the FS operations.

## Success Criteria *(mandatory)*

- **SC-001**: Right-clicking an SFTP file/dir shows a non-empty, relevant
  context menu; Change Attributes is present and opens the permissions dialog.
- **SC-002**: chmod from the menu applies correctly to a file, a multi-selection,
  and a directory subtree (recursive), verified against the test server.
- **SC-003**: Owner and group can be changed on a file and, recursively, on a
  directory subtree; an unchanged field is left intact; verified via the
  owner/group listing or re-stat.
- **SC-004**: Permission-denied / disconnect during any menu action yields a
  clear message and no crash; recursion is cancellable and handles skips.
- **SC-005**: Debug and Release x64 build clean; no regression to existing SFTP
  operations.

## Assumptions

- Builds on features 008/009/017: working SFTP transport, listing (owner/group
  already parsed), chmod + recursive apply, and the connect/session machinery.
- SFTP owner/group is numeric on the wire (UID/GID); the dialog accepts numeric
  values (and may accept a name that the server maps, best-effort). The listing
  already displays names when provided.
- The context-menu mechanism and command dispatch mirror the FTP plugin
  (per the audits) using the Salamander plugin-FS API; no core/ABI change.
- Verification: builds + static review + interactive test against the local
  test SFTP server (feature 017: localhost:2222, sftptest). Owner/group change
  may require a server/user permitted to chown; where not permitted, the clear
  error path is what is verified.
