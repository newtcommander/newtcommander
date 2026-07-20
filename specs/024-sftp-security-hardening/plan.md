# Implementation Plan: SFTP Security & Functional Review

**Branch**: `024-sftp-security-hardening` | **Date**: 2026-07-20 | **Spec**: [spec.md](spec.md)
**Input**: [spec.md](spec.md), [research.md](research.md)

## Summary

Fix the two reported bugs — the SSH handshake failure (client compiled without
ECDSA/ecdh, incompatible with modern/hardened servers) and the mojibake of the
file name in the change-owner/group dialog — and apply the consolidated
multi-agent security/quality findings (algorithm pinning, recursion-DoS cap,
local junction-follow data loss, credential/key Unicode handling, stored-secret
placeholder guard, cross-user upload guard, and a set of localized robustness
fixes). The handshake fix is proven end-to-end against a hardened OpenSSH server
in Docker (research.md R0). Scope excludes Ed25519/curve25519 (WinCNG backend
limitation) and a few deferred long-path/IDN/tooling items (research.md CF-D*).

## Technical Context

**Language/Version**: C++20 (`/std:c++latest`), MSVC v143 (VS2022)
**Primary Dependencies**: pure WinAPI; vendored libssh2 1.11.1_DEV (WinCNG
backend, `src/common/dep/libssh2`); Salamander plugin SDK (`src/plugins/shared`)
**Storage**: Windows Registry (config + secret blobs via the Salamander password
manager; host-key trust store)
**Testing**: `src/plugins/sftp/test/` — `harness.cpp` (pure-logic unit tests,
`build_and_run.cmd`) and `sftp_smoke.c` (live libssh2 handshake/auth/SFTP against
a real server); a hardened OpenSSH Docker server for the handshake before/after
**Target Platform**: Windows 11+ (ECDSA-in-WinCNG needs Win10+ at runtime)
**Project Type**: Single desktop plugin (`.spl` + `.slg`)
**Constraints**: Backward compatible (legacy DH/RSA servers must still connect);
incremental (touch only the fixed code); UTF-8-BOM + clang-format; no new
external dependencies; no libssh2 version bump
**Scope/Scope**: ~12 plugin source files + one build prop + the language `.rc2`

## Constitution Check

*GATE: passes.*

- **I. Build Reproducibility** — change is a preprocessor define in the tracked
  `sftp.props` plus source edits; no manual steps, no new artifacts. Builds via
  the existing `build.cmd`. ✅
- **II. Backward Compatibility** — enabling ECDSA and pinning algorithms is
  additive/hardening; the legacy DH-group + RSA path is preserved and
  regression-checked (research.md R0 / SC-002). No plugin-API or on-disk config
  format change. ✅
- **III. Incremental Modernization** — each fix is small, localized, and
  independently reviewable; no adjacent-code refactor. Deferrals documented. ✅
- **IV. Windows Platform Commitment** — pure WinAPI + WinCNG; Win11 target made
  explicit by the `WINVER` bump. No cross-platform layers. ✅
- **V. Plugin Architecture Preservation** — SFTP remains a self-contained plugin;
  no core changes; the missing-dependency principle is honored (uses the
  bundled WinCNG crypto, no external OpenSSL). ✅
- **VI. UI Consistency** — the mojibake fix uses the existing UTF-8 dialog helper;
  no new templates, no owner-draw/subclassing, no `ICC_STANDARD_CLASSES`. ✅

No violations → Complexity Tracking not required.

## Project Structure

### Documentation (this feature)

```text
specs/024-sftp-security-hardening/
├── spec.md        # what/why + acceptance
├── research.md    # consolidated agent findings + live reproduction
├── plan.md        # this file
└── tasks.md       # ordered implementation tasks
```

### Source Code (files touched)

```text
src/plugins/sftp/
├── vcxproj/sftp.props     # CF-1: +LIBSSH2_ECDSA_WINCNG, WINVER 0x0A00
├── session.cpp            # CF-3 (msg compose), CF-4 (method_pref), CF-9 (key W/mem),
│                          #   CF-13 (longEntry NUL), CF-19, CF-21, CF-27 (compress)
├── keyload.cpp            # CF-10 (.ppk reject)
├── dialogs.cpp            # CF-2 (owner/group U8), CF-8 (pwd/pass U8), CF-11 (placeholder)
├── fs.cpp                 # CF-7 (nav path), CF-12 (user check), CF-14 (wipe),
│                          #   CF-15 (decrypt trunc), CF-18 (MBtoWC), CF-20 (passphrase
│                          #   prompt), CF-23 (ViewFile buf), CF-26 (RealName), CF-28
├── operats.cpp            # CF-5 (recursion cap), CF-6 (reparse point), CF-16 (reserved
│                          #   names), CF-17 (partial file), CF-18/CF-19
├── listing.cpp            # CF-19 (_strdup checks)
├── hostkeys.cpp           # CF-19 (_strdup checks)
└── lang/lang.rc2 (+lang.rh) # CF-3 strings; CF-10/CF-20 new strings if needed
```

**Structure Decision**: Single self-contained plugin; all edits live under
`src/plugins/sftp/`. No new files except possibly a couple of string-table
entries. The `sftp_smoke.c` dev test is reused (compiled twice, with/without the
ECDSA define) for the live before/after; the pure-logic `harness.cpp` guards
`sftputils.cpp` (touched only if a new helper lands there).

## Approach by fix group

1. **Handshake (CF-1, CF-3)** — the headline. Add `LIBSSH2_ECDSA_WINCNG` and
   bump `WINVER`/`_WIN32_WINNT` in `sftp.props`; fix the error-message compose so
   the literal `%s` disappears. Verify by compiling `sftp_smoke` with and without
   the define and running both against the hardened Docker server (before: fails;
   after: connects), plus a legacy-config regression run.
2. **Transport hardening (CF-4)** — call `libssh2_session_method_pref` right after
   `libssh2_session_init_ex`, pinning modern-only KEX/cipher/MAC/hostkey lists
   that still include the interop floor (group14-sha256, ecdh, aes-ctr, chacha20,
   rsa-sha2, ecdsa). Non-fatal if a pref call fails (log and continue).
3. **File-operation safety (CF-5, CF-6, CF-16, CF-17)** — depth-cap the four
   recursive walkers; skip/relink local reparse points on move; extend the
   download sanitizer; clean up partial overwrite targets.
4. **Path correctness (CF-7, CF-26)** — internal navigation uses the full path.
5. **Credential/key Unicode (CF-8, CF-9)** + **secret hygiene/robustness (CF-11,
   CF-12, CF-13, CF-14, CF-15, CF-18, CF-19, CF-20, CF-21, CF-23, CF-27, CF-28)**
   and **key-format reject (CF-10)** — localized edits per the register.

## Verification strategy

- **Build**: Debug x64 clean via `build.cmd` (plugin + `english.slg`). Release
  x64 if the running-exe lock allows (known LNK1104 pitfall if the user's
  Salamander is running).
- **Unit**: `test\build_and_run.cmd` (sftputils logic) stays green.
- **Live handshake (decisive)**: `sftp_smoke` before/after against the hardened
  server proves CF-1; a legacy-config server proves CF-002 non-regression.
- **Static**: confirm `LIBSSH2_ECDSA=1` compiles the ecdh/ecdsa methods into the
  negotiated set.
- **Code review**: the mojibake and secret-handling fixes are verified by reading
  the resulting dialog/auth paths; live end-to-end connect to the user's own
  server is the user's final confirmation.
```
