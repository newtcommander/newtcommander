# Feature Specification: SFTP Plugin — Remote File Management over SSH

**Feature Branch**: `008-sftp-plugin`
**Created**: 2026-07-16
**Status**: Draft
**Input**: User description: "Nacti specifikaci ze souboru ./features/sftp-plugin.md" — the referenced file requests a new **sftp** plugin that connects to remote servers over SFTP (SSH) and provides full-featured file management, optimized for administering Linux servers. It mandates UI and integration parity with the existing FTP plugin, password and private-key authentication, host-key verification, the full set of file operations, and — as the key differentiator — display and editing of classic Unix permissions instead of Windows attributes.

## Clarifications

### Session 2026-07-16

- Q: Should password authentication transparently handle keyboard-interactive (single password prompt) when the server disables plain password auth? → A: Yes — the "password" auth method covers both plain password and single-prompt keyboard-interactive; the user only ever enters a password. Multi-prompt keyboard-interactive (2FA/PAM dialogs) remains a stretch item.
- Q: Must viewing remote files (F3, via download to a local cache) work in the initial release? → A: Yes — F3 view fetches the file to a temporary cache and opens the standard viewer, matching the FTP plugin. In-place editing (F4) is out of scope for v1.
- Q: Where should trusted host keys be stored and looked up? → A: In the plugin's own trust store, persisted within the application configuration alongside saved connections. OpenSSH's `known_hosts` file is not consulted in v1 (possible later enhancement).
- Q: How should recursive downloads treat symlinks encountered inside a directory tree? → A: File symlinks are downloaded as their target's content; directory symlinks are not descended into and are reported as skipped (prevents cycles and duplication). Explicitly entering a directory symlink in the panel still navigates into it.
- Q: Should the initial release include a per-connection session log window (parity with the FTP plugin's Logs)? → A: Yes — each connection produces a human-readable session log (connection progress, authentication steps, host-key result, operations, errors) viewable in a Logs window like the FTP plugin's. Secrets are never written to the log.

## Problem Statement

Open Salamander can manage files on FTP servers through its FTP plugin,
but it has no way to connect to servers that only expose **SFTP over
SSH** — which today means practically every Linux server, where plain
FTP is disabled by default and SSH is the universal management channel.
The one plugin that historically covered SFTP (`winscp`) was removed in
feature 007 because it depended on a proprietary runtime that cannot
ship under GPLv2 and had been unbuildable for years.

This feature adds a first-class **SFTP plugin**: users connect to a
Linux server with a password or an SSH private key, browse the remote
file system in a Salamander panel exactly as they would with the FTP
plugin, transfer files in both directions, and manage remote content —
including the operations that matter on Linux and that a Windows-centric
view cannot express: **Unix permissions (`drwxr-xr-x`), owner and group,
symlinks, and chmod**. Server identity is verified via host-key
fingerprints so users are protected against server spoofing from the
very first connection.

The plugin must look and behave like the FTP plugin wherever the two
overlap (connection dialog, saved connections with optional stored
passwords, settings, windows), so existing users need to learn nothing
new — only the protocol underneath differs.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Connect to a Linux Server and Browse Files (Priority: P1)

A user opens the SFTP plugin from the panel's drive/plugin menu, creates
a new connection by entering host, port (pre-filled with 22), user name,
and password, and connects. The remote directory appears in the panel
and the user navigates the remote file system like any other path —
including files and folders with accented, Cyrillic, or CJK names.

**Why this priority**: This is the core of the feature — without
connecting and listing, nothing else exists. Together with password
authentication it forms the smallest demonstrable slice.

**Independent Test**: Can be fully tested by connecting to a real
OpenSSH server with a user name and password and browsing directories
in the panel. Delivers immediate value: read access to any Linux server.

**Acceptance Scenarios**:

1. **Given** a reachable OpenSSH server and valid credentials, **When** the user creates a connection (host, port 22, user, password) and connects, **Then** the panel shows the remote directory listing (the connection's initial remote path, or the user's home directory when none is set).
2. **Given** a connected session, **When** the user enters a subdirectory or navigates to the parent, **Then** the listing updates to the new directory with correct names, sizes, dates, and entry types.
3. **Given** a remote directory containing names with diacritics and non-Latin scripts, **When** it is listed, **Then** every name renders correctly in the panel.
4. **Given** an invalid password, **When** the user attempts to connect, **Then** a clear error message identifies the authentication failure and the user can correct the credentials and retry without restarting the application.
5. **Given** an unreachable host or closed port, **When** the user attempts to connect, **Then** a comprehensible error distinguishes "cannot reach server" from "server refused credentials".
6. **Given** a server that disables plain password authentication but offers a single keyboard-interactive password prompt, **When** the user connects with the password auth method, **Then** the connection succeeds using the entered password with no extra interaction.

---

### User Story 2 - See and Change Unix Permissions (Priority: P1)

While browsing a remote Linux server, the user sees each entry's classic
Unix permissions (`drwxr-xr-x`), its owner, and its group directly in the
panel columns — instead of meaningless Windows attributes. The user
selects one or more entries, opens a permissions dialog, changes the
mode (e.g. `0644` → `0755`), and the change is applied on the server.

**Why this priority**: Explicitly designated the key requirement of the
feature. Displaying and editing Unix permissions is the main reason a
Linux administrator would choose this plugin over generic transfer
tools; without it the plugin is just another file-copy channel.

**Independent Test**: Connect to a server with a prepared set of entries
(regular file, directory, symlink, setuid binary, sticky directory),
verify each renders the correct symbolic permission string, owner, and
group; change a file's mode via the dialog and verify the server
reflects it.

**Acceptance Scenarios**:

1. **Given** a listed remote directory, **When** the user views the panel, **Then** each entry shows its permissions symbolically (`drwxrwxrwx` form), including the type character (`-`, `d`, `l`, …).
2. **Given** entries with setuid, setgid, or sticky bits, **When** they are listed, **Then** the special bits are visible in the symbolic string (e.g. `rwsr-xr-x`, `rwxrwxrwt`).
3. **Given** a listed entry, **When** the user views owner and group columns, **Then** they show names when the server provides them, otherwise the numeric uid/gid.
4. **Given** a selected remote file, **When** the user opens the permissions (chmod) dialog, changes the mode, and confirms, **Then** the new permissions are applied on the server and shown after refresh.
5. **Given** a chmod attempt on an entry the user does not own, **When** the server rejects it, **Then** the error is reported clearly and the listing remains consistent.
6. **Given** the permissions display, **When** the user prefers octal, **Then** an octal representation (e.g. `0755`) is available in addition to the symbolic one.
7. **Given** a user who prefers the classic attribute view, **When** they switch the display mode, **Then** the panel can show the original attribute-style columns instead of Unix rights, fitting the existing column layout.

---

### User Story 3 - Transfer and Manage Remote Files (Priority: P1)

The user copies files and directories from the remote server to the
local panel (download) and back (upload), renames and moves remote
entries, deletes files and directories, creates directories, reads where
symlinks point and creates new ones, and adjusts file modification
times — the complete day-to-day management workflow.

**Why this priority**: Browsing alone is read-only; server
administration requires the full operation set. This story completes
the "plnohodnotná práce se soubory" (full-featured file work) goal.

**Independent Test**: Against a real server, run a round-trip: upload a
directory tree, rename entries, create/remove directories, create a
symlink and read its target, download the tree back, and compare
contents byte-for-byte.

**Acceptance Scenarios**:

1. **Given** a connected session, **When** the user copies remote files/directories to the local panel, **Then** the download completes with contents identical to the originals and progress is shown with the option to cancel.
2. **Given** local files selected, **When** the user copies them to the remote panel, **Then** the upload completes and the files appear in the remote listing.
3. **Given** an interrupted large transfer, **When** the user retries the same transfer, **Then** the plugin offers to resume from the interruption point instead of restarting from zero.
4. **Given** a remote entry, **When** the user renames or moves it (including names with non-ASCII characters), **Then** the entry appears under the new name/location and the old one is gone.
5. **Given** selected remote files or directories, **When** the user deletes them, **Then** they are removed from the server after the same confirmation flow used elsewhere in the application.
6. **Given** a connected session, **When** the user creates a directory or removes an empty one, **Then** the listing reflects the change.
7. **Given** a remote symlink, **When** it is listed, **Then** its target is visible; **When** the user creates a new symlink, **Then** it appears on the server with the requested target.
8. **Given** a remote file, **When** the user sets its modification time, **Then** the server stores the new timestamp.
9. **Given** a remote file, **When** the user invokes the View command (F3), **Then** the file is fetched to a temporary local cache and opens in the application's standard viewer, exactly as with the FTP plugin.

---

### User Story 4 - Verify the Server's Identity (Priority: P2)

On the first connection to a server, the user is shown the server's
host-key fingerprint and decides whether to trust it; accepted keys are
remembered. If a known server's key later changes, the user gets an
unmistakable warning and the connection is not established unless they
explicitly accept the new key. The plugin never accepts a host key
silently.

**Why this priority**: A security MUST — it protects credentials and
data from server spoofing. It is P2 only because it activates as part
of the connection flow delivered in Story 1 and can be layered onto it;
it must ship before any public release.

**Independent Test**: Connect to a fresh server → fingerprint prompt
appears; accept → subsequent connections are silent; change the server's
host key → warning appears and connection is refused until explicitly
accepted.

**Acceptance Scenarios**:

1. **Given** a server never connected to before, **When** the user connects, **Then** the host-key fingerprint is displayed and the user chooses to trust (and store) it, connect once without storing, or abort.
2. **Given** a stored host key, **When** the user reconnects to the same server and the key matches, **Then** no prompt appears.
3. **Given** a stored host key, **When** the server presents a different key, **Then** a prominent warning explains the risk (possible server change or attack) and the connection proceeds only after explicit user acceptance; declining leaves the stored key unchanged.
4. **Given** any connection attempt, **When** the host key is unknown or mismatched, **Then** there is no code path that accepts it without user interaction.

---

### User Story 5 - Authenticate with a Private SSH Key (Priority: P2)

A user whose server disallows password login configures a connection to
use a private key file instead: they pick the key file, and if the key
is protected by a passphrase they enter it at connect time — or choose
to store it, protected the same way as saved passwords. Common key
formats work: OpenSSH, PEM/PKCS#8, and ideally PuTTY's `.ppk`.

**Why this priority**: Key-based login is the norm on hardened Linux
servers (password authentication is frequently disabled), so this
unlocks a large share of real-world servers. It builds on the
connection flow of Story 1.

**Independent Test**: Generate keys in each supported format, configure
a connection with each, connect against a real OpenSSH server with
password authentication disabled; verify passphrase prompt, stored
passphrase, and wrong-passphrase handling.

**Acceptance Scenarios**:

1. **Given** a connection configured with a private key file (auth method "key"), **When** the user connects to a server that accepts that key, **Then** the session is established without a password.
2. **Given** a passphrase-protected key, **When** the user connects, **Then** they are prompted for the passphrase (unless it is stored) and may choose to store it with the same protection as saved passwords.
3. **Given** a wrong passphrase, **When** the user connects, **Then** the error clearly says the key could not be unlocked (as opposed to the server rejecting the key) and the user may retry.
4. **Given** key files in OpenSSH and PEM/PKCS#8 formats, **When** used for connections, **Then** both formats work; **Given** a PuTTY `.ppk` file, **Then** it either works or is rejected with a message naming the expected formats.
5. **Given** a configured key file that no longer exists on disk, **When** the user connects, **Then** the error names the missing file path.

---

### User Story 6 - Save Connections and Reuse Them After Restart (Priority: P2)

The user manages a list of saved connections — create, edit, delete,
organize — exactly as in the FTP plugin's bookmark dialog. A saved
connection remembers host, port, user, authentication method, key-file
path, optional initial remote path, and optionally the password or key
passphrase. Everything survives an application restart, and connecting
to a saved server with a stored secret requires no typing at all.

**Why this priority**: Server administrators connect to the same
machines daily; retyping settings would make the plugin impractical.
Depends on Story 1's connection definition but is separately testable.

**Independent Test**: Create a saved connection with a stored password,
restart the application, connect from the saved list without entering
anything; verify edit and delete flows.

**Acceptance Scenarios**:

1. **Given** the connection dialog, **When** the user saves a connection with all its settings, **Then** it appears in the saved-connections list and can be connected with one action.
2. **Given** saved connections (including a stored password and a stored key passphrase), **When** the application is restarted, **Then** the list and the stored secrets are intact and functional.
3. **Given** a saved connection, **When** the user chooses not to store the password, **Then** each connection attempt prompts for it and it is never written to persistent storage.
4. **Given** the saved-connections UI, **When** compared with the FTP plugin's, **Then** the layout, workflow, and capabilities are recognizably the same (a user of one can use the other without instructions).

---

### User Story 7 - Survive Network Problems Gracefully (Priority: P3)

During long sessions the connection stays alive through keepalives;
when the network drops anyway, the next operation detects it and the
plugin reconnects (or clearly offers to), returning the user to the
directory they were in. Slow servers produce timeouts with
understandable messages, huge directories list without freezing the
application, and multi-gigabyte files transfer reliably.

**Why this priority**: Robustness is what separates a usable
administration tool from a demo, but it refines flows delivered by
earlier stories rather than adding new capability.

**Independent Test**: Simulate a network interruption mid-session and
mid-transfer, verify reconnect behavior and message quality; list a
directory with tens of thousands of entries; transfer a file larger
than 4 GB.

**Acceptance Scenarios**:

1. **Given** an idle connected session, **When** the network briefly drops and returns, **Then** keepalives or the next user action re-establish the session without losing the panel's current directory.
2. **Given** a lost connection, **When** the user invokes any remote operation, **Then** the plugin attempts reconnection (or asks first) instead of failing with a cryptic error.
3. **Given** an interrupted transfer, **When** the connection is restored, **Then** the user can resume the transfer (see Story 3, scenario 3).
4. **Given** a remote directory with tens of thousands of entries, **When** it is listed, **Then** the panel remains responsive and the listing can be cancelled.
5. **Given** a file larger than 4 GB, **When** it is transferred in either direction, **Then** it completes with correct size and content.
6. **Given** any failure (timeout, refused operation, out of space), **When** it is reported, **Then** the message states what failed and why in user terms, not just an internal error code.

---

### Edge Cases

- **Host key changed after server reinstall**: legitimate but indistinguishable from an attack — warning must let the user replace the stored key deliberately (Story 4).
- **Server provides only numeric uid/gid** (older protocol level): owner/group columns show numbers; no crash or blank columns.
- **Broken symlink** (target does not exist): listed with its target, clearly not traversable as a directory; operations on it fail with clear messages.
- **Symlink pointing to a directory**: entering it navigates to the target; the panel path reflects where the user is. During recursive downloads it is not descended into (reported as skipped) so symlink cycles cannot cause runaway transfers.
- **File names that are not valid UTF-8** on the server: shown in a lossless-or-clearly-marked fallback form; operations on such entries either work or fail with an explicit naming problem — never silently corrupt names.
- **Extremely long remote paths / names** exceeding classic Windows limits: handled consistently with the application's long-path support when transferring to local disks.
- **Permission denied** on listing a directory or reading a file: reported per entry/operation; the session stays usable.
- **Local disk full during download / remote quota exceeded during upload**: transfer stops with a clear message; partial files are identified so resume can pick up.
- **User cancels** a long listing or transfer: operation stops promptly; session remains usable.
- **Concurrent panels**: both panels (or a second window) connected to the same server work independently without corrupting each other's state.
- **Chmod on a symlink**: applies to the target per platform convention; no surprising recursion.
- **Timezone / DST**: remote timestamps display consistently with how the application shows local file times.

## Requirements *(mandatory)*

### Functional Requirements

#### Connection & Authentication

- **FR-001**: The plugin MUST provide a connection definition with: host, port (defaulting to 22), user name, authentication method, and an optional initial remote path.
- **FR-002**: The plugin MUST support password authentication, including servers that deliver the password prompt through a single-prompt keyboard-interactive exchange (common on hardened OpenSSH with plain password auth disabled); the fallback is transparent — the user only ever enters a password.
- **FR-003**: The plugin MUST support private-key authentication with a per-connection key file; OpenSSH and PEM/PKCS#8 key formats MUST be accepted, PuTTY `.ppk` SHOULD be accepted, and unsupported formats MUST be rejected with a message naming the supported ones.
- **FR-004**: The plugin MUST support passphrase-protected keys, prompting at connect time, with the option to store the passphrase under the same protection as stored passwords.
- **FR-005**: Stored secrets (passwords, passphrases) MUST use the same secure-storage mechanism and protection level as the FTP plugin's saved passwords, and MUST survive application restarts.
- **FR-006**: The plugin MUST verify server host keys: on first contact it MUST show the key's fingerprint and let the user trust-and-store, connect once, or abort; on a key mismatch it MUST warn prominently and refuse to proceed without explicit acceptance. No code path may accept an unknown or changed key without user interaction.
- **FR-007**: The plugin MUST let users create, edit, delete, and organize saved connections, persisted across restarts, with UI and workflow matching the FTP plugin's connection management.
- **FR-008**: The plugin SHOULD support authentication via a running SSH agent (ssh-agent / Pageant) and multi-prompt keyboard-interactive authentication (e.g. 2FA verification codes). *(Stretch — not required for initial release; the single-prompt password case is covered by FR-002.)*

#### Browsing & File Operations

- **FR-009**: The plugin MUST list remote directories in the panel with name, size, date/time, entry type (file, directory, symlink, other), permissions, owner, and group.
- **FR-010**: The plugin MUST handle UTF-8 file names correctly in every operation (list, transfer, rename, delete, create); names that are not valid UTF-8 MUST NOT be silently corrupted.
- **FR-011**: The plugin MUST support download and upload of files and directory trees with progress display and cancellation; interrupted transfers SHOULD be resumable from the point of interruption.
- **FR-012**: The plugin MUST support rename and move of remote entries.
- **FR-013**: The plugin MUST support deleting remote files and directories, creating directories, and removing directories.
- **FR-014**: The plugin MUST display symlink targets and MUST be able to create new symlinks. In recursive transfers, symlinks to files are transferred as their target's content, while symlinks to directories are not descended into and are reported as skipped; explicitly entering a directory symlink in the panel still navigates into it.
- **FR-015**: The plugin MUST support changing permissions (chmod) and setting file modification times on remote entries.
- **FR-016**: The plugin MAY support changing owner and group (chown/chgrp). *(Stretch — not required for initial release.)*
- **FR-031**: The plugin MUST support viewing a remote file (View command, F3) by fetching it to a temporary local cache and opening the application's standard viewer, matching the FTP plugin's behavior; in-place editing of remote files is out of scope for the initial release.

#### Permissions Display *(key requirement)*

- **FR-017**: The panel MUST display each entry's Unix permissions symbolically in `drwxrwxrwx` form, including the entry-type character (`-`, `d`, `l`, …) and the special bits (setuid, setgid, sticky) in their conventional positions.
- **FR-018**: An octal representation of the mode (e.g. `0755`) SHOULD be available in addition to the symbolic form.
- **FR-019**: The panel MUST display owner and group — names when the server provides them, numeric uid/gid otherwise.
- **FR-020**: The plugin MUST provide a permissions dialog through which the user changes an entry's mode; the dialog MUST be usable for single entries and SHOULD support multiple selected entries.
- **FR-021**: The permissions display SHOULD be switchable between Unix rights and the original attribute-style view so it fits the existing panel column system.

#### Reliability & Error Handling

- **FR-022**: The plugin MUST apply connection and operation timeouts and send keepalives to hold idle sessions open; timeout behavior MUST be configurable in the plugin's settings.
- **FR-023**: After a connection loss the plugin MUST re-establish the session (automatically or after user confirmation) and return the user to their working directory; it MUST NOT require an application restart.
- **FR-024**: Error messages MUST state what failed and why in terms the user can act on (e.g. authentication failed vs. host unreachable vs. permission denied vs. disk full).
- **FR-025**: The plugin MUST handle files larger than 4 GB and directories with tens of thousands of entries without freezing the user interface.
- **FR-026**: The plugin MAY support parallel transfers and transport compression. *(Stretch — not required for initial release.)*
- **FR-032**: Each connection MUST produce a human-readable session log (connection progress, authentication steps, host-key verification result, operations, errors) viewable in a Logs window matching the FTP plugin's; passwords, passphrases, and key material MUST never appear in the log.

#### Integration & Compatibility

- **FR-027**: The plugin's dialogs, windows, settings, and overall workflow MUST closely mirror the FTP plugin so that an FTP-plugin user can operate the SFTP plugin without instructions.
- **FR-028**: The existing FTP plugin MUST remain fully functional and unchanged in behavior; shipping this feature MUST NOT regress it.
- **FR-029**: The plugin MUST integrate into the product's standard plugin lifecycle: it is registered like other plugins, participates in the build-time plugin configuration (`plugins.cfg`), and appears in the Plugin Manager.
- **FR-030**: All user-visible texts MUST follow the product's localization pattern (language module, English first), consistent with other plugins.

### Key Entities

- **Connection profile**: A saved definition of a remote server — host, port, user name, authentication method, key-file path, optional initial remote path, optional stored secret (password or passphrase), display name/organization within the saved list.
- **Server trust record**: The remembered identity (host-key fingerprint) of a server the user has decided to trust, keyed by host and port; consulted on every connection, updated only by explicit user decision. Stored in the plugin's own trust store within the application configuration (OpenSSH's `known_hosts` file is not consulted in the initial release), and persisted across restarts like saved connections.
- **Remote file entry**: An item in a remote directory listing — name, entry type (file/directory/symlink/other), size, modification time, Unix mode (permission bits including special bits), owner, group, symlink target where applicable.
- **Transfer operation**: A download or upload of one or more files/trees — tracks progress, supports cancellation, and records enough state for resuming an interrupted transfer.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Connecting with a password and connecting with a private key (with and without passphrase) both succeed against a real OpenSSH server on current Linux.
- **SC-002**: 100% of entries in a remote listing show a correct symbolic permission string, owner, and group; a permissions change made in the dialog is visible on the server and in the refreshed listing.
- **SC-003**: The first connection to an unknown server presents a fingerprint decision in 100% of cases, and a changed host key is never accepted without an explicit user action.
- **SC-004**: Saved connections — including stored passwords and passphrases — work after an application restart with zero re-entry of data.
- **SC-005**: A directory round-trip (upload a tree, rename, chmod, download back) preserves file contents byte-for-byte and names character-for-character, including non-ASCII names.
- **SC-006**: A user familiar with the FTP plugin can create an SFTP connection and reach a remote listing in under 2 minutes without documentation.
- **SC-007**: A remote directory with 10,000 entries lists without the application becoming unresponsive, and a file of at least 4 GB transfers to completion in each direction.
- **SC-008**: After a forced network interruption, the user regains a working session in the same remote directory without restarting the application.
- **SC-009**: The FTP plugin passes the same smoke checks (connect, list, transfer) after this feature as before it — zero regressions.
- **SC-010**: Core operations (connect, list, download, upload, rename, delete, mkdir, chmod) are covered by automated or scripted verification against a real server.

