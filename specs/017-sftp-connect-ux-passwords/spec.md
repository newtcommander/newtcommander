# Feature Specification: SFTP Connect Window — Clear Bookmarks & Working Password Saving

**Feature Branch**: `017-sftp-connect-ux-passwords`
**Created**: 2026-07-18
**Status**: Draft
**Input**: User: In the SFTP connect window (Ctrl+Shift+S) the bookmarks/tabs
are confusing — it is not clear how to create, edit, and save a bookmark; the
functions feel mixed together. Also password saving does not work: a password
typed with "save" chosen was not remembered. Make the connection window more
user-friendly; verify how passwords and local keys are stored and fix the bugs.

## Problem Statement

The SFTP plugin (features 008/009) ships a connect window that manages saved
connections ("bookmarks") plus a quick-connect entry. Two concrete problems:

1. **Password saving is broken.** Forensics (research.md) confirm the
   quick-connect entry (`Config.QuickConnect`) — including its saved password
   blob and all its fields — is held only in memory: `SaveConfiguration` and
   `LoadConfiguration` (sftp.cpp) persist `Config.Bookmarks` and the known-hosts
   store, but **never the quick-connect entry**. A user who quick-connects
   (no bookmark selected), types a password and checks "Save password", loses
   that password when Salamander is restarted — matching the report. (The
   in-session connect itself works, which is why it looks intermittent.)

2. **Bookmark lifecycle is muddled.** The window has New / Rename / Remove /
   Copy / Connect buttons, but:
   - There is **no way to save edits to an existing bookmark**: selecting a
     bookmark loads its fields, but after editing host/user/password/etc. there
     is no "Save"/"Apply" action. Connect uses the edited values yet discards
     them; only New (which prompts for a brand-new name) persists anything.
   - The **Copy button is dead** — its control exists but has no handler.
   - It is not visually clear whether a stored bookmark has a saved password,
     nor which fields belong to the selected bookmark vs. a new connection.

   So "create / edit / save" of a bookmark is not discoverable or complete.

The remaining local-key / password-manager mechanics are also to be verified
and any additional defects fixed (research.md consolidates the agent audits).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - A saved password is actually remembered (Priority: P1)

A user opens the connect window, enters a host/user/password, checks
"Save password", and connects. Later — including after restarting Salamander —
opening the window and connecting to the same target does not require retyping
the password.

**Why this priority**: Directly reported and user-blocking; a "save password"
that silently forgets is worse than no option.

**Independent Test**: Quick-connect with "Save password" → close & reopen
Salamander → connect again → no password prompt / connection succeeds with the
remembered secret. Repeat for a named bookmark.

**Acceptance Scenarios**:

1. **Given** a quick-connect with "Save password" checked, **When** Salamander
   is restarted, **Then** the saved password is still available for that
   connection (persisted, decryptable).
2. **Given** a bookmark with a saved password, **When** the config is saved and
   reloaded, **Then** the bookmark still has its password.
3. **Given** the master-password feature is on or off, **When** a password is
   saved, **Then** it round-trips correctly in either mode (encrypted when a
   master password is set; scrambled otherwise) and is never stored as
   readable plaintext at rest.
4. **Given** "Save password" is unchecked, **When** the user connects, **Then**
   no password blob is persisted for that entry.

---

### User Story 2 - Creating, editing, and saving a bookmark is obvious (Priority: P1)

The connect window makes the bookmark lifecycle explicit: the user can create a
new bookmark from the current fields, select and edit an existing bookmark and
**save the changes back to it**, rename it, duplicate it, and delete it — each
via a clearly labelled action, with no dead controls.

**Why this priority**: The reported confusion; without an editable/saveable
bookmark the feature is half-usable.

**Independent Test**: Create a bookmark; reselect it; change the port and
password; save; reselect it → the changes are present. Duplicate it → a second
bookmark appears. Every button performs a defined, discoverable action.

**Acceptance Scenarios**:

1. **Given** field values entered, **When** the user chooses "Save as new
   bookmark", **Then** a named bookmark is created from those fields.
2. **Given** a selected bookmark whose fields were edited, **When** the user
   chooses "Save" (update), **Then** the edits (including a newly typed/saved
   password) are written back to that bookmark and survive reselecting it.
3. **Given** a selected bookmark, **When** the user chooses Duplicate, **Then**
   a copy is created (distinct name) — the control is no longer inert.
4. **Given** a selected bookmark, **When** Rename / Delete are used, **Then**
   they behave as labelled.
5. **Given** a bookmark with a stored password, **When** it is selected,
   **Then** the UI indicates a password is stored (without revealing it) and
   the "Save password" state reflects reality.
6. **Given** unsaved field edits and a switch to another bookmark, **Then** the
   user is not silently surprised — either edits apply only via an explicit
   Save, or the design makes the transient nature clear (documented choice).

---

### User Story 3 - Local keys and passphrases are handled correctly (Priority: P2)

Private-key authentication stores the key file path and (optionally) the
passphrase using the same correct, round-tripping, no-plaintext-at-rest
mechanism as passwords; loading a key and unlocking it with a saved passphrase
works.

