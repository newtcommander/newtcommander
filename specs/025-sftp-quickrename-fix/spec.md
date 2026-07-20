# Feature Specification: SFTP Plugin — Fix F2 Quick Rename ("Cannot rename")

**Feature Branch**: `025-sftp-quickrename-fix`
**Created**: 2026-07-20
**Status**: Draft
**Input**: User: F2 Quick Rename does not work in the SFTP plugin — renaming a
file or a directory always shows "Cannot rename \"[file/dir]\".." .

## Problem Statement

Renaming a file or directory over SFTP via F2 (Quick Rename) fails with the
message `Cannot rename "<name>": <error>`, where the error text is empty (the
user sees it end with `..`). Two defects were found and confirmed:

1. **The rename error is never captured (Medium — always visible).**
   `CSFTPSession::Rename` (session.cpp) calls `libssh2_sftp_rename_ex` and
   returns `rc == 0`, but on failure it does **not** call `SetLastErrorFromSsh`.
   So `LastErrorText` holds a stale/empty string from an earlier operation, and
   the message `IDS_ERR_RENAME` = `Cannot rename "%s": %s` prints an empty
   second field — exactly the `..` the user sees. This also hid the real cause
   during diagnosis.

2. **Rename cannot overwrite an existing target (Medium).** The plugin calls
   `libssh2_sftp_rename_ex(..., OVERWRITE | ATOMIC | NATIVE)`. libssh2 always
   negotiates SFTP protocol **version 3** (`LIBSSH2_SFTP_VERSION 3`), and the
   rename flags are only sent for version ≥ 5 — so against every real server
   (OpenSSH speaks v3) a plain `SSH_FXP_RENAME` is sent with **no overwrite**.
   Modern OpenSSH's `SSH_FXP_RENAME` refuses when the destination already
   exists (returns `SSH_FX_FAILURE`). Confirmed by a live libssh2 test against
   OpenSSH 9.2: renaming to a **new** name (file or directory) succeeds, but
   renaming onto an **existing** target fails with `rc=-31 "SFTP Protocol
   Error", sftp_errno=4`. The correct, portable way to do an atomic,
   overwrite-capable rename against OpenSSH is the `posix-rename@openssh.com`
   extension (`libssh2_sftp_posix_rename_ex`), which the test confirmed works.

The robust fix is to use `posix-rename@openssh.com` when the server advertises
it (OpenSSH and most servers do) and fall back to the standard rename
otherwise, and to always capture the libssh2 error so the message is
actionable. A diagnostic log line records the `from → to` and error.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Renaming a file or directory works (Priority: P1)

A user connected over SFTP presses F2 on a file or directory, types a new name,
and confirms; the item is renamed on the server and the panel refreshes.

**Independent Test**: Against an OpenSSH server, rename a file to a new name;
rename a directory to a new name; rename a file onto an existing name
(overwrite). All succeed. Verified at the libssh2 layer against the live test
server (posix-rename path) and by build; the final in-GUI F2 test is the user's.

**Acceptance Scenarios**:

1. **Given** a file, **When** the user F2-renames it to a name that does not
   exist, **Then** the rename succeeds.
2. **Given** a directory, **When** the user F2-renames it, **Then** it succeeds.
3. **Given** a target name that already exists, **When** the user renames onto
   it, **Then** the atomic overwrite succeeds (posix-rename) instead of failing.
4. **Given** a server that does not offer `posix-rename@openssh.com`, **When**
   the user renames to a new name, **Then** the standard rename still works
   (fallback, no regression).

---

### User Story 2 - A failed rename reports the real reason (Priority: P2)

When a rename genuinely cannot be done (e.g. permission denied), the message
shows the actual server/libssh2 error, not an empty string.

**Acceptance Scenarios**:

1. **Given** a rename that fails, **When** the error dialog appears, **Then** it
   contains the concrete libssh2/SFTP error text (no trailing empty `: `).

---

### Edge Cases

- Server offers posix-rename (OpenSSH) vs. one that does not (fallback path).
- Rename to an existing file/dir (overwrite) vs. to a new name.
- Non-ASCII / UTF-8 names (paths already flow through the UTF-8-correct
  `SFTPRealName` / `PosixPathAppend`).

## Requirements *(mandatory)*

- **FR-001**: `CSFTPSession::Rename` MUST perform an atomic, overwrite-capable
  rename using `posix-rename@openssh.com` when the server advertises it, and
  MUST fall back to the standard `SSH_FXP_RENAME` (with the OVERWRITE flag for
  v5+ servers) when it does not.
- **FR-002**: On failure, `CSFTPSession::Rename` MUST capture the libssh2 error
  into `LastErrorText` (via `SetLastErrorFromSsh`) so the user-facing message is
  meaningful.
- **FR-003**: A rename to a non-existent target (file or directory) MUST
  succeed; a rename onto an existing target MUST succeed via the atomic
  overwrite path (posix-rename).
- **FR-004**: No regression to rename against servers lacking the posix-rename
  extension (standard rename fallback).
- **FR-005**: The plugin SHOULD log the `from → to` of a rename attempt and the
  error on failure, to aid future diagnosis.

## Success Criteria *(mandatory)*

- **SC-001**: File and directory renames to new names succeed over SFTP.
- **SC-002**: Rename onto an existing target succeeds (atomic overwrite).
- **SC-003**: A failed rename shows the real error text (message no longer ends
  with an empty `: `).
- **SC-004**: Debug + Release x64 build clean; the live libssh2 rename test
  passes for file/dir/new/existing cases.

## Assumptions

- Builds on features 008/024. Scope is `CSFTPSession::Rename` (session.cpp) and,
  if needed, the QuickRename caller (fs.cpp) — not the transfer or listing
  layers.
- The vendored libssh2 exposes `libssh2_sftp_posix_rename_ex` (verified present)
  and the WinCNG build links it.
- The reported "always fails" may be the overwrite case and/or a case the empty
  error message hid; capturing the error makes any residual cause visible for a
  precise follow-up. The libssh2 layer itself renames correctly against OpenSSH.