## Assumptions

- **Target servers** are OpenSSH-based Linux systems. The protocol baseline is what current OpenSSH offers (SFTP protocol version 3); owner/group may therefore arrive as numeric uid/gid, and name resolution is used when the server provides it.
- **The removed `winscp` plugin** (feature 007) is not a constraint or a code source; this plugin is a clean implementation.
- **Secret storage parity** means: whatever protection the FTP plugin applies to saved passwords (including any master-password integration) applies equally here — no weaker, no separate scheme.
- **Stretch items** — ssh-agent/Pageant, multi-prompt keyboard-interactive (2FA), chown/chgrp, parallel transfers, compression — are explicitly *not* required for the initial release (marked SHOULD/MAY above) and their absence does not block Definition of Done.
- **Proxy support** is not required for the initial release; it may come for free if shared connection infrastructure provides it.
- **One server connection per panel**, mirroring the FTP plugin's session model; simultaneous connections from both panels are independent sessions.
- **English resources first**, following the product's plugin language-module pattern; additional translations follow the existing translation process.
- **Resume granularity** is per-file (continue an interrupted file from its last byte), not sub-file integrity verification.
- **In-place editing of remote files (F4)** is out of scope for the initial release; viewing (F3) is in scope (FR-031).

## Dependencies & Process Constraints

- **Analysis before implementation (user-mandated)**: Before any implementation, an analysis of the existing FTP plugin must be produced and **confirmed by the user**. It must answer: the plugin contract (what a plugin implements and how it registers), how the FTP plugin implements the connection dialog, saved-connection management, and secure password storage, how the host application renders columns/attributes, and what language/framework/dependencies the project offers. It must also justify the choice of SSH/SFTP library, preferring dependencies already present in the project. This maps to the planning phase (`/speckit.plan`) and gates `/speckit.implement`.
- **UI reuse over duplication**: Where the FTP plugin's user-facing behavior is shared, the SFTP plugin must not fork-and-diverge; shared foundations are reused so both plugins stay consistent over time.
- **Definition of Done (from the zadání)**: password and key login work against a real OpenSSH server; saved connections including secrets survive restart; permissions display as `drwxrwxrwx` and can be changed; basic operations verified by tests; the FTP plugin is not broken; project code style respected; localized texts provided.
