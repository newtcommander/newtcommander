# Research & Root-Cause: SFTP F2 Quick Rename fails

**Feature**: 025-sftp-quickrename-fix
**Date**: 2026-07-20

## Investigation path

1. **QuickRename flow (fs.cpp:834)** — verified correct against the SDK
   contract (spl_fs.h): `mode==1` returns FALSE (use the standard in-place
   dialog), `mode==2` receives the new base name; the plugin builds
   `from = PosixPathAppend(Path, SFTPRealName(&file))` and
   `to = PosixPathAppend(Path, newName)` and calls `Session.Rename(from, to)`.
   The core caller (fileswn5.cpp:2485/2523) passes a clean base name in
   `newName`. Path construction (PosixPathAppend, sftputils.cpp:229) is correct
   (root, trailing-slash, and normal cases). `EnsureConnected` does not
   reconnect when already connected, so `Sftp` stays valid; `IsConnected()`
   tracks `Connected`, which is set/cleared together with `Sftp`.

2. **libssh2 rename layer** — `libssh2_sftp_rename_ex` sends a plain
   `SSH_FXP_RENAME`; the OVERWRITE/ATOMIC/NATIVE flags are only serialized for
   SFTP **version ≥ 5** (sftp.c:2925). libssh2 always requests **version 3**
   (`LIBSSH2_SFTP_VERSION 3`, libssh2_sftp.h:58), so the flags are never sent —
   every rename is a plain, non-overwriting rename.

3. **Live reproduction** — a standalone libssh2 test (WinCNG, ECDSA build)
   against the running OpenSSH 9.2 test server (localhost:2222):

   | case | flags | result |
   |------|-------|--------|
   | file → new name | OVERWRITE\|ATOMIC\|NATIVE | **rc=0 (ok)** |
   | file → new name | 0 | rc=0 (ok) |
   | file → new name | posix_rename | rc=0 (ok) |
   | **directory → new name** | OVERWRITE\|ATOMIC\|NATIVE | **rc=0 (ok)** |
   | **file → EXISTING target** | OVERWRITE\|ATOMIC\|NATIVE | **rc=-31 "SFTP Protocol Error", sftp_errno=4 (FAILURE)** |

   So the standard rename **works for new names** (file and dir) but **fails to
   overwrite an existing target** — because the OVERWRITE flag is dropped on
   v3 and modern OpenSSH refuses `SSH_FXP_RENAME` when the destination exists.
   The `posix-rename@openssh.com` extension performs the atomic overwrite and
   succeeded in the test.

## Confirmed defects

- **RC-1 — error never captured.** `CSFTPSession::Rename` (session.cpp:791)
  does not call `SetLastErrorFromSsh` on failure, so `IDS_ERR_RENAME`
  (`Cannot rename "%s": %s`) prints an empty/stale second field — the `..` the
  user reported, and the reason the failure was opaque.

- **RC-2 — no overwrite.** Rename onto an existing target fails because the
  OVERWRITE intent is lost on SFTP v3. The portable fix is the
  `posix-rename@openssh.com` extension (atomic + overwrite), with a fallback to
  the standard rename for servers that do not advertise it.

## Fix decision

`CSFTPSession::Rename`:
1. Try `libssh2_sftp_posix_rename_ex(Sftp, from, to)` first — atomic, overwrite,
   supported by OpenSSH and most servers (returns `LIBSSH2_FX_OP_UNSUPPORTED`
   fast when not advertised, so the fallback is cheap).
2. If that is unsupported/fails, fall back to
   `libssh2_sftp_rename_ex(..., LIBSSH2_SFTP_RENAME_OVERWRITE)` (OVERWRITE only;
   ATOMIC/NATIVE dropped — they are v5+ only and add nothing on v3, and ATOMIC
   can be rejected by some v5+ servers).
3. On failure of both, call `SetLastErrorFromSsh("rename")` so the real error
   surfaces.
4. `LogFmt` the `from → to` (and, on failure, the error) for diagnosis.

## Scope / limits

- Verified at the libssh2 layer against the live OpenSSH test server. The
  in-GUI F2 flow cannot be driven headlessly, so the final F2 confirmation is
  the user's — but with RC-2 fixed (overwrite works) and RC-1 fixed (real error
  shown), any residual cause is now both less likely and fully visible.
- The plugin's rename path is UTF-8-correct already (`SFTPRealName`,
  `PosixPathAppend`); no encoding change needed.