**Acceptance Scenarios**:

1. **Given** key auth with "Save passphrase", **When** config is saved/reloaded
   (incl. quick-connect), **Then** the key file path and passphrase persist and
   the passphrase round-trips.
2. **Given** a saved passphrase, **When** connecting, **Then** the key unlocks
   without re-prompting.

---

### Edge Cases

- Quick-connect (no bookmark) vs. a named bookmark: both must persist their
  saved secrets across restart.
- Master password turned on/off after a secret was saved (re-encrypt path
  `PasswordManagerEvent` must cover the quick-connect entry too, not only
  bookmarks).
- "Save password" checked but the field left blank while a blob already exists
  (reuse vs. clear) must be well-defined.
- Editing a bookmark then Cancel must not mutate the stored bookmark.
- No blob is written when the corresponding Save flag is off (no secret leak).
- Deleting/duplicating updates the list and selection consistently.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The quick-connect entry MUST be persisted and restored (all its
  fields and its saved password/passphrase blobs), so a saved quick-connect
  password survives a restart.
- **FR-002**: Saved passwords and passphrases MUST round-trip through the
  password manager for both bookmarks and quick-connect, in both
  master-password-set and not-set modes, and MUST never be stored as readable
  plaintext at rest.
- **FR-003**: The connect window MUST provide explicit, discoverable actions to:
  create a new bookmark from the current fields, **save (update) edits back to
  the selected bookmark**, duplicate a bookmark, rename it, and delete it — with
  no inert controls.
- **FR-004**: Saving/updating a bookmark MUST capture the current field values,
  including a newly typed password (encrypted per FR-002) or the retained
  existing blob when the field is left blank and "Save password" stays on.
- **FR-005**: The window MUST make clear whether the selected bookmark has a
  stored password/passphrase, without revealing it, and the Save-secret
  checkbox state MUST reflect the stored reality.
- **FR-006**: Committing changes MUST be via explicit, discoverable actions
  (New / Duplicate / Rename / Delete / Save / Connect); Cancel MUST discard
  uncommitted **field** edits. (Structural actions take effect when invoked;
  a full atomic-staging model where Cancel also rolls back New/Rename/Delete is
  a documented follow-up — see research.md R4.)
- **FR-007**: When "Save password"/"Save passphrase" is off, no secret blob may
  be persisted for that entry, and any previously stored blob for that entry
  MUST be cleared on update.
- **FR-008**: The re-encrypt-on-master-password-change path MUST cover the
  quick-connect entry as well as all bookmarks.
- **FR-009**: All additional defects found by the audits (leaks, double-frees,
  wrong-size/flag decrypt, dead handlers, inconsistent password/passphrase
  handling) MUST be fixed; no regression to connecting, listing, or transfers.
- **FR-010**: The private-key file path MUST persist for bookmarks and
  quick-connect; a saved passphrase MUST unlock the key without re-prompting.

### Key Entities

- **Quick-connect entry** (`Config.QuickConnect`): the unnamed connection whose
  non-persistence is the core password bug.
- **Bookmark** (`CSFTPServer` in `Config.Bookmarks`): a named saved connection
  with an editable lifecycle.
- **Secret blob**: password/passphrase stored via the password manager
  (encrypted or scrambled), never plaintext at rest.

## Success Criteria *(mandatory)*

- **SC-001**: A quick-connect saved password is present and usable after a full
  Salamander restart (verified via the plugin registry key: a quick-connect
  entry with a password blob exists after save, and connect uses it).
- **SC-002**: A bookmark can be created, reselected, edited, saved, and the
  edits (including password) are present on reselect and after restart.
- **SC-003**: Every button in the connect window performs a defined action
  (no dead controls); the bookmark create/edit/save/duplicate/rename/delete set
  is complete and discoverable.
- **SC-004**: Secrets never appear as plaintext in the registry; they decrypt
  correctly with and without a master password.
- **SC-005**: Debug and Release x64 build clean; the SFTP dev test harness (if
  affected) still passes; no regression in connect/list/transfer.

## Assumptions

- Builds on features 008/009 (working transport, transfers, host-key store) and
  010 (UTF-8 control text). The fix is scoped to the connect window, the
  config persistence of the quick-connect entry, and the audited secret/key
  mechanics — not the SSH/transfer layers.
- Verification is autonomous where possible: builds; static confirmation of the
  persistence/round-trip fix; and, where the environment allows, launching the
  plugin path is limited (SFTP needs a live server), so the primary secret-
  persistence proof is the registry round-trip of a saved entry plus code
  review; the final live connect test is the user's.
- The password-manager API (`GetSalamanderPasswordManager`,
  `EncryptPassword`/`DecryptPassword`, `IsPasswordEncrypted`,
  `IsUsingMasterPassword`/`IsMasterPasswordSet`/`AskForMasterPassword`) is the
  existing, correct mechanism; the bug is in how the plugin uses/persists it,
  not in the manager.
