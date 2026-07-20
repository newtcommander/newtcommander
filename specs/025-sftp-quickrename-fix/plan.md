# Implementation Plan: SFTP F2 Quick Rename fix

**Branch**: `025-sftp-quickrename-fix` | **Date**: 2026-07-20
**Spec**: [spec.md](spec.md) · **Research**: [research.md](research.md)

## Summary

Fix SFTP rename so F2 Quick Rename works: make `CSFTPSession::Rename` use the
atomic, overwrite-capable `posix-rename@openssh.com` extension with a fallback
to the standard rename, and capture the libssh2 error on failure (the missing
capture is why the message showed an empty reason). Add a diagnostic log line.
Proven at the libssh2 layer against the live OpenSSH test server.

## Technical Context

**Language/Version**: C++20, MSVC v143 (VS2022)
**Primary Dependencies**: vendored libssh2 1.11.1_DEV (WinCNG); Salamander SDK
**Target Platform**: Windows 11+
**Testing**: standalone libssh2 rename test against the OpenSSH Docker server
(file/dir, new/existing target); Debug+Release x64 build
**Constraints**: backward compatible (fallback for servers without posix-rename);
incremental (touch only `Rename`); no new dependencies; UTF-8-BOM + clang-format

## Constitution Check

- **II. Backward Compatibility** — posix-rename is tried first but falls back to
  the existing standard rename; no API/behavior regression. ✅
- **III. Incremental Modernization** — change localized to `CSFTPSession::Rename`
  (+ its declaration comment); no adjacent refactor. ✅
- **IV. Windows Platform Commitment** — pure WinAPI + vendored libssh2. ✅
- **V. Plugin Architecture Preservation** — self-contained plugin change. ✅
  (I. Build Reproducibility and VI. UI Consistency unaffected.)

No violations → no Complexity Tracking.

## Project Structure

```text
specs/025-sftp-quickrename-fix/
├── spec.md
├── research.md
├── plan.md
└── tasks.md

src/plugins/sftp/
└── session.cpp        # CSFTPSession::Rename: posix-rename-first + fallback +
                       #   SetLastErrorFromSsh on failure + LogFmt from->to
```

**Structure Decision**: Single self-contained change in `session.cpp`. The
QuickRename caller (`fs.cpp`) already builds correct paths and shows
`Session.GetLastErrorText()`, so no change is required there once `Rename`
captures the error.

## Approach

`CSFTPSession::Rename(from, to)`:

```
if (Sftp == NULL) return FALSE;
LogFmt("Rename %s -> %s", from, to);
rc = libssh2_sftp_posix_rename_ex(Sftp, from, len, to, len);   // atomic+overwrite
if (rc != 0)
    rc = libssh2_sftp_rename_ex(Sftp, from, len, to, len,
                                LIBSSH2_SFTP_RENAME_OVERWRITE);  // fallback
if (rc != 0)
    SetLastErrorFromSsh(LoadStr(IDS_ERR_RENAMEFAIL_PREFIX?) or "rename");
return rc == 0;
```

Notes:
- `posix_rename` returns `LIBSSH2_FX_OP_UNSUPPORTED` immediately when the server
  did not advertise the extension, so the fallback path is cheap.
- Keep only `LIBSSH2_SFTP_RENAME_OVERWRITE` in the fallback (ATOMIC/NATIVE are
  v5+ only and unnecessary; ATOMIC may be rejected by some v5 servers).
- `SetLastErrorFromSsh` composes the prefix + libssh2 last-error text.

## Verification strategy

- **libssh2 layer (decisive):** the standalone rename test already proves
  posix-rename works for file/dir/new/existing against the live OpenSSH server;
  re-confirm after wiring the same sequence.
- **Build:** Debug + Release x64 clean (output to `$(OPENSAL_BUILD_DIR)` =
  gitignored `/build`).
- **In-GUI F2:** the user's final confirmation (cannot be driven headlessly).
