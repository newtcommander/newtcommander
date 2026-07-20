# Tasks: SFTP F2 Quick Rename fix

**Feature**: 025-sftp-quickrename-fix | [spec.md](spec.md) · [plan.md](plan.md) · [research.md](research.md)

## Phase A — Implement

- [x] **T001 (FR-001/FR-002/FR-005)** `session.cpp` `CSFTPSession::Rename`:
  - log the attempt (`LogFmt("Rename %s -> %s", from, to)`);
  - try `libssh2_sftp_posix_rename_ex` first (atomic, overwrite);
  - on non-zero, fall back to `libssh2_sftp_rename_ex(..., LIBSSH2_SFTP_RENAME_OVERWRITE)`;
  - on failure of both, `SetLastErrorFromSsh("rename")` so the error surfaces;
  - return `rc == 0`.
- [x] **T002** Update the `Rename` declaration comment in `session.h` to note the
  posix-rename-first behaviour (if a comment exists there).

## Phase B — Verify

- [x] **T010** Standalone libssh2 rename test against the OpenSSH test server:
  file→new, dir→new, file→existing (overwrite) all succeed via the
  posix-rename path.
- [x] **T011** Debug x64 build clean (`OPENSAL_BUILD_DIR` set → output in
  gitignored `/build`; no junk in `./src`).
- [x] **T012** Release x64 build clean.
- [x] **T013** Update memory / feature docs; final report. In-GUI F2 test is the
  user's final confirmation.
